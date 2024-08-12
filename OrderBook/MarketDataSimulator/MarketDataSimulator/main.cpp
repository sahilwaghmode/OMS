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
#include "ServerConnections.hpp"
#include "Logger.hpp"

Common::Logger * logger;

int main(int argc, const char * argv[]) 
{
//    SocketConnection server(8080);
//    server.start_listening();
    using namespace Common;
    logger = new Common::Logger("/Users/sahilwaghmode/CppDesigns/OMS/OrderBook/MktSimulatorlog.txt");
    
    logger->info("Starting the mkt simulator");
    
    ServerConnections server(8080);
    server.start_listening_for_client_connections();
    
    logger->info("Ending the mkt simulator");
    
}
