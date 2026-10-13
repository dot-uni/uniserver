#ifndef LOGGING_MANAGER_INCLUDED
#define LOGGING_MANAGER_INCLUDED


#include <filesystem>
#include <optional>
#include <memory>
#include <atomic>
#include <yaml-cpp/yaml.h>

#include "project/logging/logging_logger.h"
#include "project/common/status/log.h"
#include "project/common/syslog.h"


namespace uni::logging::manager {

/**
 * LoggingManager is a special class whose responsibility is to manage the logger
 */

class LoggingManager
{
    std::mutex mtx_;
    std::unique_ptr<Logger> logger_ = nullptr;
    std::atomic_bool paused_ = false;
public:
    LoggingManager() = default;
    LoggingManager(LogConfig config);
    virtual ~LoggingManager() = default;

    void Pause() noexcept;
    void Remain() noexcept;

    std::optional<std::reference_wrapper<Logger>> Get() noexcept;
    common::status::log GetLevel() noexcept;
    bool Paused() noexcept;
}; 

} // namespace uni::logging::manager

#endif