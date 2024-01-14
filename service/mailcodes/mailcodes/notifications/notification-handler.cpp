#include "module.h"
#include "notification-handler.h"

namespace Notifications
{

void NotificationHandler::toastActivated() const
{
    ENTERED();

    LOGGER().debug("Toast was activated without an action index. Calling "
                   "OK-action handler");
    mToastOkCallback();

    FINISHED();
}

void NotificationHandler::toastActivated(int actionIndex) const
{
    ENTERED();

    switch (mActions.at(actionIndex))
    {
    case ToastAction::Ok:
        LOGGER().debug("Toast was activated with an OK-action");
        mToastOkCallback();
        break;
    case ToastAction::Cancel:
        LOGGER().debug("Toast was activated with a Cancel-action");
        mToastCancelCallback();
        break;
    }

    FINISHED();
}

void NotificationHandler::toastDismissed(WinToastDismissalReason state) const
{
    ENTERED();

    LOGGER().debug("Toast was dismissed with reason: {}", (int) state);
    mToastDismissedCallback(state);

    FINISHED();
}

void NotificationHandler::toastFailed() const
{
    ENTERED();

    LOGGER().debug("Toast failed");

    FINISHED();
}
} // namespace Notifications