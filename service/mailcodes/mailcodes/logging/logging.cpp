#include "logging.h"

spdlog::logger& getDefaultLogger(std::string const& svModuleName)
{
    auto pExistingLogger = spdlog::get(svModuleName);
    if (!pExistingLogger)
    {
        const auto loggerPath = Paths::kLOGS_DIR / std::filesystem::path(svModuleName + ".txt");
        const auto maxSize    = 10 * 1024 * 1024; // 10M
        const auto maxFiles   = 5;                // 5

        auto logger = spdlog::rotating_logger_mt(svModuleName, loggerPath.string(), maxSize, maxFiles);
        spdlog::register_logger(logger); // logger moved

        pExistingLogger = spdlog::get(svModuleName);
    }

    return *pExistingLogger;
}
