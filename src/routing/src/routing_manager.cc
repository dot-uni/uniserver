#include "project/routing/routing_manager.h"


namespace uni::routing::manager {

namespace status = ::uni::common::status;
using ::uni::logging::field;


void RoutingManager::Reset() noexcept
{
    LOG_DEBUG("Resetting router instance");
    Base::Destroy();
}


bool RoutingManager::Get(std::string_view path, Handler h) 
{
    if (!Base::Initialized()) {
        LOG_ERROR("Router not initialized", {
            field("path", path)
        });
        throw std::runtime_error("Router not initialized");
    }

    LOG_TRACE("Registering handler", {
        field("path", path)
    });
    bool result = Base::Require().get(path, std::move(h));
    if (!result) {
        LOG_WARN("Failed to register handler", {
            field("path", path)
        });
    }

    return result;
}


bool RoutingManager::Post(std::string_view path, Handler h) 
{
    if (!Base::Initialized()) {
        LOG_ERROR("Router not initialized", {
            field("path", path)
        });
        throw std::runtime_error("Router not initialized");
    }

    LOG_TRACE("Registering handler", {
        field("path", path)
    });
    bool result = Base::Require().post(path, std::move(h));
    if (!result) {
        LOG_WARN("Failed to register handler", {
            field("path", path)
        });
    }

    return result;
}


bool RoutingManager::Put(std::string_view path, Handler h) 
{
    if (!Base::Initialized()) {
        LOG_ERROR("Router not initialized", {
            field("path", path)
        });
        throw std::runtime_error("Router not initialized");
    }

    LOG_TRACE("Registering handler", {
        field("path", path)
    });
    bool result = Base::Require().put(path, std::move(h));
    if (!result) {
        LOG_WARN("Failed to register handler", {
            field("path", path)
        });
    }

    return result;
}


bool RoutingManager::Del(std::string_view path, Handler h) 
{
    if (!Base::Initialized()) {
        LOG_ERROR("Router not initialized", {
            field("path", path)
        });
        throw std::runtime_error("Router not initialized");
    }

    LOG_TRACE("Registering handler", {
        field("path", path)
    });
    bool result = Base::Require().del(path, std::move(h));
    if (!result) {
        LOG_WARN("Failed to register handler", {
            field("path", path)
        });
    }

    return result;
}


bool RoutingManager::Patch(std::string_view path, Handler h) 
{
    if (!Base::Initialized()) {
        LOG_ERROR("Router not initialized", {
            field("path", path)
        });
        throw std::runtime_error("Router not initialized");
    }

    LOG_TRACE("Registering handler", {
        field("path", path)
    });
    bool result = Base::Require().patch(path, std::move(h));
    if (!result) {
        LOG_WARN("Failed to register handler", {
            field("path", path)
        });
    }

    return result;
}


std::optional<server::http::Response> RoutingManager::Route(const server::http::Request& req) noexcept
{
    if (!Base::Initialized()) {
        LOG_ERROR("Router not initialized, request dropped");
        return std::nullopt;
    }

    LOG_TRACE("Routing request");
    auto response = Base::Require().route(req);

    if (!response) {
        LOG_DEBUG("No matching route found");
        return server::http::makeResp(status::retCode::NotFound, req.id);
    }
    return *response;
}

} // namespace uni::routing::manager