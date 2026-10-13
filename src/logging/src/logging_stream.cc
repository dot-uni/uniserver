#include "project/logging/logging_stream.h"


namespace uni::logging::manager {

bool ShouldLog(status::log level) noexcept
{
    common::status::log set_level = LoggingService::GetLevel();

    if (!static_cast<int>(set_level) || set_level > level) {
        return false;
    }
    return true;
}


/** logrr::LogStream 
 */

LogStream::LogStream(
    const Logger& logger, 
    status::log level, 
    std::source_location loc,
    std::string_view msg, 
    std::vector<LogField>&& details
) : logger_(logger), linfo_(level, msg, std::move(details), std::move(loc)) {}


LogStream::LogStream(
    const Logger& logger, 
    status::log level, 
    std::source_location loc,
    std::vector<LogField>&& details
) : LogStream(logger, level, std::move(loc), "", std::move(details)) {}


LogStream::LogStream(
    const Logger& logger, 
    status::retCode code, 
    std::source_location loc,
    std::string_view msg, 
    std::vector<LogField>&& details
) : logger_(logger)
{
    status::http s = status::to_http_status(code);
    std::vector<LogField> details_base = {
        field("retCode", code),
        field("retMesg", status::retMesg(code)),
        field("status", s),
        field("obsolete_reason", status::obsolete_reason(s))
    };
    details.insert(details.end(), details_base.begin(), details_base.end());

    linfo_ = LogInfo(to_log(code), msg, std::move(details), std::move(loc));
}


LogStream::LogStream(
    const Logger& logger, 
    status::retCode code, 
    std::source_location loc,
    std::vector<LogField>&& details
) : LogStream(logger, code, std::move(loc), "", std::move(details)) {}


LogStream::LogStream(
    const Logger& logger, 
    status::http status, 
    std::source_location loc,
    std::string_view msg, 
    std::vector<LogField>&& details
) : logger_(logger)
{
    std::vector<LogField> details_base = {
        field("status", status),
        field("obsolete_reason", status::obsolete_reason(status))
    };
    details.insert(details.end(), details_base.begin(), details_base.end());

    linfo_ = LogInfo(to_log(status), msg, std::move(details), std::move(loc));
}


LogStream::LogStream(
    const Logger& logger, 
    status::http status, 
    std::source_location loc,
    std::vector<LogField>&& details
) : LogStream(logger, status, std::move(loc), "", std::move(details)) {}


LogStream::~LogStream()
{
    try {
        if (!buffer_.empty()) {
            fmt::dynamic_format_arg_store<fmt::format_context> store;
            for (const auto& arg : buffer_) {
                store.push_back(arg);
            }
            linfo_.info = fmt::vformat(linfo_.info, store);
        }
        logger_.log(std::move(linfo_));
    } catch(const std::exception& msg) {
        common::sys_error(msg.what());
    } catch(...) {
        common::sys_error("unknown error in LogStream::~LogStream");
    }
} 

} // namespace uni::logging::manager