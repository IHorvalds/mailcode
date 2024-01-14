#include "mail-service.h"
#include "common/defines.h"
#include "module.h"
#include "notifications/notification-manager.h"
#include <functional>

Mailservice::Mailservice() : CServiceBase(SERVICE_NAMEW, TRUE, TRUE, FALSE)
{
    mStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (mStoppedEvent == NULL)
    {
        throw GetLastError();
    }
}

Mailservice::~Mailservice()
{
    if (mStoppedEvent)
    {
        CloseHandle(mStoppedEvent);
        mStoppedEvent = NULL;
    }
}

void Mailservice::OnStart(DWORD dwArgc, PWSTR* pszArgv)
{
    ENTERED();

    LOGGER().info("Starting mail service");

    mWorker = std::jthread(std::bind(&Mailservice::ServiceWorkerThread, this,
                                     std::placeholders::_1));

    FINISHED();
}

void Mailservice::OnStop()
{
    ENTERED();

    LOGGER().info("Stopping mail service");

    mWorker.request_stop();

    if (WaitForSingleObject(mStoppedEvent, INFINITE) != WAIT_OBJECT_0)
    {
        throw GetLastError();
    }

    LOGGER().debug("Service stopped.");

    FINISHED();
}

void Mailservice::ServiceWorkerThread(std::stop_token token)
{
    ENTERED();

    // TODO: start file watcher here to watch emails db file
    //       on changes, it should log into new mailboxes and
    //       log out of removed mailboxes

    while (!token.stop_requested())
    {
        LOGGER().info("Checking emails");

        // TODO: Search and download mails here. Should only check
        //       logged in mailboxes
        // TODO: Search and extract auth code here
        // TODO: if any codes found, for each code show toast

        std::string auth_code = "123456";
        std::string email     = "abdefg@gmail.com";

        Notifications::NotificationManager::instance().showNotification(
            "New auth code found",
            std::format("Found new auth code for {}", email),
            [&auth_code, &email]() {
                LOGGER().info("Copying auth code \'{}\' to clipboard",
                              auth_code);
            },
            []() {
                LOGGER().info("Toast dismissed");
            },
            [](WinToastLib::IWinToastHandler::WinToastDismissalReason reason) {
                LOGGER().info("Toast dismissed with reason {}", (int) reason);
            });

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    LOGGER().info("Stopping service now");

    SetEvent(mStoppedEvent);

    FINISHED();
}
