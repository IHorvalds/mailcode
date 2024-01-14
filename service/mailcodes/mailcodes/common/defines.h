#pragma once

// Internal name of the service
// Both multi-byte and standard char, to avoid conversion boilerplate
#define SERVICE_NAMEW L"MailcodesService"
#define SERVICE_NAME "MailcodesService"

// Displayed name of the service
// Both multi - byte and standard char, to avoid conversion boilerplate
#define SERVICE_DISPLAY_NAMEW L"Mailcodes Service"
#define SERVICE_DISPLAY_NAME "Mailcodes Service"

// Service start options.
#define SERVICE_START_TYPE SERVICE_AUTO_START

// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES L""

// The name of the account under which the service should run
#define SERVICE_ACCOUNT L"NT AUTHORITY\\LocalService"

// The password to the service account name
#define SERVICE_PASSWORD NULL

// AUMI Information for WinToast
#define TOAST_APP_NAME L"Mailcodes"
#define TOAST_APP_COMPANY L"Taylor Labs"
