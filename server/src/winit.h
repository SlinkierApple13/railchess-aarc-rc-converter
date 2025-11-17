#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"

namespace winit {
    
crow::App<crow::CORSHandler>* app();
void run(crow::App<crow::CORSHandler>* app);

}
