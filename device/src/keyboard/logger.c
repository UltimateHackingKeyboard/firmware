#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/logger.h"
#include "shell.h"
#include "keyboard/uart.h"
#include <zephyr/drivers/uart.h>
#include "bt_central_uart.h"
#include "bt_peripheral_uart.h"
#include "device.h"
#include "oled/widgets/console_widget.h"

void Uart_LogConstant(const char* buffer)
{
#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_LEFT
    SendPeripheralUart(buffer, strlen(buffer)+1);
#elif CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT
    SendCentralUart(buffer, strlen(buffer)+1);
#endif
    printk("%s\n", buffer);
    for (uint8_t i=0; i<strlen(buffer); i++) {
        uart_poll_out(uart_dev, buffer[i]);
    }
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
