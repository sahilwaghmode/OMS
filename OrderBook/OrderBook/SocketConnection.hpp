//
//  SocketConnection.hpp
//  OrderBook
//
//  Created by Sahil Waghmode on 21/07/24.
//

#ifndef SocketConnection_hpp
#define SocketConnection_hpp

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <functional>

class SocketConnection {
private:
    int _port = 0;
    int _socket = 0;
    int _socket_fd = 0;
    struct sockaddr_in _address;
    
    void close_server();
    
public:
    SocketConnection(int port);
    ~SocketConnection();
    int start_listening();
    bool create_connection(const std::string& ip_addr);
    bool bind();
    bool send_data(const std::string& msg);
    std::string read_data();
    void close_connection();
    
    int get_server_conn_fd() const {return _socket_fd;}
    void listen_on_socket();
};

#endif /* SocketConnection_hpp */
