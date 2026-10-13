#include "project/logging/logging_service.h"


namespace uni::logging::manager {


std::unique_ptr<ISink> CreateSink(const YAML::Node& sink)
{
    if (!sink.IsMap() || sink.size() != 1) {
        common::sys_error("Each sink entry must be a single-key mapping");
        return nullptr;
    }

    auto item = sink.begin();
    YAML::Node sink_name = item->first;
    YAML::Node settings = item->second;

    if (!sink_name.IsScalar()) {
        common::sys_error("The name 'sink' is not a scalar");
        return nullptr;
    }

    if (!settings.IsMap()) {
        common::sys_error("The set of arguments is not presented as a dictionary");
        return nullptr;
    }
    
    const auto enabled = settings["enabled"];

    if (!enabled || !enabled.IsScalar()) {
        common::sys_error("'enabled' is missing or is not a scalar");
        return nullptr;
    }

    if (!enabled.as<bool>()) {
        return nullptr;
    }

    std::string name = sink_name.as<std::string>();
    if (name == "console") return ConsoleSink::create(settings);
    else if (name == "file") return FileSink::create(settings);

    common::sys_error("There is no such sink: `{}`", name);
    return nullptr;

}


std::optional<LogConfig> ParseLogConfig(std::filesystem::path path)
{
    YAML::Node config;
    try {
        config = YAML::LoadFile(path.string());
    } catch(const YAML::Exception& msg) {
        common::sys_error(msg.what());
        return std::nullopt;
    }

    if (!config["logging"] || !config["logging"].IsMap()) {
        common::sys_error("'logging' is missing or is not a dictionary");
        return std::nullopt;
    }

    bool enabled = true;
    if (config["logging"]["enabled"]) {
        if (!config["logging"]["enabled"].IsScalar()) {
            common::sys_error("'enabled' field in 'logging' is not a scalar");
            return std::nullopt;
        }
        
        if (!config["logging"]["enabled"].as<bool>()) {
            enabled = false;
            common::sys_info("Logging has been disabled: 'enabled: false'");
            return std::nullopt;
        }
    }

    if (!config["logging"]["level"] || !config["logging"]["level"].IsScalar()) {
        common::sys_error("`level` is missing or is not a scalar");
        return std::nullopt;
    }

    if (!config["logging"]["sinks"] || !config["logging"]["sinks"].IsSequence()) {
        common::sys_error("`sinks` is missing or is not a sequence");
        return std::nullopt;
    } 

    common::status::log level = common::status::to_log(config["logging"]["level"].as<std::string>());
    if (level == common::status::log::unknown) {
        common::sys_error("unknown logging level");
        return std::nullopt;
    }

    std::vector<std::shared_ptr<ISink>> sinks;
    for (const auto& sink : config["logging"]["sinks"]) {
        if (auto created = CreateSink(sink)) {
            sinks.push_back(std::move(created));
        }
    }

    return LogConfig(enabled, level, std::move(sinks));
}


bool LoggingService::Init(std::filesystem::path path)
{
    std::unique_lock lock(mtx_ls_);

    std::unique_ptr<LoggingManager> lm;
    std::unique_ptr<common::config::ConfigHandler<LogConfig>> chandler;

    try {
        chandler = std::make_unique<common::config::ConfigHandler<LogConfig>>(ParseLogConfig, std::move(path));

        auto config = chandler->GetConfig();
        if (!config) {
            common::sys_warn("LoggingService is not initialized");
            lm = std::make_unique<LoggingManager>();
        }
        else {
            lm = std::make_unique<LoggingManager>(std::move(*config));
        }

    } catch(const std::bad_alloc& msg) {
        common::sys_error("Error initializing LoggingManager and config::LogConfigWatcher: {}", msg.what());
        return false;
    } catch(...) {
        common::sys_error("Error initializing LoggingManager and config::LogConfigWatcher");
        return false;
    }

    if (chandler_) {
        update_cycle_.request_stop();
        chandler_->StopWaiting();
    }

    chandler_ = std::move(chandler);
    lm_ = std::move(lm);

    lm_->Pause();

    lock.unlock();

    common::sys_info("LoggingManager and config::LogConfigWatcher initialized");
    return true;
}


bool LoggingService::Start() 
{
    Stop();

    std::lock_guard lock(mtx_ls_);
    if (!chandler_ || !lm_) {
        common::sys_error("The LoggingService::Init method was not called to initialize the linking");
        return false;
    } 

    update_cycle_ = std::jthread(&LoggingService::LoggerConfigUpdate);
    common::sys_info("The LoggingService was successfully started");

    return true;
}


void LoggingService::LoggerConfigUpdate(std::stop_token stop_tocken)
{
    while(!stop_tocken.stop_requested()) {
        if (!chandler_) return;

        auto config = chandler_->WaitForConfig();

        if (!config) {
            continue;
        }

        std::lock_guard lock(mtx_ls_);

        std::unique_ptr<LoggingManager> lm;
        try {
            lm = std::make_unique<LoggingManager>(std::move(*config));
        } catch(const std::bad_alloc& err) {
            common::sys_error("{}", err.what());
        }
        lm_ = std::move(lm);
    }
}


bool LoggingService::Start(std::filesystem::path path)
{
    std::unique_lock lock(mtx_ls_);

    std::unique_ptr<LoggingManager> lm;
    std::unique_ptr<common::config::ConfigHandler<LogConfig>> chandler;

    try {
        chandler = std::make_unique<common::config::ConfigHandler<LogConfig>>(ParseLogConfig, std::move(path));

        auto config = chandler->GetConfig();
        if (!config) {
            common::sys_warn("LoggingService is not initialized");
            lm = std::make_unique<LoggingManager>();
        }
        else {
            lm = std::make_unique<LoggingManager>(std::move(*config));
        }

    } catch(const std::bad_alloc& msg) {
        common::sys_error("Error initializing LoggingManager and config::LogConfigWatcher: {}", msg.what());
        return false;
    } catch(...) {
        common::sys_error("Error initializing LoggingManager and config::LogConfigWatcher");
        return false;
    }

    if (chandler_) {
        update_cycle_.request_stop();
        chandler_->StopWaiting();
    }

    chandler_ = std::move(chandler);
    lm_ = std::move(lm);

    update_cycle_ = std::jthread(&LoggingService::LoggerConfigUpdate);
    lock.unlock();

    common::sys_info("The LoggingService was successfully started");
    return true;
}


void LoggingService::Stop()
{
    std::lock_guard lock(mtx_ls_);

    if (chandler_) {
        update_cycle_.request_stop();
        chandler_->StopWaiting();
    }

    if (lm_) {
        lm_->Pause();
    }
}


std::optional<std::reference_wrapper<Logger>> LoggingService::Get() noexcept
{
    if (!lm_) {
        return std::nullopt;
    }
    return lm_->Get();
}


common::status::log LoggingService::GetLevel() noexcept
{
    if (!lm_) {
        return common::status::log::unknown;
    }
    return lm_->GetLevel();
}

} // namespace uni::logging::manager