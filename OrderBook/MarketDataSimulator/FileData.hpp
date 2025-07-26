//
//  FileData.hpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 13/08/24.
//

#ifndef FileData_hpp
#define FileData_hpp
#include <string>
#include <unordered_map>
#include <vector>

class FileData {
private:
  std::unordered_map<std::string, std::vector<std::vector<std::string>>>
      _map_symbol_data;

  FileData();
  ~FileData();
  void add_symbol_data(const std::string &symbol,
                       const std::vector<std::vector<std::string>> &data);

public:
  FileData(const FileData &) = delete;
  static FileData &get_instance();

  const std::vector<std::vector<std::string>> &
  get_symbol_data(const std::string &symbol) const;
  void add_file_data(const std::string &file_name, const std::string &symbol);
};

#endif /* FileData_hpp */
