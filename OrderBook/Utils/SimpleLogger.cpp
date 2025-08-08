
#include "SimpleLogger.h"
#include "time_utils.hpp"
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
namespace Common {
SimpleLogger::SimpleLogger(const std::string &file_name)
    : _file_name(file_name) {
  _file.open(_file_name, std::ios::out | std::ios::app);
  _logger_thread = new std::thread(&SimpleLogger::process_queue, this);
}

SimpleLogger::~SimpleLogger() {
  {
    std::unique_lock<std::mutex> lock(_mtx);
    _running = false;
  }
  cv.notify_all();
  _logger_thread->join();
  delete _logger_thread;
  _file.close();
}

void SimpleLogger::process_queue() {
  if (!_file.is_open()) {
    std::cerr << "SimpleLogger file is not open" << std::endl;
    return;
  }
  while (true) {
    std::unique_lock<std::mutex> lock(_mtx);
    cv.wait(lock, [this] { return !_queue.empty() || !_running; });

    std::string msg = _queue.front();
    _queue.pop();
    _file << getCurrentTime() << " " << msg << std::endl;

    if (!_running && _queue.empty()) {
      break;
    }
  }
}
} // namespace Common