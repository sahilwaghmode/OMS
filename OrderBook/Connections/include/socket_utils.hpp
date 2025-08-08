#pragma once

#ifdef _WIN32
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
typedef int socklen_t;
#define close closesocket
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/epoll.h>
#endif
#endif

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

namespace Common {
struct SocketCfg {
  std::string ip_;
  std::string iface_;
  int port_ = -1;
  bool is_udp_ = false;
  bool is_listening_ = false;
  bool needs_so_timestamp_ = false;

  auto toString() const {
    std::stringstream ss;
    ss << "SocketCfg[ip:" << ip_ << " iface:" << iface_ << " port:" << port_
       << " is_udp:" << is_udp_ << " is_listening:" << is_listening_
       << " needs_SO_timestamp:" << needs_so_timestamp_ << "]";

    return ss.str();
  }
};

/// Represents the maximum number of pending / unaccepted TCP connections.
constexpr int MaxTCPServerBacklog = 1024;

/// Convert interface name "eth0" to ip "123.123.123.123".
inline auto getIfaceIP(const std::string &iface) -> std::string {
#ifdef _WIN32
  // Windows implementation using GetAdaptersAddresses
  ULONG bufferSize = 0;
  GetAdaptersAddresses(AF_INET, 0, nullptr, nullptr, &bufferSize);

  auto buffer = std::make_unique<char[]>(bufferSize);
  PIP_ADAPTER_ADDRESSES pAddresses =
      reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.get());

  if (GetAdaptersAddresses(AF_INET, 0, nullptr, pAddresses, &bufferSize) ==
      NO_ERROR) {
    for (PIP_ADAPTER_ADDRESSES pCurrent = pAddresses; pCurrent;
         pCurrent = pCurrent->Next) {
      std::string adapterName(pCurrent->AdapterName);
      if (adapterName.find(iface) != std::string::npos ||
          (pCurrent->FriendlyName &&
           std::wstring(pCurrent->FriendlyName)
                   .find(std::wstring(iface.begin(), iface.end())) !=
               std::wstring::npos)) {
        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast =
                 pCurrent->FirstUnicastAddress;
             pUnicast; pUnicast = pUnicast->Next) {
          if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
            char buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,
                      &((sockaddr_in *)pUnicast->Address.lpSockaddr)->sin_addr,
                      buf, INET_ADDRSTRLEN);
            return std::string(buf);
          }
        }
      }
    }
  }
  return "";
#else
  char buf[NI_MAXHOST] = {'\0'};
  ifaddrs *ifaddr = nullptr;

  if (getifaddrs(&ifaddr) != -1) {
    for (ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET &&
          iface == ifa->ifa_name) {
        getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buf, sizeof(buf), NULL,
                    0, NI_NUMERICHOST);
        break;
      }
    }
    freeifaddrs(ifaddr);
  }

  return buf;
#endif
}

/// Sockets will not block on read, but instead return immediately if data is
/// not available.
inline auto setNonBlocking(int fd) -> bool {
#ifdef _WIN32
  u_long mode = 1;
  return (ioctlsocket(fd, FIONBIO, &mode) == 0);
#else
  const auto flags = fcntl(fd, F_GETFL, 0);
  if (flags & O_NONBLOCK)
    return true;
  return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
#endif
}

/// Disable Nagle's algorithm and associated delays.
inline auto disableNagle(int fd) -> bool {
  int one = 1;
  return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                     reinterpret_cast<const char *>(&one), sizeof(one)) != -1);
}

/// Allow software receive timestamps on incoming packets.
inline auto setSOTimestamp(int fd) -> bool {
#ifdef _WIN32
  // Windows doesn't have SO_TIMESTAMP, return true to maintain compatibility
  return true;
#else
  int one = 1;
  return (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP,
                     reinterpret_cast<void *>(&one), sizeof(one)) != -1);
#endif
}

/// Add / Join membership / subscription to the multicast stream specified and
/// on the interface specified.
inline auto join(int fd, const std::string &ip) -> bool {
  const ip_mreq mreq{{inet_addr(ip.c_str())}, {htonl(INADDR_ANY)}};
  return (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                     reinterpret_cast<const char *>(&mreq),
                     sizeof(mreq)) != -1);
}

/// Create a TCP / UDP socket to either connect to or listen for data on or
/// listen for connections on the specified interface and IP:port information.
[[nodiscard]] inline auto createSocket(Logger &logger,
                                       const SocketCfg &socket_cfg) -> int {
  std::string time_str;

#ifdef _WIN32
  // Initialize Winsock
  static bool wsaInitialized = false;
  if (!wsaInitialized) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    wsaInitialized = true;
  }
