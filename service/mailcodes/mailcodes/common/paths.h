#pragma once

#include <filesystem>

namespace Paths
{

using namespace std::filesystem;

const path kBASE_PATH = current_path();
const path kLOGS_DIR  = kBASE_PATH / path("logs");
const path kDB_DIR    = kBASE_PATH / path("db");

const path kDB_FILE = []() {
    // TODO: read this from a config file
    path filename = "emails.db";
    return kDB_DIR / filename;
}();

} // namespace Paths