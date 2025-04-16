#ifndef __PROXY_LOG_BACKEND_H__
#define __PROXY_LOG_BACKEND_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Macros:

// Typedefs:

// Variables:


    extern bool ProxyLog_IsAttached;
    extern bool ProxyLog_HasLog;

// Functions:

    void ProxyLog_SetAttached(bool attached);
    void InitProxyLogBackend(void);
    uint16_t ProxyLog_ConsumeLog(uint8_t* outBuf, uint16_t outBufSize);

#endif /* __MACRO_SET_COMMAND_H__ */
