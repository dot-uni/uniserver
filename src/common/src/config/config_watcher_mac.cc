#include "project/common/config/config_watcher.h"


namespace {
    constexpr uint32_t kWatchFlags =
        NOTE_WRITE | NOTE_EXTEND | NOTE_DELETE | NOTE_RENAME | NOTE_REVOKE;
} // namespace


namespace uni::common::config {

struct ConfigWatcher::OSContext
{
    int kq_ = -1;
};


ConfigWatcher::ConfigWatcher(std::filesystem::path&& path, std::function<void(const std::filesystem::path&)> call) 
    : os_context_(std::make_unique<OSContext>()), call_(std::move(call)) {}


void ConfigWatcher::Init(std::filesystem::path path)
{
    if (!std::filesystem::exists(path)) {
        std::runtime_error("File not found: " + path.string());
    }
    
    int fd = open(path.c_str(), O_EVTONLY);
    if (fd == -1) {
        throw std::runtime_error("Unable to open file for watching: " + path.string());
    }

    int kq;
    if ((kq = kqueue()) == -1) {
        close(fd);
        throw std::runtime_error("kqueue failed to return a descriptor");
    }

    struct kevent kev[2];
    EV_SET(&kev[0], fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE, 0, nullptr);
    EV_SET(&kev[1], 1, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);

    int ret = kevent(kq, kev, 2, nullptr, 0, nullptr);
    if (ret == -1) {
        close(fd); 
        close(kq);
        throw std::runtime_error("kevent registration error");
    }


    Reset();

    std::lock_guard lock(mtx_);
    fd_ = fd;
    os_context_->kq_ = kq;
    path_ = std::move(path);
}


void ConfigWatcher::Start()
{
    if (fd_ == -1 || os_context_->kq_ == -1) {
        throw std::runtime_error("File sockets are invalid. The ConfigWatcher::Init() method may not have been called");
    }

    Stop();

    std::lock_guard lock(mtx_);
    started_.store(true);
    worker_ = std::thread(&ConfigWatcher::WatchLoop, this);
}


void ConfigWatcher::Stop() noexcept
{
    if (!started_.exchange(false)) {
        return;
    }

    struct kevent kev;
    EV_SET(&kev, 1, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
    kevent(os_context_->kq_, &kev, 1, nullptr, 0, nullptr);

    std::thread t;
    {
        std::lock_guard lock(mtx_);
        t = std::move(worker_);
    }
    if (t.joinable()) {
        t.join();
    }
}


void ConfigWatcher::Reset() noexcept
{

    Stop();

    std::lock_guard lock(mtx_);
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
    if (os_context_->kq_ != -1) {
        close(os_context_->kq_);
        os_context_->kq_ = -1;
    }
}


ConfigWatcher::~ConfigWatcher()
{
    Reset();
}


bool ConfigWatcher::Rewatch()
{
    std::filesystem::path path;
    {
        std::lock_guard lock(mtx_);
        path = path_;
    }

    int new_fd = -1;
    for (int i = 0; i < 50 && started_.load(); ++i) {
        new_fd = open(path.c_str(), O_EVTONLY);
        if (new_fd != -1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (new_fd == -1) {
        return false;
    }

    struct kevent kev;
    EV_SET(&kev, new_fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, ::kWatchFlags, 0, nullptr);
    if (kevent(os_context_->kq_, &kev, 1, nullptr, 0, nullptr) == -1) {
        close(new_fd);
        return false;
    }

    int old_fd;
    {
        std::lock_guard lock(mtx_);
        old_fd = fd_;
        fd_ = new_fd;
    }
    if (old_fd != -1) {
        close(old_fd);
    }
    return true;
}


void ConfigWatcher::WatchLoop()
{   
    try {
        while(true) {
            struct kevent triggered;
            const int n = kevent(os_context_->kq_, nullptr, 0, &triggered, 1, nullptr);


            if (n == -1) {
                sys_error("Config watcher kevent() failed: {}", std::strerror(errno));
                return;
            }

            if (n == 0) {
                sys_info("Config watcher kevent() returned no events");
                continue;
            }

            if (triggered.flags & EV_ERROR) {
                sys_error(
                    "Config watcher received EV_ERROR: filter={}, ident={}, error={}", 
                    triggered.filter, triggered.ident, std::strerror(static_cast<int>(triggered.data))
                );
                return;
            }


            switch (triggered.filter) {
                case EVFILT_VNODE: {
                    const uint32_t f = triggered.fflags;

                    if (f & (NOTE_DELETE | NOTE_RENAME | NOTE_REVOKE)) {
                        sys_info("Config file was replaced or moved, re-opening by path");
                        if (!Rewatch()) {
                            sys_error("Config watcher failed to re-open config file");
                            return;
                        }
                    } else if (!(f & (NOTE_WRITE | NOTE_EXTEND))) {
                        break;
                    }

                    std::filesystem::path path;
                    {
                        std::lock_guard lock(mtx_);
                        path = path_;
                    }

                    try {
                        call_(path_);
                        sys_info("Config watcher callback completed: {}", path_.native());
                    } catch (const std::exception& error) {
                        sys_error("Config watcher callback failed for {}: {}", path_.native(), error.what());
                        throw;
                    } catch (...) {
                        sys_error("Config watcher callback failed for {}: unknown exception", path_.native());
                        throw;
                    }
                    break;
                }
                case EVFILT_USER: {
                    return;
                }
                default: {
                    break;
                }
            }
        }
    } catch (const std::exception& error) {
        sys_error("Config watcher thread failed: {}", error.what());
    } catch (...) {
        sys_error("Config watcher thread failed with an unknown exception");
    }
}

} // namespace uni::common::config