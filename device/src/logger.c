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

#ifdef DEVICE_HAS_OLED
#include "keyboard/oled/widgets/console_widget.h"
#endif

void Uart_LogConstant(const char* buffer) {
    printk("%s\n", buffer);
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
#ifdef DEVICE_HAS_OLED
    Oled_LogConstant(buffer);
#endif
}

void LogBt(const char *fmt, ...) {
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);

    Messenger_Send(DeviceId_Uhk80_Right, MessageId_Log, buffer, strlen(buffer)+1);
}
