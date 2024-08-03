//
//  main.cpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 21/07/24.
//

#include <iostream>
#include "SocketConnection.hpp"
#include "CSVReader.hpp"
#include <vector>
#include "../../OrderBook/ServerConnections.hpp"

int main(int argc, const char * argv[]) 
{
//    SocketConnection server(8080);
//    server.start_listening();
    
    ServerConnections server(8080);
    server.StartListeningOnClient();
    
    std::cout << "Done closing the server" << std::endl;
    
}
