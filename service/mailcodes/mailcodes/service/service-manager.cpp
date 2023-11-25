#include <iostream>
#include <gsl/gsl>

#include "module.h"
#include "common/defines.h"
#include "logging/logging.h"

namespace
{

void GetServiceStatusOrThrow(SC_HANDLE schService, LPSERVICE_STATUS lpssSvcStatus)
{
    if (!QueryServiceStatus(schService, lpssSvcStatus))
    {
        throw std::runtime_error(std::format("QueryServiceStatus failed with err {:#016Lx}", GetLastError()));
    }
}

} // namespace

bool installService()
{
    ENTERED();
    bool result = true;

    try
    {
        wchar_t   szPath[MAX_PATH];
        SC_HANDLE schSCManager = NULL;
        SC_HANDLE schService   = NULL;

        auto cleanup = gsl::finally([&schSCManager, &schService]() {
            if (schSCManager)
            {
                CloseServiceHandle(schSCManager);
                schSCManager = NULL;
            }
            if (schService)
            {
                CloseServiceHandle(schService);
                schService = NULL;
            }
        });

        if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0)
        {
            throw std::runtime_error(std::format("GetModuleFileName failed with err {:#016Lx}", GetLastError()));
        }

        // Open the local default service control manager database
        schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
        if (schSCManager == NULL)
        {
            throw std::runtime_error(std::format("OpenSCManager failed with err {:#016Lx}", GetLastError()));
        }

        // Install the service into SCM by calling CreateService
        schService = CreateService(schSCManager,              // SCManager database
                                   SERVICE_NAMEW,             // Name of service
                                   SERVICE_DISPLAY_NAMEW,     // Name to display
                                   SERVICE_QUERY_STATUS,      // Desired access
                                   SERVICE_WIN32_OWN_PROCESS, // Service type
                                   SERVICE_START_TYPE,        // Service start type
                                   SERVICE_ERROR_NORMAL,      // Error control type
                                   szPath,                    // Service's binary
                                   NULL,                      // No load ordering group
                                   NULL,                      // No tag identifier
                                   SERVICE_DEPENDENCIES,      // Dependencies
                                   SERVICE_ACCOUNT,           // Service running account
                                   SERVICE_PASSWORD           // Password of the account
        );

        if (schService == NULL)
        {
            throw std::runtime_error(std::format("CreateService failed with err {:#016Lx}", GetLastError()));
        }

        auto successMsg = std::format("Service {} is installed", SERVICE_NAME);
        LOGGER().info(successMsg);
        std::cout << successMsg << std::endl;
    }
    catch (const std::runtime_error& roError)
    {
        LOGGER().error("Error installing the service: {}", roError.what());
        result = false;
    }

    FINISHED("Installed service: {}", result);
    return result;
}

