#include <iomanip>
#include <chrono>

#include "utils.h"

namespace utils {

std::ostream& log_info() {
    auto _t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    return std::cerr << "(" << std::put_time(std::gmtime(&_t), "%Y-%m-%d %H:%M:%S") << ") [INFO    ] ";
}

std::ostream& log_err() {
    auto _t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    return std::cerr << "(" << std::put_time(std::gmtime(&_t), "%Y-%m-%d %H:%M:%S") << ") [ERROR   ] ";
}

}