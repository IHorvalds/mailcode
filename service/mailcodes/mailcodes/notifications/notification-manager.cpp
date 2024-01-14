#include "module.h"
#include "notification-manager.h"
#include "common/defines.h"

#include <cstdlib>
#include <wintoast/wintoastlib.h>

using namespace WinToastLib;

namespace Notifications
{

NotificationManager::NotificationManager()
{
    WinToast::WinToastError error;
    WinToast::instance()->setAppName(TOAST_APP_NAME);
    const auto aumi =
        WinToast::configureAUMI(TOAST_APP_COMPANY, TOAST_APP_NAME);
    WinToast::instance()->setAppUserModelId(aumi);

    if (!WinToast::instance()->initialize(&error))
    {
        LOGGER().info("Failed to initialize Toast Notifications: {}",
                      (int) error);
        std::exit(1);
    }
}

NotificationManager& NotificationManager::instance()
{
    static NotificationManager _instance;
    return _instance;
}

} // namespace Notifications
