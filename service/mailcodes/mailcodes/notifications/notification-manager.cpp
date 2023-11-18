#include "module.h"
#include "notification-manager.h"

#include <wintoast/wintoastlib.h>

using namespace WinToastLib;

namespace Notifications
{

NotificationManager::NotificationManager()
{
}

NotificationManager& NotificationManager::instance()
{
    static NotificationManager _instance;
    return _instance;
}

bool NotificationManager::showNotification(std::string const& rsTitle, std::string const& rsContent,
                                           NotificationCallback<>&& rCallback)
{
    ENTERED();
    bool result = true;

    // Actually show the notification

    FINISHED();
    return result;
}

} // namespace Notifications
