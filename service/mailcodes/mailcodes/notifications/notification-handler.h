#ifndef NOTIFICATION_HANDLER_H
#define NOTIFICATION_HANDLER_H

#include <concepts>
#include <wintoast/wintoastlib.h>
#include "notification-callback.h"

using namespace WinToastLib;

namespace Notifications
{

enum class NotificationState : int
{
    Invalid    = -1,
    Successful = 0,
    Dismissed,
    Failed
};

class NotificationHandler : public IWinToastHandler
{
private:
    const std::vector<NotificationCallback<>> mCallbacks;
    mutable NotificationState                 mState;

public:
    NotificationHandler(std::vector<NotificationCallback<>>& rCallbacks)
        : mCallbacks(rCallbacks), mState(NotificationState::Invalid)
    {
    }

    void toastActivated() const;

    void toastActivated(int actionIndex) const;

    void toastDismissed(WinToastDismissalReason state) const;

    void toastFailed() const;

    NotificationState getCurrentState() const;
};

} // namespace Notifications

#endif // NOTIFICATION_HANDLER_H