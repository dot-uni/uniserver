#include "project/server/http_server_manager.h"


namespace uni::server::manager::http {

HttpServerManager::HttpServerManager() : server_(std::make_unique<http::HttpServer>()) {}

HttpServerManager::HttpServerManager(
    const std::string& cert, 
    const std::string& key
) : server_(std::make_unique<https::HttpsServer>(cert, key)) {}




} // namespace uni::server::manager::http