bool uninstallService()
{
    ENTERED();
    bool result = true;

    try
    {
        SC_HANDLE      schSCManager = NULL;
        SC_HANDLE      schService   = NULL;
        SERVICE_STATUS ssSvcStatus  = {};

        auto cleanup = gsl::finally([&schSCManager, &schService]() {
            if (schSCManager)
            {
                CloseServiceHandle(schSCManager);
                schSCManager = NULL;
            }
            if (schService)
            {
                CloseServiceHandle(schService);
                schService = NULL;
            }
        });

        // Open the local default service control manager database
        schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (schSCManager == NULL)
        {
            throw std::runtime_error(std::format("OpenSCManager failed with err {:#016Lx}", GetLastError()));
        }

        // Open the service with delete, stop, and query status permissions
        schService = OpenService(schSCManager, SERVICE_NAMEW, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
        if (schService == NULL)
        {
            throw std::runtime_error(std::format("OpenService failed with err {:#016Lx}", GetLastError()));
        }

        // Try to stop the service
        if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
        {
            LOGGER().info("Stopping {}", SERVICE_NAME);

            while (QueryServiceStatus(schService, &ssSvcStatus))
            {
                if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
                {
                    LOGGER().info("Waiting for service to stop");
                    Sleep(1000);
                }
                else
                {
                    break;
                }
            }

            if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
            {
                LOGGER().info("Service {} has stopped", SERVICE_NAME);
            }
            else
            {
                LOGGER().error("Service {} failed to stop", SERVICE_NAME);
            }
        }

        // Now remove the service by calling DeleteService.
        if (!DeleteService(schService))
        {
            throw std::runtime_error(std::format("DeleteService failed with err {:#016Lx}", GetLastError()));
        }

        auto successMsg = std::format("Service {} has been removed", SERVICE_NAME);
        LOGGER().info(successMsg);
        std::cout << successMsg << std::endl;
    }
    catch (const std::runtime_error& roError)
    {
        LOGGER().error("Error uninstalling the service: {}", roError.what());
        result = false;
    }

    FINISHED("Uninstalled service: {}", result);
    return result;
}

bool startService()
{
    ENTERED();
    bool result = true;

    try
    {
        SC_HANDLE      schSCManager = NULL;
        SC_HANDLE      schService   = NULL;
        SERVICE_STATUS ssSvcStatus  = {};

        auto cleanup = gsl::finally([&schSCManager, &schService]() {
            if (schSCManager)
            {
                CloseServiceHandle(schSCManager);
                schSCManager = NULL;
            }
            if (schService)
            {
                CloseServiceHandle(schService);
                schService = NULL;
            }
        });

        // Open the local default service control manager database
        schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (schSCManager == NULL)
        {
            throw std::runtime_error(std::format("OpenSCManager failed with err {:#016Lx}", GetLastError()));
        }

        // Open the service with read, start, and query status permissions
        schService = OpenService(schSCManager, SERVICE_NAMEW, SERVICE_START | SERVICE_QUERY_STATUS | READ_CONTROL);
        if (schService == NULL)
        {
            throw std::runtime_error(std::format("OpenService failed with err {:#016Lx}", GetLastError()));
        }

        // Check if the service is stopped or in a pending state
        GetServiceStatusOrThrow(schService, &ssSvcStatus);

        if (ssSvcStatus.dwCurrentState == SERVICE_RUNNING)
        {
            throw std::runtime_error("Service is already running");
        }

        if (ssSvcStatus.dwCurrentState == SERVICE_START_PENDING)
        {
            throw std::runtime_error("Service is already starting");
        }

        auto dwStartTickCount = GetTickCount64();
        auto dwOldCheckPoint  = (ULONGLONG) ssSvcStatus.dwCheckPoint;

        // Wait for it to stop
        while (ssSvcStatus.dwCurrentState != SERVICE_STOPPED)
        {
            auto dwWaitTime = std::clamp(ssSvcStatus.dwWaitHint / 10, (DWORD) 1000, (DWORD) 10000);

            Sleep(dwWaitTime);

            GetServiceStatusOrThrow(schService, &ssSvcStatus);

            if (ssSvcStatus.dwCheckPoint > dwOldCheckPoint)
            {
                // Continue to wait and check.

                dwStartTickCount = GetTickCount64();
                dwOldCheckPoint  = ssSvcStatus.dwCheckPoint;
            }
            else
            {
                if (GetTickCount64() - dwStartTickCount > ssSvcStatus.dwWaitHint)
                {
                    throw std::runtime_error("Timeout waiting for service to stop");
                }
            }
        }

        // Attempt to start the service
        if (!StartService(schService, 0, NULL))
        {
            throw std::runtime_error(std::format("StartService failed with err {:#016Lx}", GetLastError()));
        }

        LOGGER().info("Service {} starting...", SERVICE_NAME);
        std::cout << std::format("Service {} starting...", SERVICE_NAME);

        GetServiceStatusOrThrow(schService, &ssSvcStatus);

        dwStartTickCount = GetTickCount64();
        dwOldCheckPoint  = (ULONGLONG) ssSvcStatus.dwCheckPoint;

        while (ssSvcStatus.dwCurrentState == SERVICE_START_PENDING)
        {
            auto dwWaitTime = std::clamp(ssSvcStatus.dwWaitHint, (DWORD) 1000, (DWORD) 10000);

            Sleep(dwWaitTime);

            GetServiceStatusOrThrow(schService, &ssSvcStatus);

            if (ssSvcStatus.dwCheckPoint > dwOldCheckPoint)
            {
                // Continue to wait and check.

                dwStartTickCount = GetTickCount64();
                dwOldCheckPoint  = ssSvcStatus.dwCheckPoint;
            }
            else
            {
                if (GetTickCount64() - dwStartTickCount > ssSvcStatus.dwWaitHint)
                {
                    throw std::runtime_error("Timeout waiting for service to start");
                }
            }
        }

        result = ssSvcStatus.dwCurrentState == SERVICE_RUNNING;

        LOGGER().info("Service {} is running: {}", SERVICE_NAME, result);
    }
    catch (const std::runtime_error& roError)
    {
        LOGGER().error("Error starting the service: {}", roError.what());
        result = false;
    }

    FINISHED("Started service: {}", result);
    return result;
}

bool stopService()
{
    ENTERED();
    bool result = true;

    try
    {
        SC_HANDLE      schSCManager = NULL;
        SC_HANDLE      schService   = NULL;
        SERVICE_STATUS ssSvcStatus  = {};

        auto cleanup = gsl::finally([&schSCManager, &schService]() {
            if (schSCManager)
            {
                CloseServiceHandle(schSCManager);
                schSCManager = NULL;
            }
            if (schService)
            {
                CloseServiceHandle(schService);
                schService = NULL;
            }
        });

        // Open the local default service control manager database
        schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (schSCManager == NULL)
        {
            throw std::runtime_error(std::format("OpenSCManager failed with err {:#016Lx}", GetLastError()));
        }

        // Open the service with delete, stop, and query status permissions
        schService = OpenService(schSCManager, SERVICE_NAMEW, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
        if (schService == NULL)
        {
            throw std::runtime_error(std::format("OpenService failed with err {:#016Lx}", GetLastError()));
        }

        // Try to stop the service
        if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
        {
            LOGGER().info("Stopping {}", SERVICE_NAME);

            while (QueryServiceStatus(schService, &ssSvcStatus))
            {
                if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
                {
                    LOGGER().info("Waiting for service to stop");
                    Sleep(1000);
                }
                else
                {
                    break;
                }
            }

            if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
            {
                LOGGER().info("Service {} has stopped", SERVICE_NAME);
            }
            else
            {
                LOGGER().error("Service {} failed to stop", SERVICE_NAME);
            }
        }

        LOGGER().info("Service {} has been stoppped", SERVICE_NAME);
    }
    catch (const std::runtime_error& roError)
    {
        LOGGER().error("Error stopping the service: {}", roError.what());
        result = false;
    }

    FINISHED("Stopped service: {}", result);
    return result;
}
