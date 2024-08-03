//
//  ServerConnections.hpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 24/07/24.
//

#ifndef ServerConnections_hpp
#define ServerConnections_hpp

#include "SocketConnection.hpp"
#include <unordered_map>
#include <thread>

class ServerConnections
{
private:
    unsigned int _port;
    SocketConnection _server_socket;
    std::unordered_map<int, std::thread> _client_connections;
public:
    ServerConnections(unsigned int port);
    void StartListeningOnClient();
};

#endif /* ServerConnections_hpp */
