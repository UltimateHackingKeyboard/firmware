#include "module/init_clock.h"
#include "init_peripherals.h"
#include "bootloader.h"
#include <stdio.h>
#include "module/key_scanner.h"
#include "module.h"
#include "uart.h"

DEFINE_BOOTLOADER_CONFIG_AREA(I2C_ADDRESS_MODULE_BOOTLOADER)


int main(void)
{
    InitClock();
    InitPeripherals();
    Module_Init();
    InitKeyScanner();

    while (1) {
        Module_Loop();

        if (MODULE_OVER_UART) {
            ModuleUart_Loop();
        }

        __WFI();
    }
}

// https://stackoverflow.com/questions/73742774/gcc-arm-none-eabi-11-3-is-not-implemented-and-will-always-fail
#if 1
void _exit(int n)
{
    while (1);
}
int _close(int n)
{
    return -1;
}
int _lseek(int n, int m, int l)
{
    return 0;
}
int _read(int n, char*p, int m)
{
    return 0;
}
int _write(int n, char *p, int m)
{
    return 0;
}
struct stat;
int _fstat(int n, struct stat *st)
{
    return 0;
}
int _getpid(int n)
{
    return 1;
}
int _isatty(int n)
{
    return 1;
}
int _kill (int n, int m)
{
    while (1);
}
#endif
