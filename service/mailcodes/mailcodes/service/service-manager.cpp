#include <iostream>
#include <gsl/gsl>
#include <Aclapi.h>
#include <AccCtrl.h>

#include "module.h"
#include "common/defines.h"
#include "logging/logging.h"
#include "service-manager.h"

namespace
{

void GetServiceStatusOrThrow(SC_HANDLE        schService,
                             LPSERVICE_STATUS lpssSvcStatus)
{
    if (!QueryServiceStatus(schService, lpssSvcStatus))
    {
        throw std::runtime_error(std::format(
            "QueryServiceStatus failed with err {:#016Lx}", GetLastError()));
    }
}

//! @brief Gives the current user READ_CONTROL, START_SERVICE and STOP_SERVICE
//!        privileges.
//! @note Does **not** own the handles in the arguments and so does **not**
//!       clean them up. It's the caller's responsibility to do that.
//! @note Adapted from the example code on
//!       https://learn.microsoft.com/en-us/windows/win32/services/modifying-the-dacl-for-a-service
//! @return true on success, false otherwise
bool GiveLoggedInUserStartStopAccess(SC_HANDLE schSCManager,
                                     SC_HANDLE schService)
{
    if (!schSCManager)
    {
        LOGGER().debug("Invalid SCM pointer given");
        return false;
    }

    if (!schService)
    {
        LOGGER().debug("Invalid Service pointer given");
        return false;
    }

    EXPLICIT_ACCESS      ea             = {};
    SECURITY_DESCRIPTOR  sd             = {};
    PSECURITY_DESCRIPTOR psd            = NULL;
    PACL                 pacl           = NULL;
    PACL                 pNewAcl        = NULL;
    BOOL                 bDaclPresent   = FALSE;
    BOOL                 bDaclDefaulted = FALSE;
    DWORD                dwError        = 0;
    DWORD                dwSize         = 0;
    DWORD                dwBytesNeeded  = 0;

    // Cleanup allocated things at scope-exit
    auto cleanup = gsl::finally([&pNewAcl, &psd] {
        if (NULL != pNewAcl)
            LocalFree((HLOCAL) pNewAcl);
        if (NULL != psd)
            HeapFree(GetProcessHeap(), 0, (LPVOID) psd);
    });

    // Get the current security descriptor.

    if (!QueryServiceObjectSecurity(
            schService, DACL_SECURITY_INFORMATION,
            &psd, // using NULL does not work on all versions
            0, &dwBytesNeeded))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            dwSize = dwBytesNeeded;
            psd    = (PSECURITY_DESCRIPTOR) HeapAlloc(GetProcessHeap(),
                                                      HEAP_ZERO_MEMORY, dwSize);
            if (psd == NULL)
            {
                // Note: HeapAlloc does not support GetLastError.
                LOGGER().error("HeapAlloc failed");
                return false;
            }

            if (!QueryServiceObjectSecurity(schService,
                                            DACL_SECURITY_INFORMATION, psd,
                                            dwSize, &dwBytesNeeded))
            {
                LOGGER().error("QueryServiceObjectSecurity failed: {:#016Lx}",
                               GetLastError());
                return false;
            }
        }
        else
        {
            LOGGER().error("QueryServiceObjectSecurity failed: {:#016Lx}",
                           GetLastError());
            return false;
        }
    }

    // Get the DACL.

    if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, &bDaclDefaulted))
    {
        LOGGER().error("GetSecurityDescriptorDacl failed: {:#016Lx}",
                       GetLastError());
        return false;
    }

    // Build the ACE.

    wchar_t current_user[] = L"CURRENT_USER";
    BuildExplicitAccessWithName(
        &ea, current_user, SERVICE_START | SERVICE_STOP | READ_CONTROL | DELETE,
        SET_ACCESS, NO_INHERITANCE);

    dwError = SetEntriesInAcl(1, &ea, pacl, &pNewAcl);
    if (dwError != ERROR_SUCCESS)
    {
        LOGGER().error("SetEntriesInAcl failed: {:#016Lx}", dwError);
        return false;
    }

    // Initialize a new security descriptor.

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
    {
        LOGGER().error("InitializeSecurityDescriptor failed: {:#016Lx}",
                       GetLastError());
        return false;
    }

    // Set the new DACL in the security descriptor.

    if (!SetSecurityDescriptorDacl(&sd, TRUE, pNewAcl, FALSE))
    {
        LOGGER().error("SetSecurityDescriptorDacl failed: {:#016Lx}",
                       GetLastError());
        return false;
    }

    // Set the new DACL for the service object.
    if (!SetServiceObjectSecurity(schService, DACL_SECURITY_INFORMATION, &sd))
    {
        LOGGER().error("SetServiceObjectSecurity failed: {:#016Lx}",
                       GetLastError());
        return false;
    }

    LOGGER().info("Service DACL updated successfully");
    return true;
}

} // namespace

