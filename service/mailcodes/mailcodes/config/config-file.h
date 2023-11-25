#pragma once

#include "module.h"
#include <string>
#include <concepts>
#include <nlohmann/json.hpp>
#include <wil/filesystem.h>

namespace Config
{

class ConfigFile : public std::enable_shared_from_this<ConfigFile>
{
public:
    //! @brief Factory method
    static std::shared_ptr<ConfigFile> open(std::string_view svFilename);

    ~ConfigFile() = default;

    std::shared_ptr<ConfigFile> getSharedPtr();

    //! @brief Watch the file for changes and reload
    void watch(
        std::function<void(std::shared_ptr<ConfigFile>)> callback = nullptr);

    //! @brief Stop watching the file for changes
    void unwatch();

    //! @brief Check if the config is loaded
    bool loaded() const noexcept;

    //! @brief Check if the config file is being watched for changes
    bool isWatching() const noexcept;

    //! @brief Get a const reference to the list of keys in this config file
    const std::list<std::string>& keys() const noexcept;

    //! @brief Get the value of a key
    //! @param svKey key
    //! @param rrDefault default value to return in case the key doesn't exist
    //! @return the value for the key or the default value
    template <typename TVal>
        requires std::default_initializable<TVal>
    TVal getValue(const std::string& key, const TVal& _default)
    {
        std::scoped_lock<std::mutex> lock(mMutex);
        try
        {
            return mContents.value(key, _default);
        }
        catch (const nlohmann::json::type_error& rJsonTypeError)
        {
            LOGGER().error("\"{}\": Failed to get value for key \"{}\": {}",
                           mConfigName, key, rJsonTypeError.what());
            return _default;
        }
    }

private:
    ConfigFile(std::string_view svFilename);

    void load(const std::filesystem::path& absolutePath);

    nlohmann::json                     mContents;
    std::string                        mConfigName;
    bool                               mLoaded;
    bool                               mWatchingChanges;
    std::list<std::string>             mKeys;
    std::mutex                         mMutex;
    wil::unique_folder_watcher_nothrow mFilesystemWatcher;
};

} // namespace Config