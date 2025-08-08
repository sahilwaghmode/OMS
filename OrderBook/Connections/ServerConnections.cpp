//
//  ServerConnections.cpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 24/07/24.
//
#include "ServerConnections.hpp"
#include "CSVReader.hpp"
#include "SocketConnection.hpp"
#include <iostream>

// kqueue
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
// kqueue

#include "Msg.hpp"
#include "SimpleLogger.h"
// #include "FileData.hpp"

extern Common::SimpleLogger *logger;

ServerConnections::ServerConnections(unsigned int port)
    : _port(port), _server_socket(port) {
  _server_socket.bind();
}

void ServerConnections::start_listening_for_client_connections() {
  while (true) {
    int newSocket = 0;
    logger->info("started listening on the socket");
    while (newSocket == 0) {
      newSocket = _server_socket.start_listening();
    }

    logger->info("connection received ", newSocket, "started threas");
    auto newthread = std::thread(start_listening_to_client, newSocket, this);
    _client_connections.insert({newSocket, std::move(newthread)});
  }
}

void ServerConnections::start_listening_to_client(int client_fd,
                                                  ServerConnections *instance) {
  SocketConnection::send_data_to_fd("Hello From Server", client_fd);
  int kq = kqueue();
  struct kevent change;
  EV_SET(&change, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0,
         sizeof(Msg::MsgHeader), NULL);

  while (1) {
    struct kevent event;
    struct timespec timeout = {10, 0};
    int newEvent = kevent(kq, &change, 1, &event, 1, &timeout);
    if (newEvent > 0) {
      if (event.filter == EVFILT_READ) {
        auto data = SocketConnection::read_data_from_fd(client_fd);
        if (data == "SEND") {
          SocketConnection::send_data_to_fd("Hello From Server", client_fd);
        } else if (data == "CLOSE") {
          close(client_fd);
          break;
        }
      }
    } else if (newEvent == 0) // timeout
    {
      logger->info("Client ", client_fd, " has timed out.");
      instance->remove_client_on_close(client_fd);
      break;
    }
  }
}

void ServerConnections::remove_client_on_close(int client_fd) {
  _client_connections.erase(client_fd);
  close(client_fd);
  logger->info("connection closed ", client_fd);
}
