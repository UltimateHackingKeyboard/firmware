#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "logger.h"
#include "shell.h"
#include "keyboard/uart.h"
#include "nus_client.h"
#include "nus_server.h"
#include "device.h"
#include "messenger.h"
#include "legacy/macros/status_buffer.h"

#ifdef DEVICE_HAS_OLED
#include "keyboard/oled/widgets/console_widget.h"
#endif

void Uart_LogConstant(const char* buffer) {
    printk("%s", buffer);
}

void Uart_Log(const char *fmt, ...) {
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);

    Uart_LogConstant(buffer);
}

void Log(const char *fmt, ...) {
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);

    Uart_LogConstant(buffer);
#if DEVICE_HAS_OLED
    Oled_LogConstant(buffer);
#endif
}


void LogRight(log_target_t logMask, const char *fmt, ...) {
    va_list myargs;
    va_start(myargs, fmt);
    static char buffer[64];
    vsprintf(buffer, fmt, myargs);
    buffer[63]=0;

    // on right, log according to logMask
    if (DEVICE_ID == DeviceId_Uhk80_Right) {
        if (logMask & LogTarget_Oled) {
            Oled_LogConstant(buffer);
        }
        if (logMask & LogTarget_Uart) {
            Uart_LogConstant(buffer);
        }
        if (logMask & LogTarget_ErrorBuffer) {
            Macros_ReportPrintf(NULL, "%s", buffer);
        }
    }

    if (DEVICE_ID == DeviceId_Uhk80_Left) {
        Messenger_Send2(DeviceId_Uhk80_Right, MessageId_Log, logMask, buffer, strlen(buffer)+1);
    }
}