#endif

  const auto ip =
      socket_cfg.ip_.empty() ? getIfaceIP(socket_cfg.iface_) : socket_cfg.ip_;
  logger.log("%:% %() % cfg:%\n", __FILE__, __LINE__, __FUNCTION__,
             Common::getCurrentTimeStr(&time_str), socket_cfg.toString());

  const int input_flags = (socket_cfg.is_listening_ ? AI_PASSIVE : 0) |
                          (AI_NUMERICHOST | AI_NUMERICSERV);
  const addrinfo hints{input_flags,
                       AF_INET,
                       socket_cfg.is_udp_ ? SOCK_DGRAM : SOCK_STREAM,
                       socket_cfg.is_udp_ ? IPPROTO_UDP : IPPROTO_TCP,
                       0,
                       0,
                       nullptr,
                       nullptr};

  addrinfo *result = nullptr;
  const auto rc = getaddrinfo(
      ip.c_str(), std::to_string(socket_cfg.port_).c_str(), &hints, &result);
#ifdef _WIN32
  ASSERT(!rc, "getaddrinfo() failed. error:" + std::string(gai_strerrorA(rc)) +
                  "errno:" + std::to_string(WSAGetLastError()));
#else
  ASSERT(!rc, "getaddrinfo() failed. error:" + std::string(gai_strerror(rc)) +
                  "errno:" + strerror(errno));
#endif

  int socket_fd = -1;
  int one = 1;
  for (addrinfo *rp = result; rp; rp = rp->ai_next) {
    ASSERT((socket_fd =
                socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) != -1,
#ifdef _WIN32
           "socket() failed. errno:" + std::to_string(WSAGetLastError()));
#else
           "socket() failed. errno:" + std::string(strerror(errno)));
#endif

    ASSERT(setNonBlocking(socket_fd),
#ifdef _WIN32
           "setNonBlocking() failed. errno:" +
               std::to_string(WSAGetLastError()));
#else
           "setNonBlocking() failed. errno:" + std::string(strerror(errno)));
#endif

    if (!socket_cfg.is_udp_) { // disable Nagle for TCP sockets.
      ASSERT(disableNagle(socket_fd),
#ifdef _WIN32
             "disableNagle() failed. errno:" +
                 std::to_string(WSAGetLastError()));
#else
             "disableNagle() failed. errno:" + std::string(strerror(errno)));
#endif
    }

    if (!socket_cfg
             .is_listening_) { // establish connection to specified address.
      ASSERT(connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != 1,
#ifdef _WIN32
             "connect() failed. errno:" + std::to_string(WSAGetLastError()));
#else
             "connect() failed. errno:" + std::string(strerror(errno)));
#endif
    }

    if (socket_cfg.is_listening_) { // allow re-using the address in the call to
                                    // bind()
      ASSERT(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                        reinterpret_cast<const char *>(&one), sizeof(one)) == 0,
#ifdef _WIN32
             "setsockopt() SO_REUSEADDR failed. errno:" +
                 std::to_string(WSAGetLastError()));
#else
             "setsockopt() SO_REUSEADDR failed. errno:" +
                 std::string(strerror(errno)));
#endif
    }

    if (socket_cfg.is_listening_) {
      // bind to the specified port number.
      uint16_t port = htons(socket_cfg.port_);
      const sockaddr_in addr{
          AF_INET, static_cast<sa_family_t>(port), {htonl(INADDR_ANY)}, {}};
      // ASSERT(bind(socket_fd,
      //             socket_cfg.is_udp_
      //                 ? reinterpret_cast<const struct sockaddr *>(&addr)
      //                 : rp->ai_addr,
      //             sizeof(addr)) == 0,
      // #ifdef _WIN32
      //              "bind() failed. errno:" +
      //              std::to_string(WSAGetLastError()));
      // #else
      //              "bind() failed. errno:%" + std::string(strerror(errno)));
      // #endif
    }

    if (!socket_cfg.is_udp_ &&
        socket_cfg.is_listening_) { // listen for incoming TCP connections.
      ASSERT(listen(socket_fd, MaxTCPServerBacklog) == 0,
#ifdef _WIN32
             "listen() failed. errno:" + std::to_string(WSAGetLastError()));
#else
             "listen() failed. errno:" + std::string(strerror(errno)));
#endif
    }

    if (socket_cfg.needs_so_timestamp_) { // enable software receive timestamps.
      ASSERT(setSOTimestamp(socket_fd),
#ifdef _WIN32
             "setSOTimestamp() failed. errno:" +
                 std::to_string(WSAGetLastError()));
#else
             "setSOTimestamp() failed. errno:" + std::string(strerror(errno)));
#endif
    }
  }

  return socket_fd;
}
} // namespace Common