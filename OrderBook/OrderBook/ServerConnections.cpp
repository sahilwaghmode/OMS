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

#include "Msg.hpp"

ServerConnections::ServerConnections(unsigned int port):_port(port),_server_socket(port)
{
    _server_socket.bind();
}

void ServerConnections::StartListeningOnClient()
{
    while(true)
    {
        int newSocket = 0;
        std::cout << "started listening on the socket " << std::endl;
        while(newSocket == 0)
        {
            newSocket = _server_socket.start_listening();
        }
        
        auto func = [&](int _socket)->void
        {
            
//            CSVReader reader("/Users/sahilwaghmode/CppDesigns/OMS/OrderBook/OrderBookDataSet/AMZN_message.csv");
//            
//            auto start_time = std::chrono::high_resolution_clock::now();
//            std::vector<std::vector<std::string>> data = reader.readCSV();
//            auto end_time = std::chrono::high_resolution_clock::now();
//            
//            auto readingTime = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
//            
//        std::cout << "File read in " << readingTime << " ms\n";
////            
//            std::string newLine;
//            for (const auto& line : data)
//            {
//                newLine="";
//                for (const auto& word : line)
//                {
//                    newLine += word + " ";
//                }
//                
//                newLine += "\n";
//                send(_socket, newLine.c_str(), newLine.length(), 0);
//            }
            
            _server_socket.listen_on_socket();
           
            
        };
        std::cout << "connection received "<< std::endl;
        auto newthread = std::thread(func, newSocket);
        _client_connections.insert({newSocket, std::move(newthread)});
        
    }
}
