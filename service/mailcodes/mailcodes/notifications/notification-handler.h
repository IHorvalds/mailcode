#ifndef NOTIFICATION_HANDLER_H
#define NOTIFICATION_HANDLER_H

#include <wintoast/wintoastlib.h>

using namespace WinToastLib;

namespace Notifications
{

enum class ToastAction
{
    Ok = 0,
    Cancel
};

class NotificationHandler : public IWinToastHandler
{
public:
    NotificationHandler(auto&& toastOkCallback, auto&& toastCancelCallback,
                        auto&&                   toastDismissedCallback,
                        std::vector<ToastAction> actions)
        : mToastOkCallback(toastOkCallback),
          mToastCancelCallback(toastCancelCallback),
          mToastDismissedCallback(toastDismissedCallback), mActions(actions)
    {
    }

    void toastActivated() const;

    void toastActivated(int actionIndex) const;

    void toastDismissed(WinToastDismissalReason state) const;

    void toastFailed() const;

private:
    const std::function<void(void)>                    mToastOkCallback;
    const std::function<void(void)>                    mToastCancelCallback;
    const std::function<void(WinToastDismissalReason)> mToastDismissedCallback;
    const std::vector<ToastAction>                     mActions;
};

} // namespace Notifications

#endif // NOTIFICATION_HANDLER_H