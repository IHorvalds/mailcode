#pragma once

#include <filesystem>
#include "defines.h"
#include <spdlog/sinks/win_eventlog_sink.h>

namespace Paths
{

using namespace std::filesystem;

static const path kBASE_PATH = [] {
    char szPath[MAX_PATH];
    if (!GetModuleFileNameA(NULL, szPath, ARRAYSIZE(szPath)))
    {
        auto logger = std::make_shared<spdlog::logger>(
            "win_event_logger",
            std::make_shared<spdlog::sinks::win_eventlog_sink_st>(
                SERVICE_NAME));
        if (logger)
        {
            logger->critical("Failed to get executable file path. Exiting");
        }
        std::abort();
    }

    return std::filesystem::path(szPath).parent_path();
}();

static const path kCONFIG_DIR = kBASE_PATH / path("config");
static const path kLOGS_DIR   = kBASE_PATH / path("logs");
static const path kDB_DIR     = kBASE_PATH / path("db");

static const path kDB_FILE = []() {
    // TODO: read this from a config file
    path filename = "emails.db";
    return kDB_DIR / filename;
}();

} // namespace Paths