#ifndef LOGGING_INCLUDED
#define LOGGING_INCLUDED

#include <source_location>

#include "project/common/status/log.h"
#include "project/logging/logging_stream.h"
#include "project/logging/logging_service.h"

#define LOG(level, ...)                                                                                                              \
    for (bool should_log = uni::logging::manager::ShouldLog(uni::common::status::to_log(level)); should_log; should_log = false)     \
        if (auto logger = uni::logging::manager::LoggingService::Get(); logger)                                                           \
            uni::logging::manager::LogStream(*logger, level, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)

#define LOG_INFO(...) LOG(uni::common::status::log::info, __VA_ARGS__)
#define LOG_ERROR(...) LOG(uni::common::status::log::error, __VA_ARGS__)
#define LOG_WARN(...) LOG(uni::common::status::log::warning, __VA_ARGS__)
#define LOG_CRIT(...) LOG(uni::common::status::log::critical, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(uni::common::status::log::debug, __VA_ARGS__)
#define LOG_TRACE(...) LOG(uni::common::status::log::trace, __VA_ARGS__)

#define LOG_USING_RETCODE(code, ...) LOG(code __VA_OPT__(,) __VA_ARGS__)
#define LOG_USING_STATUS(status, ...) LOG(status __VA_OPT__(,) __VA_ARGS__)


#endif 