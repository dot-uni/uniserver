#ifndef LOGGING_CONFIG_WATCHER_INCLUDED
#define LOGGING_CONFIG_WATCHER_INCLUDED

#include <fcntl.h>
#include <fstream>
#include <filesystem>
#include <string_view>
#include <thread>
#include <functional>

#ifdef __APPLE__
#include <sys/event.h>
#endif

#include "project/logging/logging_manager.h"


namespace uni::logging::config {

std::unique_ptr<ISink> CreateSink(const YAML::Node& sink);

std::optional<LogConfig> ParseLogConfig(std::filesystem::path path);
bool ExecuteLogConfig(LogConfig&& config);


class LogConfigWatcher
{
private:
    class LogConfigWatcherImpl;
    std::unique_ptr<LogConfigWatcherImpl> impl_;
public:
    LogConfigWatcher(std::filesystem::path path);
    virtual ~LogConfigWatcher();

    LogConfigWatcher(const LogConfigWatcher&) = delete;
    LogConfigWatcher(LogConfigWatcher&&) = default;

    LogConfigWatcher& operator=(const LogConfigWatcher&) = delete;
    LogConfigWatcher& operator=(LogConfigWatcher&&) = default;

    void Start() override;
    void Stop() override;
};


/*




template <typename Parser, typename ConfigType>
class ConfigWatcher 

*/

} // namespace uni::logging::config

#endif