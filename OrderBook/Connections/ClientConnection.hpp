//
//  Connection.hpp
//  OrderBook
//
//  Created by Sahil Waghmode on 03/08/24.
//

#ifndef Connection_hpp
#define Connection_hpp

#include <stdio.h>
#include "SocketConnection.hpp"

class ClientConnection {
private:
    unsigned int _port;
    SocketConnection _client_socket;
    bool _is_successful_connection = false;
    
public:
    ClientConnection(unsigned int port, const std::string& ip_addr);
    
    void start_listening_to_server();
    bool is_valid_connection(){return _is_successful_connection;}
};

#endif /* Connection_hpp */
