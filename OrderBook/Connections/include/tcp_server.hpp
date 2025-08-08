#pragma once

#include "Logging.h"
#include "tcp_socket.hpp"
#include <functional>
#include <vector>

// Platform-specific includes and definitions
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
// Windows uses select() or IOCP, we'll use select for simplicity
typedef fd_set poll_set_t;
typedef struct {
  int fd;
  uint32_t events;
  uint32_t revents;
} poll_event_t;
#elif defined(__linux__)
#include <sys/epoll.h>
typedef int poll_set_t;
typedef epoll_event poll_event_t;
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/event.h>
typedef int poll_set_t;
typedef struct kevent poll_event_t;
#endif

namespace Common {
struct TCPServer {
  explicit TCPServer(Logger &logger)
      : listener_socket_(logger), logger_(logger) {
    initializePlatformSpecific();
  }

  ~TCPServer() { cleanupPlatformSpecific(); }

  /// Start listening for connections on the provided interface and port.
  auto listen(const std::string &iface, int port) -> void;

  /// Check for new connections or dead connections and update containers that
  /// track the sockets.
  auto poll() noexcept -> void;

  /// Publish outgoing data from the send buffer and read incoming data from the
  /// receive buffer.
  auto sendAndRecv() noexcept -> void;

  /// Initialize platform-specific polling mechanism
  auto initializePlatformSpecific() -> void;

  /// Cleanup platform-specific resources
  auto cleanupPlatformSpecific() -> void;

  /// Add and remove socket file descriptors to and from the polling list.
  auto addToPollList(TCPSocket *socket) -> bool;

  /// Remove socket from polling list
  auto removeFromPollList(TCPSocket *socket) -> void;

  /// Platform-specific polling file descriptor/handle
#ifdef _WIN32
  // Windows doesn't have a single poll fd, we'll manage fd_sets
  poll_set_t read_fds_, write_fds_, except_fds_;
  int max_fd_ = -1;
#else
  // Linux epoll_fd or macOS/BSD kqueue fd
  poll_set_t poll_fd_ = -1;
#endif

  TCPSocket listener_socket_;

  // Platform-agnostic event array
  static constexpr int MAX_EVENTS = 1024;
  poll_event_t events_[MAX_EVENTS];

  /// Collection of all sockets, sockets for incoming data, sockets for outgoing
  /// data and dead connections.
  std::vector<TCPSocket *> receive_sockets_, send_sockets_;

  /// Function wrapper to call back when data is available.
  std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback_ = nullptr;
  /// Function wrapper to call back when all data across all TCPSockets has been
  /// read and dispatched this round.
  std::function<void()> recv_finished_callback_ = nullptr;

  std::string time_str_;
  Logger &logger_;
};
} // namespace Common