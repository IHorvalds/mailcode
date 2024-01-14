#pragma once

#include <string>
#include <wintoast/wintoastlib.h>

#include "notification-handler.h"

using namespace WinToastLib;

namespace Notifications
{

class NotificationManager
{
public:
    static NotificationManager& instance();

    bool showNotification(std::string const& rsTitle,
                          std::string const& rsContent, auto&& toastOkCallback,
                          auto&& toastCancelCallback,
                          auto&& toastDismissedCallback)
    {
        ENTERED();

        std::wstring wTitle(L"\0", rsTitle.length());
        std::wstring wContent(L"\0", rsContent.length());

        auto size = mbstowcs(wTitle.data(), rsTitle.data(), rsTitle.length());
        if (size == -1)
        {
            LOGGER().error("Failed to convert multi-byte to wide-char string "
                           "for toast title");
            FINISHED(false);
            return false;
        }

        size = mbstowcs(wContent.data(), rsContent.data(), rsContent.length());
        if (size == -1)
        {
            LOGGER().error(
                "Failed to convert multi-byte to wide-char string for "
                "toast content");
            FINISHED(false);
            return false;
        }

        // Actually show the notification
        WinToastTemplate templ;
        if (!wContent.empty())
        {
            templ = WinToastTemplate(WinToastTemplate::Text02);
            templ.setTextField(wTitle, WinToastTemplate::FirstLine);
            templ.setTextField(wContent, WinToastTemplate::SecondLine);
        }
        else
        {
            templ = WinToastTemplate(WinToastTemplate::Text01);
            templ.setTextField(wTitle, WinToastTemplate::FirstLine);
        }

        templ.setDuration(WinToastTemplate::Duration::System);

        templ.addAction(L"Copy code");
        templ.addAction(L"Cancel");

        auto pToastManager = new NotificationHandler(
            toastOkCallback, toastCancelCallback, toastDismissedCallback,
            {ToastAction::Ok, ToastAction::Cancel});

        if (WinToast::instance()->showToast(templ, pToastManager) < 0)
        {
            LOGGER().error("Failed to show notification");
            FINISHED();
            return false;
        }

        FINISHED();
        return true;
    }

private:
    NotificationManager();
};

} // namespace Notifications
