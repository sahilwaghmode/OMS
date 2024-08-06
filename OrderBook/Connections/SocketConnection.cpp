//
//  SocketConnection.cpp
//  OrderBook
//
//  Created by Sahil Waghmode on 21/07/24.
//

#include "SocketConnection.hpp"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

#include "Msg.hpp"

SocketConnection::SocketConnection(int port):_port(port)
{
    if ((_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cout << "socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
            perror("setsockopt");
            exit(EXIT_FAILURE);
    }
    
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(port);
    
}

SocketConnection::~SocketConnection()
{
    close_connection();
    close_server();
}

bool SocketConnection::bind()
{
    if (::bind(_socket_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
    {
        std::cout << "Bind failed" << std::endl;
        return 0;
    }
    return true;
}

int SocketConnection::start_listening()
{
    if (listen(_socket_fd, 10) < 0)
    {
        std::cout << "listen failed" << std::endl;
        return 0;
    }
    
    int addrlen = sizeof(_address);
    if ((_socket = accept(_socket_fd, (struct sockaddr *)&_address, (socklen_t*)&addrlen)) < 0)
    {
        perror("accept");
        return 0;
    }
    
    return _socket;
}

bool SocketConnection::create_connection(const std::string& ip_addr)
{
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip_addr.c_str(), &_address.sin_addr) <= 0)
    {
        std::cout << "Invalid address/ Address not supported" << std::endl;
        return false;
    }
    
    if (connect(_socket_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
    {
        std::cout << "Connection Failed" << std::endl;
        return false;
    }
    return true;
}

bool SocketConnection::send_data(const std::string& msg)
{
    int length = 0;
    auto msgToSend = Msg::create_txt_msg(msg, length);
    send(_socket, msgToSend.get(), length, 0);
    return true;
}

std::string SocketConnection::read_data()
{
    auto header_length = 0;
    char header[sizeof(Msg::MsgHeader)] = {0};
    read(_socket, header, header_length);
    
    Msg::MsgHeader* msgHeader = (Msg::MsgHeader*)header;
    char* msg = new char(msgHeader->length);
    read(_socket, msg, msgHeader->length);
    return {msg};
}

void SocketConnection::close_connection()
{
    close(_socket);
}

void SocketConnection::close_server()
{
    close(_socket_fd);
}

std::string SocketConnection::read_data_from_fd(int fd)
{
    auto header_length = sizeof(Msg::MsgHeader);
    char header[sizeof(Msg::MsgHeader)] = {0};
    read(fd, header, header_length);
    
    Msg::MsgHeader* msgHeader = (Msg::MsgHeader*)header;
    char* msg = new char(msgHeader->length);
    read(fd, msg, msgHeader->length);
    return {msg};
}

void SocketConnection::send_data_to_fd(std::string msg, int fd)
{
    //send(fd, msg.c_str(), msg.length(), 0);
    int length = 0;
    auto msgToSend = Msg::create_txt_msg(msg, length);
    send(fd, msgToSend.get(), length, 0);
}

std::string SocketConnection::read_data_dummy(int fd)
{
//    char buff[100];
//    read(fd, buff, 100);
//    return {buff};
    
    auto header_length = 0;
    char header[sizeof(Msg::MsgHeader)] = {0};
    read(fd, header, header_length);
    
    Msg::MsgHeader* msgHeader = (Msg::MsgHeader*)header;
    char* msg = new char(msgHeader->length);
    read(fd, msg, msgHeader->length);
    return {msg};
}
