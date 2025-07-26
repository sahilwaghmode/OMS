//
//  main.cpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 21/07/24.
//

#include "CSVReader.hpp"
#include "Logger.hpp"
#include "ServerConnections.hpp"
#include "SocketConnection.hpp"
#include <iostream>
#include <vector>

Common::Logger *logger;

int main(int argc, const char *argv[]) {
  //    SocketConnection server(8080);
  //    server.start_listening();
  logger = new Common::Logger(
      "/Users/sahilwaghmode/CppDesigns/OMS/OrderBook/MktSimulatorlog.txt");

  logger->info("Starting the mkt simulator");

  ServerConnections server(8080);
  server.start_listening_for_client_connections();

  logger->info("Ending the mkt simulator");
}
