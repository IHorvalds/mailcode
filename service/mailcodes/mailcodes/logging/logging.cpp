#include "logging.h"
#include <iostream>

spdlog::logger& getDefaultLogger(std::string const& svModuleName)
{
    static std::once_flag spdlog_error_handler_flag;
    std::call_once(spdlog_error_handler_flag, []() {
        spdlog::set_error_handler([](const std::string& msg) {
            spdlog::critical("Logger had an error: {}", msg);
        });
    });

    auto pExistingLogger = spdlog::get(svModuleName);
    if (!pExistingLogger)
    {
        if (!std::filesystem::exists(Paths::kLOGS_DIR) && !std::filesystem::create_directory(Paths::kLOGS_DIR))
        {
            std::cerr << "Failed to create logs directory\n";
            std::abort();
        }

        const auto loggerPath = Paths::kLOGS_DIR / std::filesystem::path(svModuleName + ".txt");
        const auto maxSize    = 10 * 1024 * 1024; // 10M
        const auto maxFiles   = 5;                // 5

        spdlog::rotating_logger_mt(svModuleName, loggerPath.string(), maxSize, maxFiles);

        pExistingLogger = spdlog::get(svModuleName);
    }

    return *pExistingLogger;
}
