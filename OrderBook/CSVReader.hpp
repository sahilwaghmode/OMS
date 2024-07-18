//
//  CSVReader.hpp
//  OrderBook
//
//  Created by Sahil Waghmode on 18/07/24.
//

#ifndef CSVReader_hpp
#define CSVReader_hpp

#include <string>
#include <fstream>

class CSVReader
{
public:
    CSVReader(const std::string& filename);

    std::vector<std::vector<std::string>> readCSV();

private:
    std::ifstream file;
};


#endif /* CSVReader_hpp */
