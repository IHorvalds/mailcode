#include "mail-service.h"
#include "common/defines.h"
#include "module.h"
#include <functional>
#include <iostream>
#include <fstream>

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

    mWorker = std::jthread(std::bind(&Mailservice::ServiceWorkerThread, this, std::placeholders::_1));

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

    while (!token.stop_requested())
    {
        // TODO: Add the mail checking here
        LOGGER().info(std::format("Not stopping yet at {}...", std::chrono::system_clock::now()));
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    LOGGER().info(std::format("Stopping now {}", std::chrono::system_clock::now()));

    SetEvent(mStoppedEvent);

    FINISHED();
}
