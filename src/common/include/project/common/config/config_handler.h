#ifndef COMMON_CONFIG_HANDLER_INCLUDED
#define COMMON_CONFIG_HANDLER_INCLUDED


#include <condition_variable>
#include <optional>

#include "project/common/syslog.h"
#include "project/common/config/config_watcher.h"
#include "project/common/config/config_handler_base.h"


namespace uni::common::config {

template <typename ConfigType>
class ConfigHandler : private ConfigWatcher
{
    std::mutex mtx_ch_;
    std::condition_variable cv_;
    std::atomic_bool config_changed_ = false;
    std::atomic_bool stopped_ = false;
    std::function<std::optional<ConfigType>(std::filesystem::path)> parser_;
    std::optional<ConfigType> config_;
public:
    template <typename P, typename... Args>
    requires std::is_invocable_r_v<std::optional<ConfigType>, P&, const std::filesystem::path&, Args&...>
    ConfigHandler(P parser, std::filesystem::path path, Args... args) 
        : ConfigWatcher([this](std::filesystem::path path){ UpdateConfig(path); }, path)
        , parser_([
            prs = std::forward<P>(parser),
            ...args = std::move(args)
        ](const std::filesystem::path& p) mutable {
            return prs(p, args...);
        })
    {
        Start();
    }

    virtual ~ConfigHandler() {
        Stop();
        {
            std::lock_guard lock(mtx_ch_);
            stopped_.store(true);
        }
        cv_.notify_all();
    }

    void StopWaiting();
    std::optional<ConfigType> WaitForConfig();
    std::optional<ConfigType> GetConfig();
private:
    void UpdateConfig(std::filesystem::path path) noexcept;
};


template <typename ConfigType>
void ConfigHandler<ConfigType>::UpdateConfig(std::filesystem::path path) noexcept
{
    try {
        auto cnfg = parser_(path);
        if (!cnfg) return;
        {
            std::lock_guard lock(mtx_ch_);
            config_ = std::move(*cnfg);
            config_changed_.store(true);
        }
        cv_.notify_all();
    } catch(const std::exception& err) {
        sys_error("Error while trying to update config: {}", err.what());
    } catch(...) {
        sys_error("Unknown error while attempting to update the configuration");
    }
}


template <typename ConfigType>
void ConfigHandler<ConfigType>::StopWaiting()
{
    std::lock_guard lock(mtx_ch_);
    stopped_ = true;
    cv_.notify_all();
}


template <typename ConfigType>
std::optional<ConfigType> ConfigHandler<ConfigType>::WaitForConfig()
{
    std::unique_lock lock(mtx_ch_);
    cv_.wait(lock, [this] { return config_changed_ || stopped_; });

    if (stopped_ || !config_changed_) {
        stopped_.store(false);
        return std::nullopt;
    }

    config_changed_.store(false);
    return config_;
}


template <typename ConfigType>
std::optional<ConfigType> ConfigHandler<ConfigType>::GetConfig()
{
    std::lock_guard lock(mtx_ch_);
    
    if (stopped_) { 
        return std::nullopt;
    }

    auto config = parser_(path_);
    if (!config) {
        return std::nullopt;
    }

    return *config;
}

} // namespace uni::common::config

#endif