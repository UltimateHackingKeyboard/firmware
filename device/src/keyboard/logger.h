#ifndef __LOGGER_H__
#define __LOGGER_H__

// Includes:

    #include "device.h"
    #include "stdint.h"

// Variables:

// Functions:

void Uart_LogConstant(const char* buffer);
void Uart_Log(const char *fmt, ...);
void Log(const char *fmt, ...);

#endif // __LOGGER_H__
