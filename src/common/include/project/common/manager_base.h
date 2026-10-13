#ifndef COMMON_MANAGER_BASE_INCLUDED
#define COMMON_MANAGER_BASE_INCLUDED


#include <memory>
#include <optional>
#include <stdexcept>
#include <functional>


namespace uni::common::manager {

template <typename T>
class ManagerBase 
{
    inline static std::unique_ptr<T> instance_ = nullptr;
public:
    virtual ~ManagerBase() = default;

    ManagerBase(const ManagerBase&) = delete;
    ManagerBase& operator=(const ManagerBase&) = delete;
protected:
    ManagerBase() = default;

    static bool Install(std::unique_ptr<T> instance) noexcept {
        if (instance_ || !instance) {
            return false;
        }
        instance_ = std::move(instance);
        return true;
    }

    static bool Initialized() noexcept { return instance_ != nullptr; }

    static void Destroy() noexcept { instance_.reset(); }

    static T* Instance() noexcept { return instance_.get(); }

    static T& Require() {
        if (!instance_) {
            throw std::logic_error("Manager is not initialized");
        }
        return *instance_;
    }

    static std::optional<std::reference_wrapper<T>> TryGet() noexcept {
        if (!instance_) {
            return std::nullopt;
        }
        return std::ref(*instance_);
    }
};

} // namespace uni::common::manager

#endif