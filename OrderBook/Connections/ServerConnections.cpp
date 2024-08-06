//
//  ServerConnections.cpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 24/07/24.
//
#include <iostream>
#include "ServerConnections.hpp"
#include "SocketConnection.hpp"
#include "CSVReader.hpp"

//kqueue
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
//kqueue

#include "Msg.hpp"

ServerConnections::ServerConnections(unsigned int port):_port(port),_server_socket(port)
{
    _server_socket.bind();
}

void ServerConnections::start_listening_for_client_connections()
{
    while(true)
    {
        int newSocket = 0;
        std::cout << "started listening on the socket " << std::endl;
        while(newSocket == 0)
        {
            newSocket = _server_socket.start_listening();
        }
        
        std::cout << "connection received "<< std::endl;
        auto newthread = std::thread(start_listening_to_client, newSocket);
        _client_connections.insert({newSocket, std::move(newthread)});
    }
}

void ServerConnections::start_listening_to_client(int client_fd)
{
    SocketConnection::send_data_to_fd("Hello From Server", client_fd);
    int kq = kqueue();
    struct kevent change;
    EV_SET(&change, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, sizeof(Msg::MsgHeader), NULL);
    
    while (1)
    {
        struct kevent event;
        int newEvent = kevent(kq, &change, 1, &event, 1, NULL);
        if (newEvent > 0)
        {
            if (event.filter == EVFILT_READ)
            {
                std::cout << "New Data : " << SocketConnection::read_data_from_fd(client_fd) << std::endl;
                SocketConnection::send_data_to_fd("ACK", client_fd);
            }
        }
    }
}
