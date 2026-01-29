#ifndef __PROXY_LOG_BACKEND_H__
#define __PROXY_LOG_BACKEND_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Macros:

// Typedefs:

// Variables:

    extern bool ProxyLog_IsAttached;
    extern bool ProxyLog_IsInPanicMode;

// Functions:

    void ProxyLog_SetAttached(bool attached);
    void InitProxyLogBackend(void);

#endif
