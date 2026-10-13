#ifndef ROUTING_MANAGER_INCLUDED
#define ROUTING_MANAGER_INCLUDED


#include <optional>

#include "project/common/status/retcode.h" 
#include "project/routing/router.h"
#include "project/common/manager_base.h"


namespace uni::routing::manager {

class RoutingManager final : private common::manager::ManagerBase<RouterBase>
{
    using Base = common::manager::ManagerBase<RouterBase>;
public:
    template <
        typename HashKey = cryp::SipHashKey,
        typename Hash = cryp::SipHash
    > static void Init();

    template <
        typename HashKey = cryp::SipHashKey,
        typename Hash = cryp::SipHash
    > static void Init(const HashKey& key);

    static void Reset() noexcept;

    static bool Get(std::string_view path, Handler h);
    static bool Post(std::string_view path, Handler h);
    static bool Put(std::string_view path, Handler h);
    static bool Del(std::string_view path, Handler h);
    static bool Patch(std::string_view path, Handler h);
    
    static std::optional<server::http::Response> Route(const server::http::Request& req) noexcept;
};


template <typename HashKey, typename Hash> 
void RoutingManager::Init()
{
    if (Base::Initialized()) {
        LOG_WARN("Router already initialized, replacing existing instance");
    }
    LOG_DEBUG("Initializing router with default hash key");
    Base::Install(std::make_unique<Router<HashKey, Hash>>());
}


template <typename HashKey, typename Hash> 
void RoutingManager::Init(const HashKey& key)
{
    if (Base::Initialized()) {
        LOG_WARN("Router already initialized, replacing existing instance");
    }
    LOG_DEBUG("Initializing router with provided hash key");
    Base::Install(std::make_unique<Router<HashKey, Hash>>(key));
}

} // namespace uni::routing::manager

#endif 