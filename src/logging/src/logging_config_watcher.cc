#include "project/logging/logging_config_watcher.h"

#ifdef __APPLE__
#include "project/logging/logging_config_watcher_macos.h"
#endif


namespace uni::logging::config {


std::unique_ptr<ISink> CreateSink(const YAML::Node& sink)
{
    if (!sink.IsMap() || sink.size() != 1) {
        sys_error("Each sink entry must be a single-key mapping");
        return nullptr;
    }

    auto item = sink.begin();
    YAML::Node sink_name = item->first;
    YAML::Node settings = item->second;

    if (!sink_name.IsScalar()) {
        sys_error("The name 'sink' is not a scalar");
        return nullptr;
    }

    if (!settings.IsMap()) {
        sys_error("The set of arguments is not presented as a dictionary");
        return nullptr;
    }
    
    const auto enabled = settings["enabled"];

    if (!enabled || !enabled.IsScalar()) {
        sys_error("'enabled' is missing or is not a scalar");
        return nullptr;
    }

    if (!enabled.as<bool>()) {
        return nullptr;
    }

    std::string name = sink_name.as<std::string>();
    if (name == "console") return ConsoleSink::create(settings);
    else if (name == "file") return FileSink::create(settings);

    sys_error("There is no such sink: `{}`", name);
    return nullptr;

}


std::optional<LogConfig> ParseLogConfig(std::filesystem::path path)
{
    YAML::Node config;
    try {
        config = YAML::LoadFile(path.string());
    } catch(const YAML::Exception& msg) {
        sys_error(msg.what());
        return std::nullopt;
    }

    if (!config["logging"] || !config["logging"].IsMap()) {
        sys_error("'logging' is missing or is not a dictionary");
        return std::nullopt;
    }

    bool enabled = true;
    if (config["logging"]["enabled"]) {
        if (!config["logging"]["enabled"].IsScalar()) {
            sys_error("'enabled' field in 'logging' is not a scalar");
            return std::nullopt;
        }
        
        if (!config["logging"]["enabled"].as<bool>()) {
            enabled = false;
            sys_info("Logging has been disabled: 'enabled: false'");
            return std::nullopt;
        }
        else {
            sys_info("Logging has been enabled: 'enabled: true'");
        }
    }

    if (!config["logging"]["level"] || !config["logging"]["level"].IsScalar()) {
        sys_error("`level` is missing or is not a scalar");
        return std::nullopt;
    }

    if (!config["logging"]["sinks"] || !config["logging"]["sinks"].IsSequence()) {
        sys_error("`sinks` is missing or is not a sequence");
        return std::nullopt;
    } 

    common::status::log level = common::status::to_log(config["logging"]["level"].as<std::string>());
    if (level == common::status::log::unknown) {
        sys_error("unknown logging level");
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


LogConfigWatcher::LogConfigWatcher(std::filesystem::path path) : impl_(std::make_unique<LogConfigWatcherImpl>(std::move(path))) {}

LogConfigWatcher::~LogConfigWatcher() = default;

void LogConfigWatcher::Start() { impl_->Start(); }
void LogConfigWatcher::Stop() { impl_->Stop(); }

} // namespace uni::logging::config