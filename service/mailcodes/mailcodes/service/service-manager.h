#pragma once

//! @brief Installs the service using the Windows Service Manager
//! @return true if successful, false otherwise
bool installService();

//! @brief Uninstall the service using the Windows Service Manager
//! @return true if successful, false otherwise
bool uninstallService();

//! @brief Starts the service
//! @return true if successful, false otherwise
bool startService();

//! @brief Stops the service
//! @return true if successful, false otherwise
bool stopService();
