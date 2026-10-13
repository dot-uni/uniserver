#include "project/logging/logging_manager.h"


namespace uni::logging::manager {


LoggingManager::LoggingManager(LogConfig config)
{
    if (config.enabled) {
        logger_ = std::make_unique<Logger>(std::move(config));
    }
}


void LoggingManager::Pause() noexcept
{
    {
        std::lock_guard lock(mtx_);
        paused_ = true;
    }
    common::sys_info("A pause has been set in LoggingManager");
}


void LoggingManager::Remain() noexcept
{
    {
        std::lock_guard lock(mtx_);
        paused_ = false;
    }
    common::sys_info("The pause in LoggingManager has been removed");
}


std::optional<std::reference_wrapper<Logger>> LoggingManager::Get() noexcept
{
    if (!logger_ || paused_) {
        return std::nullopt;
    }
    return std::ref(*logger_);
}


common::status::log LoggingManager::GetLevel() noexcept
{
    if (!logger_) {
        return common::status::log::unknown;
    }
    return logger_->level();
}


bool LoggingManager::Paused() noexcept
{
    return paused_;
}

} // namespace uni::logging::manager