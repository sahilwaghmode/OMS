#include "tcp_server.hpp"

namespace Common {

#ifdef _WIN32
// Windows implementation using select()
auto TCPServer::initializePlatformSpecific() -> void {
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  FD_ZERO(&read_fds_);
  FD_ZERO(&write_fds_);
  FD_ZERO(&except_fds_);
  max_fd_ = -1;
}

auto TCPServer::cleanupPlatformSpecific() -> void { WSACleanup(); }

auto TCPServer::addToPollList(TCPSocket *socket) -> bool {
  int fd = socket->socket_fd_;
  FD_SET(fd, &read_fds_);
  if (fd > max_fd_)
    max_fd_ = fd;
  return true;
}

auto TCPServer::removeFromPollList(TCPSocket *socket) -> void {
  int fd = socket->socket_fd_;
  FD_CLR(fd, &read_fds_);
  FD_CLR(fd, &write_fds_);
  FD_CLR(fd, &except_fds_);
}

#elif defined(__linux__)
// Linux implementation using epoll
auto TCPServer::initializePlatformSpecific() -> void {
  // Nothing needed for Linux epoll
}

auto TCPServer::cleanupPlatformSpecific() -> void {
  if (poll_fd_ >= 0) {
    close(poll_fd_);
  }
}

auto TCPServer::addToPollList(TCPSocket *socket) -> bool {
  epoll_event ev{EPOLLET | EPOLLIN, {reinterpret_cast<void *>(socket)}};
  return !epoll_ctl(poll_fd_, EPOLL_CTL_ADD, socket->socket_fd_, &ev);
}

auto TCPServer::removeFromPollList(TCPSocket *socket) -> void {
  epoll_ctl(poll_fd_, EPOLL_CTL_DEL, socket->socket_fd_, nullptr);
}

#elif defined(__APPLE__) || defined(__FreeBSD__)
// macOS/BSD implementation using kqueue
auto TCPServer::initializePlatformSpecific() -> void {
  // Nothing needed for kqueue initialization
}

auto TCPServer::cleanupPlatformSpecific() -> void {
  if (poll_fd_ >= 0) {
    close(poll_fd_);
  }
}

auto TCPServer::addToPollList(TCPSocket *socket) -> bool {
  struct kevent ev;
  EV_SET(&ev, socket->socket_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
         socket);
  return kevent(poll_fd_, &ev, 1, nullptr, 0, nullptr) != -1;
}

auto TCPServer::removeFromPollList(TCPSocket *socket) -> void {
  struct kevent ev;
  EV_SET(&ev, socket->socket_fd_, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
  kevent(poll_fd_, &ev, 1, nullptr, 0, nullptr);
}
#endif

/// Start listening for connections on the provided interface and port.
auto TCPServer::listen(const std::string &iface, int port) -> void {
#ifdef _WIN32
  // Windows doesn't need a poll_fd_, it uses fd_sets directly
#elif defined(__linux__)
  poll_fd_ = epoll_create(1);
  ASSERT(poll_fd_ >= 0,
         "epoll_create() failed error:" + std::string(std::strerror(errno)));
#elif defined(__APPLE__) || defined(__FreeBSD__)
  poll_fd_ = kqueue();
  ASSERT(poll_fd_ >= 0,
         "kqueue() failed error:" + std::string(std::strerror(errno)));
#endif

  ASSERT(listener_socket_.connect("", iface, port, true) >= 0,
         "Listener socket failed to connect. iface:" + iface +
             " port:" + std::to_string(port) +
             " error:" + std::string(std::strerror(errno)));

  ASSERT(addToPollList(&listener_socket_),
         "Failed to add listener to poll list. error:" +
             std::string(std::strerror(errno)));
}

/// Publish outgoing data from the send buffer and read incoming data from the
/// receive buffer.
auto TCPServer::sendAndRecv() noexcept -> void {
  auto recv = false;

  std::for_each(receive_sockets_.begin(), receive_sockets_.end(),
                [&recv](auto socket) { recv |= socket->sendAndRecv(); });

  if (recv) // There were some events and they have all been dispatched, inform
            // listener.
    recv_finished_callback_();

  std::for_each(send_sockets_.begin(), send_sockets_.end(),
                [](auto socket) { socket->sendAndRecv(); });
}

/// Check for new connections or dead connections and update containers that
/// track the sockets.
auto TCPServer::poll() noexcept -> void {
  const int max_events = 1 + send_sockets_.size() + receive_sockets_.size();
  int n = 0;
  bool have_new_connection = false;

#ifdef _WIN32
  // Windows select() implementation
  fd_set temp_read_fds = read_fds_;
  fd_set temp_write_fds = write_fds_;
  fd_set temp_except_fds = except_fds_;

  struct timeval timeout = {0, 0}; // Non-blocking
  n = select(max_fd_ + 1, &temp_read_fds, &temp_write_fds, &temp_except_fds,
             &timeout);

  if (n > 0) {
    // Simulate epoll-like events for compatibility
    int event_count = 0;
    for (auto *socket : receive_sockets_) {
      if (FD_ISSET(socket->socket_fd_, &temp_read_fds)) {
        events_[event_count].fd = socket->socket_fd_;
        events_[event_count].events = POLLIN;
        events_[event_count].revents = POLLIN;
        event_count++;
      }
    }
    for (auto *socket : send_sockets_) {
      if (FD_ISSET(socket->socket_fd_, &temp_write_fds)) {
        events_[event_count].fd = socket->socket_fd_;
        events_[event_count].events = POLLOUT;
        events_[event_count].revents = POLLOUT;
        event_count++;
      }
    }
    if (FD_ISSET(listener_socket_.socket_fd_, &temp_read_fds)) {
      events_[event_count].fd = listener_socket_.socket_fd_;
      events_[event_count].events = POLLIN;
      events_[event_count].revents = POLLIN;
      event_count++;
    }
    n = event_count;
  }

#elif defined(__linux__)
  // Linux epoll implementation
  n = epoll_wait(poll_fd_, events_, max_events, 0);

#elif defined(__APPLE__) || defined(__FreeBSD__)
  // macOS/BSD kqueue implementation
  struct timespec timeout = {0, 0}; // Non-blocking
  n = kevent(poll_fd_, nullptr, 0, events_, max_events, &timeout);
#endif

  for (int i = 0; i < n; ++i) {
    const auto &event = events_[i];
    TCPSocket *socket = nullptr;
    bool is_read_event = false, is_write_event = false, is_error_event = false;

#ifdef _WIN32
    // Windows: Find socket by fd
    if (event.fd == listener_socket_.socket_fd_) {
      socket = &listener_socket_;
    } else {
      // Find socket in our containers
      for (auto *s : receive_sockets_) {
        if (s->socket_fd_ == event.fd) {
          socket = s;
          break;
        }
      }
      if (!socket) {
        for (auto *s : send_sockets_) {
          if (s->socket_fd_ == event.fd) {
            socket = s;
            break;
          }
        }
      }
    }
    is_read_event = (event.revents & POLLIN);
    is_write_event = (event.revents & POLLOUT);
    is_error_event = (event.revents & (POLLERR | POLLHUP));

#elif defined(__linux__)
    // Linux epoll
    socket = reinterpret_cast<TCPSocket *>(event.data.ptr);
    is_read_event = (event.events & EPOLLIN);
    is_write_event = (event.events & EPOLLOUT);
    is_error_event = (event.events & (EPOLLERR | EPOLLHUP));

#elif defined(__APPLE__) || defined(__FreeBSD__)
    // macOS/BSD kqueue
    socket = reinterpret_cast<TCPSocket *>(event.udata);
    is_read_event = (event.filter == EVFILT_READ);
    is_write_event = (event.filter == EVFILT_WRITE);
    is_error_event = (event.flags & EV_ERROR);
#endif

    if (!socket)
      continue;

    // Check for new connections.
    if (is_read_event) {
      if (socket == &listener_socket_) {
        logger_.log("%:% %() % READ listener_socket:%\n", __FILE__, __LINE__,
                    __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                    socket->socket_fd_);
        have_new_connection = true;
        continue;
      }
      logger_.log("%:% %() % READ socket:%\n", __FILE__, __LINE__, __FUNCTION__,
                  Common::getCurrentTimeStr(&time_str_), socket->socket_fd_);
      if (std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) ==
          receive_sockets_.end())
        receive_sockets_.push_back(socket);
    }

    if (is_write_event) {
      logger_.log("%:% %() % WRITE socket:%\n", __FILE__, __LINE__,
                  __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                  socket->socket_fd_);
      if (std::find(send_sockets_.begin(), send_sockets_.end(), socket) ==
          send_sockets_.end())
        send_sockets_.push_back(socket);
    }

    if (is_error_event) {
      logger_.log("%:% %() % ERROR socket:%\n", __FILE__, __LINE__,
                  __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                  socket->socket_fd_);
      if (std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) ==
          receive_sockets_.end())
        receive_sockets_.push_back(socket);
    }
  }

  // Accept a new connection, create a TCPSocket and add it to our containers.
  while (have_new_connection) {
    logger_.log("%:% %() % have_new_connection\n", __FILE__, __LINE__,
                __FUNCTION__, Common::getCurrentTimeStr(&time_str_));
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    int fd = accept(listener_socket_.socket_fd_,
                    reinterpret_cast<sockaddr *>(&addr), &addr_len);
    if (fd == -1)
      break;

    ASSERT(setNonBlocking(fd) && disableNagle(fd),
           "Failed to set non-blocking or no-delay on socket:" +
               std::to_string(fd));

    logger_.log("%:% %() % accepted socket:%\n", __FILE__, __LINE__,
                __FUNCTION__, Common::getCurrentTimeStr(&time_str_), fd);

    auto socket = new TCPSocket(logger_);
    socket->socket_fd_ = fd;
    socket->recv_callback_ = recv_callback_;
    ASSERT(addToPollList(socket),
           "Unable to add socket. error:" + std::string(std::strerror(errno)));

    if (std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) ==
        receive_sockets_.end())
      receive_sockets_.push_back(socket);
  }
}
} // namespace Common