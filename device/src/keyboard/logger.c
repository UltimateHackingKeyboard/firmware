#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/logger.h"
#include "shell.h"
#include "keyboard/uart.h"
#include "bt_central_uart.h"
#include "bt_peripheral_uart.h"
#include "device.h"
#include "oled/widgets/console_widget.h"

void Uart_LogConstant(const char* buffer)
{
#if DEVICE_IS_UHK80_LEFT
    SendPeripheralUart(buffer, strlen(buffer)+1);
#elif DEVICE_IS_UHK80_RIGHT
    SendCentralUart(buffer, strlen(buffer)+1);
#endif
    printk("%s\n", buffer);
}

void Uart_Log(const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);

    Uart_LogConstant(buffer);
}

void Log(const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);

    Uart_LogConstant(buffer);
    Oled_LogConstant(buffer);
}
