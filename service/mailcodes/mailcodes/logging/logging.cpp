#include "logging.h"
#include "common/defines.h"
#include "config/config-file.h"
#include <iostream>

namespace
{
static std::shared_ptr<Config::ConfigFile> pLoggingConfig = [] {
    const std::string loggerConfigFile = "logger.json";
    auto pLoggingConfig = Config::ConfigFile::open(loggerConfigFile);

    if (!pLoggingConfig)
    {
        getSystemEventLogger().critical(
            "Failed to allocate memory for Config File {}", loggerConfigFile);
    }

    std::string logLevel = SPDLOG_LEVEL_NAME_INFO.data();
    if (pLoggingConfig->loaded())
    {
        logLevel = pLoggingConfig->getValue<std::string>(
            std::string("logLevel"), logLevel);
    }

    spdlog::set_level(spdlog::level::from_str(logLevel));

    pLoggingConfig->watch([](std::shared_ptr<Config::ConfigFile> pConfig) {
        std::string logLevel =
            spdlog::level::to_string_view(spdlog::get_level()).data();
        if (pConfig->loaded())
        {
            logLevel = pConfig->getValue("logLevel", logLevel);
        }

        spdlog::set_level(spdlog::level::from_str(logLevel));
    });

    return pLoggingConfig;
}();
} // namespace

spdlog::logger& getDefaultLogger(std::string const& svModuleName)
{
    static std::once_flag spdlog_error_handler_flag;
    std::call_once(spdlog_error_handler_flag, []() {
        spdlog::set_error_handler([](const std::string& msg) {
            spdlog::critical("Logger had an error: {}", msg);
        });

        spdlog::flush_every(std::chrono::seconds(1));
    });

    auto pExistingLogger = spdlog::get(svModuleName);
    if (!pExistingLogger)
    {
        if (!std::filesystem::exists(Paths::kLOGS_DIR) &&
            !std::filesystem::create_directory(Paths::kLOGS_DIR))
        {
            std::cerr << "Failed to create logs directory\n";
            std::abort();
        }

        const auto loggerPath =
            Paths::kLOGS_DIR / std::filesystem::path(svModuleName + ".txt");
        const auto maxSize  = 10 * 1024 * 1024; // 10M
        const auto maxFiles = 5;                // 5

        spdlog::rotating_logger_mt(svModuleName, loggerPath.string(), maxSize,
                                   maxFiles);

        pExistingLogger = spdlog::get(svModuleName);
    }

    return *pExistingLogger;
}

spdlog::logger& getSystemEventLogger()
{
    static auto logger = std::make_shared<spdlog::logger>(
        "win_event_logger",
        std::make_shared<spdlog::sinks::win_eventlog_sink_st>(SERVICE_NAME));
    return *logger;
}
