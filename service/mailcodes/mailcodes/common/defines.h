#pragma once

// Internal name of the service
#define SERVICE_NAME             L"MailcodesService"


// Displayed name of the service
#define SERVICE_DISPLAY_NAME     L"Mailcodes Service"


// Service start options.
#define SERVICE_START_TYPE       SERVICE_AUTO_START


// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     L""


// The name of the account under which the service should run
#define SERVICE_ACCOUNT          L"NT AUTHORITY\\LocalService"


// The password to the service account name
#define SERVICE_PASSWORD         NULL