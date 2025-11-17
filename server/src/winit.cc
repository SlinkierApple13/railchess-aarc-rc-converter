#include "utils.h"
#include "winit.h"

namespace winit {

crow::App<crow::CORSHandler>* app() {

    crow::App<crow::CORSHandler>* app = new crow::App<crow::CORSHandler>();

    CROW_ROUTE((*app), "/<path>").methods("OPTIONS"_method)([](const crow::request&, crow::response& res, const std::string&){
        res.code = 204;
        res.end();
    });

    return app;
}

void run(crow::App<crow::CORSHandler>* app) {
    auto& cors = app->get_middleware<crow::CORSHandler>();

    cors.global()
        .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method, "OPTIONS"_method)
        .headers("Content-Type, Authorization")
        .allow_credentials();

    LOG_INFO << "Starting HTTP server on port 3005..." << std::endl;
    app->port(3005)
        .multithreaded()
        .run();
}

}