namespace ServiceManager
{

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
            throw std::runtime_error(std::format(
                "GetModuleFileName failed with err {:#016Lx}", GetLastError()));
        }

        // Open the local default service control manager database
        schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (schSCManager == NULL)
        {
            throw std::runtime_error(std::format(
                "OpenSCManager failed with err {:#016Lx}", GetLastError()));
        }

        // Install the service into SCM by calling CreateService
        schService = CreateService(
            schSCManager,          // SCManager database
            SERVICE_NAMEW,         // Name of service
            SERVICE_DISPLAY_NAMEW, // Name to display
            SERVICE_QUERY_STATUS | READ_CONTROL | WRITE_DAC, // Desired access
            SERVICE_WIN32_OWN_PROCESS,                       // Service type
            SERVICE_START_TYPE,   // Service start type
            SERVICE_ERROR_NORMAL, // Error control type
            szPath,               // Service's binary
            NULL,                 // No load ordering group
            NULL,                 // No tag identifier
            SERVICE_DEPENDENCIES, // Dependencies
            SERVICE_ACCOUNT,      // Service running account
            SERVICE_PASSWORD      // Password of the account
        );

        if (schService == NULL)
        {
            throw std::runtime_error(std::format(
                "CreateService failed with err {:#016Lx}", GetLastError()));
        }

        if (!GiveLoggedInUserStartStopAccess(schSCManager, schService))
        {
            // Try to uninstall. If it fails, we're already in an error state so
            // just leave it alone.
            ServiceManager::uninstallService();
            throw std::runtime_error("Failed to give current user start and "
                                     "stop permissions for the service");
        }

        LOGGER().info("Service {} is installed", SERVICE_NAME);
    }
    catch (const std::runtime_error& roError)
    {
        LOGGER().error("Error installing the service: {}", roError.what());
        std::cout << std::format("Failed to install the service: {}\n",
                                 roError.what());
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
            throw std::runtime_error(std::format(
                "OpenSCManager failed with err {:#016Lx}", GetLastError()));
        }

        // Open the service with delete, stop, and query status permissions
        schService = OpenService(schSCManager, SERVICE_NAMEW,
                                 SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
        if (schService == NULL)
        {
            throw std::runtime_error(std::format(
                "OpenService failed with err {:#016Lx}", GetLastError()));
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
            throw std::runtime_error(std::format(
                "DeleteService failed with err {:#016Lx}", GetLastError()));
        }

        LOGGER().info("Service {} has been removed", SERVICE_NAME);
    }
    catch (const std::runtime_error& roError)
    {
        LOGGER().error("Error uninstalling the service: {}", roError.what());
        std::cout << std::format("Failed to uninstall the service: {}\n",
                                 roError.what());
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
            throw std::runtime_error(std::format(
                "OpenSCManager failed with err {:#016Lx}", GetLastError()));
        }

        // Open the service with read, start, and query status permissions
        schService =
            OpenService(schSCManager, SERVICE_NAMEW,
                        SERVICE_START | SERVICE_QUERY_STATUS | READ_CONTROL);
        if (schService == NULL)
        {
            throw std::runtime_error(std::format(
                "OpenService failed with err {:#016Lx}", GetLastError()));
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
            auto dwWaitTime = std::clamp(ssSvcStatus.dwWaitHint / 10,
                                         (DWORD) 1000, (DWORD) 10000);

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
                if (GetTickCount64() - dwStartTickCount >
                    ssSvcStatus.dwWaitHint)
                {
                    throw std::runtime_error(
                        "Timeout waiting for service to stop");
                }
            }
        }

        // Attempt to start the service
        if (!StartService(schService, 0, NULL))
        {
            throw std::runtime_error(std::format(
                "StartService failed with err {:#016Lx}", GetLastError()));
        }

        LOGGER().info("Service {} starting...", SERVICE_NAME);
        std::cout << std::format("Service {} starting...\n", SERVICE_NAME);

        GetServiceStatusOrThrow(schService, &ssSvcStatus);

        dwStartTickCount = GetTickCount64();
        dwOldCheckPoint  = (ULONGLONG) ssSvcStatus.dwCheckPoint;

        while (ssSvcStatus.dwCurrentState == SERVICE_START_PENDING)
        {
            auto dwWaitTime =
                std::clamp(ssSvcStatus.dwWaitHint, (DWORD) 1000, (DWORD) 10000);

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
                if (GetTickCount64() - dwStartTickCount >
                    ssSvcStatus.dwWaitHint)
                {
                    throw std::runtime_error(
                        "Timeout waiting for service to start");
                }
            }
        }

        result = ssSvcStatus.dwCurrentState == SERVICE_RUNNING;

        LOGGER().info("Service {} is running: {}", SERVICE_NAME, result);
    }
    catch (const std::runtime_error& roError)
    {
        LOGGER().error("Error starting the service: {}", roError.what());
        std::cout << std::format("Failed to start the service: {}\n",
                                 roError.what());
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
            throw std::runtime_error(std::format(
                "OpenSCManager failed with err {:#016Lx}", GetLastError()));
        }

        // Open the service with delete, stop, and query status permissions
        schService = OpenService(schSCManager, SERVICE_NAMEW,
                                 SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
        if (schService == NULL)
        {
            throw std::runtime_error(std::format(
                "OpenService failed with err {:#016Lx}", GetLastError()));
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
        std::cout << std::format("Failed to stop the service: {}\n",
                                 roError.what());
        result = false;
    }

    FINISHED("Stopped service: {}", result);
    return result;
}

} // namespace ServiceManager