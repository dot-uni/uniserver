#ifndef COMMON_CONFIG_HANDLER_BASE_INCLUDED
#define COMMON_CONFIG_HANDLER_BASE_INCLUDED


#include <filesystem>

namespace uni::common::config {

template <typename ConfigType>
class ConfigHandlerBase
{
public:
    virtual ~ConfigHandlerBase() = default;
    virtual void Start(std::filesystem::path path) = 0;
    virtual void Stop() = 0;
    virtual ConfigType WaitForConfig() = 0;
};

} // namespace uni::common::config

#endif