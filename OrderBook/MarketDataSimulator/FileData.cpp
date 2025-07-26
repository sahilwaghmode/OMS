//
//  FileData.cpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 13/08/24.
//

#include "FileData.hpp"
#include "CSVReader.hpp"
#include "Logger.hpp"

extern Common::Logger *logger;

FileData::FileData() {}

void FileData::add_symbol_data(
    const std::string &symbol,
    const std::vector<std::vector<std::string>> &data) {
  auto it = _map_symbol_data.find(symbol);
  if (it == _map_symbol_data.end()) {
    _map_symbol_data.insert({symbol, {data}});
  } else {
    std::move(data.begin(), data.end(), std::back_inserter(it->second));
  }
}

const std::vector<std::vector<std::string>> &
FileData::get_symbol_data(const std::string &symbol) const {
  auto it = _map_symbol_data.find(symbol);
  if (it == _map_symbol_data.end()) {
    logger->error("Symbol not found in the file data");
  }
  return it->second;
}

void FileData::add_file_data(const std::string &file_name,
                             const std::string &symbol) {
  CSVReader reader(file_name);
  auto start_time = std::chrono::high_resolution_clock::now();
  std::vector<std::vector<std::string>> data = reader.readCSV();
  auto end_time = std::chrono::high_resolution_clock::now();

  auto readingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                         end_time - start_time)
                         .count();

  logger->info("File read in " + std::to_string(readingTime) + " ms");

  add_symbol_data(symbol, data);
}
