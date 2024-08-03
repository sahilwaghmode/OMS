//
//  main.cpp
//  OrderBook
//
//  Created by Sahil Waghmode on 18/07/24.
//

#include <iostream>

#include "CSVReader.hpp"
#include <vector>
#include "SocketConnection.hpp"

int main(int argc, const char * argv[])
{
//    CSVReader reader("/Users/sahilwaghmode/CppDesigns/OMS/OrderBook/OrderBookDataSet/AMZN_message.csv");
//    
//    auto start_time = std::chrono::high_resolution_clock::now();
//    std::vector<std::vector<std::string>> data = reader.readCSV();
//    auto end_time = std::chrono::high_resolution_clock::now();
//    
//    auto readingTime = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
//    
//    std::cout << "File read in " << readingTime << " ms\n";
    
    SocketConnection client(8080);
    client.create_connection("192.168.0.163");
    
    return 0;
}
