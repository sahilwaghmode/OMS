//
//  CSVReader.cpp
//  OrderBook
//
//  Created by Sahil Waghmode on 18/07/24.
//

#include "CSVReader.hpp"
#include <vector>
#include <sstream>

CSVReader::CSVReader(const std::string& filename) : file(filename) {}

std::vector<std::vector<std::string>> CSVReader::readCSV()
{
    std::vector<std::vector<std::string>> data;
    std::string line;

    while (std::getline(file, line)) 
    {
        std::vector<std::string> row;
        std::stringstream lineStream(line);
        std::string cell;

        while (std::getline(lineStream, cell, ',')) 
        {
            row.push_back(cell);
        }

        data.push_back(row);
    }

    return data;
}
