#pragma once

#include "ServiceBase.h"
#include <thread>

class Mailservice : public CServiceBase
{
public:
    Mailservice();
    virtual ~Mailservice();

protected:
    virtual void OnStart(DWORD dwArgc, PWSTR* pszArgv) override;
    virtual void OnStop() override;

    void ServiceWorkerThread(std::stop_token token);

private:
    std::atomic_bool mRunning;
    HANDLE           mStoppedEvent;
    std::jthread     mWorker;
};
