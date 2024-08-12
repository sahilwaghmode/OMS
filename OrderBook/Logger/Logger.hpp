//
//  Logger.hpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 10/08/24.
//

#ifndef Logger_hpp
#define Logger_hpp
#include <string>
#include <fstream>
#include <queue>
#include <thread>

namespace Common
{

enum class LogLevel : uint8_t
{
    INFO,
    ERROR,
    WARNING
};

class Logger {
private:
    const std::string _file_name;
    std::ofstream _file;
    std::queue<std::string> _queue;
    std::thread * _logger_thread;
    std::mutex _mtx;
    std::condition_variable cv;
    bool _running = true;
    
    void process_queue();
    
    template <typename T>
        void logHelper(std::ostringstream& oss, const T& message) {
            oss << message;
        }
    
    template <typename T, typename... Args>
        void logHelper(std::ostringstream& oss, const T& first, const Args&... args) {
            oss << first << " ";
            logHelper(oss, args...);
        }
    
public:
    explicit Logger(const std::string& file_name);
    ~Logger();
    
    template <typename T, typename... Args>
        void log(const T& first, const Args&... args) {
            std::ostringstream oss;
            logHelper(oss, first, args...);
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _queue.push(oss.str());
            }
            cv.notify_all();
        }
    
    //To do :: make std::string why template
    template <typename T>
        void log(const T& message) {
            std::ostringstream oss;
            oss << message;
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _queue.push(oss.str());
            }
            cv.notify_all();
        }
    
    template <typename... Args>
        void log(LogLevel level, const Args&... args) {
            static std::string log_level;
            switch (level) {
                case LogLevel::INFO:
                    log_level = "INFO";
                    break;
                case LogLevel::ERROR:
                    log_level = "ERROR";
                    break;
                case LogLevel::WARNING:
                    log_level = "WARNING";
                    break;
                default:
                    log_level = "INFO";
            }
            std::ostringstream oss;
            logHelper(oss, log_level, args...);
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _queue.push(oss.str());
            }
            cv.notify_all();
        }
    
    template <typename... Args>
    void info(const Args&... args) {
        log(LogLevel::INFO, args...);
    }
    
    template <typename... Args>
    void error(const Args&... args) {
        log(LogLevel::ERROR, args...);
    }
    
    template <typename... Args>
    void warning(const Args&... args) {
        log(LogLevel::WARNING, args...);
    }
    
};
}
#endif /* Logger_hpp */
