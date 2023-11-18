#pragma once

#include <string>

#include "notification-handler.h"

namespace Notifications
{

class NotificationManager
{
public:
    static NotificationManager& instance();

    bool showNotification(std::string const& rsTitle, std::string const& rsContent, NotificationCallback<>&& rCallback);

private:
    NotificationManager();
};

} // namespace Notifications
