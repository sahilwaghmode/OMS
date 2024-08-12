//
//  Logger.cpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 10/08/24.
//

#include "Logger.hpp"
#include <iostream>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include "time_utils.hpp"
namespace Common
{
Logger::Logger(const std::string& file_name) : _file_name(file_name)
{
    _file.open(_file_name, std::ios::out | std::ios::app);
    _logger_thread = new std::thread(&Logger::process_queue, this);
}

Logger::~Logger()
{
    {
        std::unique_lock<std::mutex> lock(_mtx);
        _running = false;
    }
    cv.notify_all();
    _logger_thread->join();
    delete _logger_thread;
    _file.close();
}

void Logger::process_queue()
{
    if (!_file.is_open())
    {
        std::cerr << "Logger file is not open" << std::endl;
        return;
    }
    while(true)
    {
        std::unique_lock<std::mutex> lock(_mtx);
        cv.wait(lock, [this] { return !_queue.empty() || !_running; });
        
        std::string msg = _queue.front();
        _queue.pop();
        _file << getCurrentTimeStr() << " " << msg << std::endl;
        
        if(!_running && _queue.empty())
        {
            break;
        }
    }
}
}
