#include "module.h"
#include "notification-handler.h"

namespace Notifications
{

void NotificationHandler::toastActivated() const
{
    ENTERED();

    if (mCallbacks.size() < 1)
    {
        LOGGER().warn("Toast notification was activated, but no callbacks were registered. Was this intentional?");
        FINISHED();
        return;
    }

    if (!mCallbacks[0]())
    {
        LOGGER().error("Callback function failed");
        mState = NotificationState::Failed;
    }
    else
    {
        LOGGER().info("Successfully ran callback");
        mState = NotificationState::Successful;
    }

    FINISHED();
}

void NotificationHandler::toastActivated(int actionIndex) const
{
    ENTERED();

    if (mCallbacks.size() < actionIndex - 1)
    {
        LOGGER().error("Toast notification was activated with index {}, but not callbacks were registered. This is "
                       "definitely an error.",
                       actionIndex);
        FINISHED();
        return;
    }

    if (!mCallbacks[actionIndex]())
    {
        LOGGER().error("Callback function failed");
        mState = NotificationState::Failed;
    }
    else
    {
        LOGGER().info("Successfully ran callback");
        mState = NotificationState::Successful;
    }

    FINISHED();
}

void NotificationHandler::toastDismissed(WinToastDismissalReason state) const
{
    ENTERED();

    LOGGER().debug("Toast was dimissed for reason: {}", (int) state);
    mState = NotificationState::Dismissed;

    FINISHED();
}

void NotificationHandler::toastFailed() const
{
    ENTERED();

    LOGGER().debug("Toast failed");
    mState = NotificationState::Failed;

    FINISHED();
}

NotificationState NotificationHandler::getCurrentState() const
{
    return mState;
}
} // namespace Notifications