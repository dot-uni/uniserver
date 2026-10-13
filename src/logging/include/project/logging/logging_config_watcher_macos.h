#ifndef LOGGING_CONFIG_WATCHER_IMPL_MAC_INCLUDED
#define LOGGING_CONFIG_WATCHER_IMPL_MAC_INCLUDED

#include <unistd.h>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <optional>
#include <yaml-cpp/yaml.h>

#include "project/logging/logging_config_watcher.h"
#include "project/common/syslog.h"


namespace uni::logging::config {

class LogConfigWatcher::LogConfigWatcherImpl final
{
public:
    LogConfigWatcherImpl(std::filesystem::path path);
    ~LogConfigWatcherImpl();

    LogConfigWatcherImpl(const LogConfigWatcherImpl&) = delete;
    LogConfigWatcherImpl(LogConfigWatcherImpl&&) = delete;

    LogConfigWatcherImpl& operator=(const LogConfigWatcherImpl&) = delete;
    LogConfigWatcherImpl& operator=(LogConfigWatcherImpl&&) = delete;
public:
    void Start();
    void Stop();
private:
    void Run();
private:
    int fd_ = -1;
    int kq_ = -1;
    std::thread worker_;
    std::atomic_bool started_;
    std::mutex mtx_;
};

} // namespace uni::logging::config

#endif
