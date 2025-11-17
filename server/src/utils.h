#pragma once

#include <iostream>

#include "crow.h"
#include "crow/middlewares/cors.h"

// log with time displayed in UTC+0
#define LOG_INFO ::utils::log_info()
#define LOG_ERR ::utils::log_err()

namespace utils {

std::ostream& log_info();
std::ostream& log_err();

}