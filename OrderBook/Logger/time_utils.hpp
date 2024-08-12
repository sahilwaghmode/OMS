//
//  time_utils.hpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 10/08/24.
//

#ifndef time_utils_hpp
#define time_utils_hpp

#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace Common
{
    typedef int64_t Nanos;

    constexpr Nanos NANOS_TO_MICROS = 1000;
    constexpr Nanos MICROS_TO_MILLIS = 1000;
    constexpr Nanos MILLIS_TO_SECS = 1000;
    constexpr Nanos NANOS_TO_MILLIS = NANOS_TO_MICROS * MICROS_TO_MILLIS;
    constexpr Nanos NANOS_TO_SECS = NANOS_TO_MILLIS * MILLIS_TO_SECS;

    inline auto getCurrentNanos() noexcept
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    inline auto& getCurrentDateTimeStr(std::string* time_str) 
    {
        const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        time_str->assign(ctime(&time));
//        if(!time_str->empty())
//          time_str->at(time_str->length()-1) = '\0';
        return *time_str;
    }

    inline const auto& getCurrentTimeStr()
    {
        static std::string time_str;
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        // Convert to local time
        std::tm* local_time = std::localtime(&now_time);
        std::cout << std::put_time(local_time,"%H:%M:%S") << std::endl;
        return std::put_time(local_time,"%H:%M:%S");
    }
}

#endif /* time_utils_hpp */
