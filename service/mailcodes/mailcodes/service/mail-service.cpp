#include "mail-service.h"
#include "common/defines.h"
#include "module.h"
#include <functional>
#include <iostream>

Mailservice::Mailservice() : CServiceBase(SERVICE_NAME, TRUE, TRUE, TRUE), mStoppedEvent(INVALID_HANDLE_VALUE)
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
    LOGGER().info("Starting mail service");

    mWorker = std::jthread(std::bind(&Mailservice::ServiceWorkerThread, this, std::placeholders::_1));
}

void Mailservice::OnStop()
{
    LOGGER().info("Stopping mail service");

    bool stopRequested = mWorker.request_stop();

    LOGGER().debug("Stop requested. Successful: {}", stopRequested);

    while (mRunning == true)
    {
        SetServiceStatus(SERVICE_STOP_PENDING, 0UL, 10UL);
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    }

    LOGGER().debug("Service stopped.");
}

void Mailservice::ServiceWorkerThread(std::stop_token token)
{
    while (!token.stop_requested())
    {
        // TODO: Add the mail checking here
        mRunning.store(true);
        std::cout << std::format("Not stopping yet at {}...\n", std::chrono::system_clock::now());
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    mRunning.store(false);
    std::cout << std::format("Stopping now {}\n", std::chrono::system_clock::now());
}
