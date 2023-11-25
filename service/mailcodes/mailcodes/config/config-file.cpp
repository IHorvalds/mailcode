#include "config-file.h"
#include "common/paths.h"
#include <fstream>
#include <wil/resource.h>

namespace Config
{

ConfigFile::ConfigFile(std::string_view svFilename)
{
    mConfigName       = svFilename;
    mLoaded           = false;
    auto absolutePath = Paths::kCONFIG_DIR / svFilename;
    load(absolutePath);
}

std::shared_ptr<ConfigFile> ConfigFile::open(std::string_view svFilename)
{
    return std::shared_ptr<ConfigFile>(new ConfigFile(svFilename));
}

inline std::shared_ptr<ConfigFile> ConfigFile::getSharedPtr()
{
    return shared_from_this();
}

const std::list<std::string>& ConfigFile::keys() const noexcept
{
    return mKeys;
}

void ConfigFile::load(const std::filesystem::path& absolutePath)
{
    std::scoped_lock<std::mutex> lock(mMutex);
    if (std::filesystem::exists(absolutePath))
    {
        try
        {
            std::stringstream ss;
            auto              fileStream = std::ifstream(absolutePath.string());
            while (!fileStream.eof())
            {
                std::string _str;
                std::getline(fileStream, _str);
                ss << _str;
            }

            mContents = nlohmann::json::parse(ss.str(), nullptr, true, true);

            mKeys.clear();
            for (auto itr = mContents.cbegin(); itr != mContents.cend(); ++itr)
            {
                mKeys.push_back(itr.key());
            }

            mLoaded = true;
        }
        catch (const std::ios_base::failure& fstreamError)
        {
            LOGGER().error("Failed to read config file \"{}\": {}",
                           absolutePath.string(), fstreamError.what());
        }
        catch (const nlohmann::json::parse_error& jsonParseError)
        {
            LOGGER().error("Failed to parse config file \"{}\": {}",
                           absolutePath.string(), jsonParseError.what());
        }
    }
}

bool ConfigFile::loaded() const noexcept
{
    return mLoaded;
}

bool ConfigFile::isWatching() const noexcept
{
    return mWatchingChanges;
}

void ConfigFile::watch(
    std::function<void(std::shared_ptr<Config::ConfigFile>)> callback)
{
    LOGGER().info("Starting to watch config file {}", mConfigName);
    mFilesystemWatcher = wil::make_folder_watcher_nothrow(
        Paths::kCONFIG_DIR.c_str(), true,
        wil::FolderChangeEvents::LastWriteTime,
        [self = shared_from_this(), callback]() {
            self->load(Paths::kCONFIG_DIR / self->mConfigName);

            if (callback)
            {
                callback(self);
            }
        });

    mWatchingChanges = !!mFilesystemWatcher;
    if (!mFilesystemWatcher)
    {
        LOGGER().error(
            "Failed to setup file system watcher for {}. Error {:#016Lx}",
            (Paths::kCONFIG_DIR / mConfigName).string(), GetLastError());
    }
}

void ConfigFile::unwatch()
{
    LOGGER().info("Stop watching config file {}", mConfigName);
    mFilesystemWatcher.reset();
    mWatchingChanges = false;
}

} // namespace Config
