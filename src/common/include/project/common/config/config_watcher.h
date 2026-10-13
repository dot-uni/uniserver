#ifndef COMMON_CONFIG_WATCHER_INCLUDED
#define COMMON_CONFIG_WATCHER_INCLUDED


#include <fcntl.h>
#include <fstream>
#include <string_view>
#include <thread>
#include <mutex>
#include <filesystem>
#include <functional>
#include <concepts>
#include <tuple>
#include <atomic>
#include <exception>
#include <memory>
#include <unistd.h>

#ifdef __APPLE__
#include <sys/event.h>
#endif

#include "project/common/syslog.h"
#include "project/common/tostring.h"


namespace uni::common::config {

class ConfigWatcher
{
    struct OSContext;
    friend class OSContext;

    int fd_ = -1;
    std::unique_ptr<OSContext> os_context_;
    std::mutex mtx_;
    std::function<void(const std::filesystem::path&)> call_;
    std::thread worker_;
    std::atomic_bool started_ = false;
protected:
    std::filesystem::path path_;
public:
    template <typename F, typename... Args>
    requires (std::invocable<std::decay_t<F>&, std::decay_t<Args>&...> || 
              std::invocable<std::decay_t<F>&, const std::filesystem::path&, std::decay_t<Args>&...>)
    ConfigWatcher(F&& f, std::filesystem::path path, Args&&... args) :
        ConfigWatcher(std::move(path), 
            std::function<void(const std::filesystem::path&)>([
                fn = std::forward<F>(f), 
                ...args = std::forward<Args>(args)
            ](const std::filesystem::path& p) mutable {
                using Fn = std::decay_t<F>;
                if constexpr (std::is_invocable_v<Fn&, const std::filesystem::path&, std::decay_t<Args>&...>) {
                    fn(p, args...);
                }
                else {
                    fn(args...);
                }
            })
        )
    {
        try {
            Init(std::move(path));
        } catch(...) {
            sys_crit("Object of type ConfigWatcher not created");
            throw;
        }
    }

    virtual ~ConfigWatcher();

    void Init(std::filesystem::path path);
    void Start();
    void Stop() noexcept;
private:
    ConfigWatcher(std::filesystem::path&& path, std::function<void(const std::filesystem::path&)> call);

    bool Rewatch();
    void WatchLoop();
    void Reset() noexcept;
};

} // uni::common::config

#endif 