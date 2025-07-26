//
//  Connection.cpp
//  OrderBook
//
//  Created by Sahil Waghmode on 03/08/24.
//

#include "ClientConnection.hpp"
#include <iostream>

// kqueue
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
// kqueue
#include "Logger.hpp"

extern Common::Logger *logger;

ClientConnection::ClientConnection(unsigned int port,
                                   const std::string &ip_addr)
    : _port(port), _client_socket(_port) {
  if (_client_socket.create_connection(ip_addr))
    _is_successful_connection = true;
}

void ClientConnection::start_listening_to_server() {
  int kq = kqueue();
  struct kevent change;
  EV_SET(&change, _client_socket.get_conn_fd(), EVFILT_READ, EV_ADD | EV_ENABLE,
         0, 0, NULL);

  while (1) {
    //        logger->info("Enter data to send ");
    //        std::string str;
    //        std::getline(std::cin, str);
    //_client_socket.send_data_to_fd(str, _client_socket.get_conn_fd());
    struct kevent event;
    int newEvent = kevent(kq, &change, 1, &event, 1, NULL);
    if (newEvent > 0) {
      if (event.filter == EVFILT_READ) {
        logger->info("New Data : ", _client_socket.read_data_from_fd(
                                        _client_socket.get_conn_fd()));
      }
    } else if (newEvent == 0) // timeout
    {
      logger->info("time out");
    }
  }
}
