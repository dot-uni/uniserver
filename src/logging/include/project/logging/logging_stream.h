#ifndef LOGGING_STREAM_INCLUDED
#define LOGGING_STREAM_INCLUDED

#include <source_location>
#include <vector>
#include <string_view>
#include <string>

#include "project/common/status/log.h"
#include "project/common/status/retcode.h"
#include "project/common/status/http.h"
#include "project/logging/logging_logger.h"
#include "project/logging/logging_service.h"

namespace uni::logging::manager {

bool ShouldLog(status::log level) noexcept;

class LogStream final
{
public:
    // status::log 
    LogStream(
        const Logger& logger, 
        status::log level, 
        std::source_location loc,
        std::string_view msg, 
        std::vector<LogField>&& details={}
    );
    LogStream(
        const Logger& logger, 
        status::log level, 
        std::source_location loc,
        std::vector<LogField>&& details
    );

    // status::retCode 
    LogStream(
        const Logger& logger, 
        status::retCode code, 
        std::source_location loc,
        std::string_view msg, 
        std::vector<LogField>&& details={}
    );
    LogStream(
        const Logger& logger, 
        status::retCode code, 
        std::source_location loc,
        std::vector<LogField>&& details={}
    );

    // status::http 
    LogStream(
        const Logger& logger, 
        status::http status, 
        std::source_location loc,
        std::string_view msg, 
        std::vector<LogField>&& details={}
    );
    LogStream(
        const Logger& logger, 
        status::http status, 
        std::source_location loc,
        std::vector<LogField>&& details={}
    );

    ~LogStream();
    template <typename T> LogStream& operator<<(const T& v);
private:
    Logger logger_;
    LogInfo linfo_;
    std::vector<std::string> buffer_;
};


template <typename T> 
LogStream& LogStream::operator<<(const T& v)
{
    buffer_.push_back(uni::common::to_string(v));
    return *this;
}

} // namespace uni::logging::manager

#endif 