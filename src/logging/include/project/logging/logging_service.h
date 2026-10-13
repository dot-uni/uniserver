#ifndef LOGGING_SERVICE_INCLUDED
#define LOGGING_SERVICE_INCLUDED


#include <filesystem>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <source_location>
#include <vector>
#include <string_view>

#include "project/logging/logging_logger.h"
#include "project/logging/logging_manager.h"
#include "project/common/config/config_handler.h"
#include "project/common/syslog.h"


namespace uni::logging::manager {

std::unique_ptr<ISink> CreateSink(const YAML::Node& sink);
std::optional<LogConfig> ParseLogConfig(std::filesystem::path path);


class LoggingService final
{
    inline static std::jthread update_cycle_;
    inline static std::mutex mtx_ls_;
    inline static std::unique_ptr<LoggingManager> lm_ = nullptr;
    inline static std::unique_ptr<common::config::ConfigHandler<LogConfig>> chandler_ = nullptr;
public:
    ~LoggingService() = default;

    static bool Init(std::filesystem::path path);

    static bool Start();
    static bool Start(std::filesystem::path path);
    static void Stop();

    static std::optional<std::reference_wrapper<Logger>> Get() noexcept;
    static status::log GetLevel() noexcept;
private:
    static void LoggerConfigUpdate(std::stop_token stop_tocken);
};

} // namespace uni::logging::manager

#endif 
