//
//  main.cpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 21/07/24.
//

#include "CSVReader.hpp"
#include "ServerConnections.hpp"
#include "SimpleLogger.h"
#include "SocketConnection.hpp"
#include <iostream>
#include <vector>

Common::SimpleLogger *logger;

int main(int argc, const char *argv[]) {
  //    SocketConnection server(8080);
  //    server.start_listening();
  logger = new Common::SimpleLogger(
      "/Users/sahilwaghmode/CppDesigns/OMS/OrderBook/MktSimulatorlog.txt");

  logger->info("Starting the mkt simulator");

  ServerConnections server(8080);
  server.start_listening_for_client_connections();

  logger->info("Ending the mkt simulator");
}
