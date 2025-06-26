#include "module/init_clock.h"
#include "init_peripherals.h"
#include "bootloader.h"
#include <stdio.h>
#include "module/key_scanner.h"
#include "module.h"

DEFINE_BOOTLOADER_CONFIG_AREA(I2C_ADDRESS_MODULE_BOOTLOADER)

int main(void)
{
    InitClock();
    InitPeripherals();
    Module_Init();
    InitKeyScanner();

    while (1) {
        Module_Loop();
        __WFI();
    }
}

// https://stackoverflow.com/questions/73742774/gcc-arm-none-eabi-11-3-is-not-implemented-and-will-always-fail
#if 1 // using nano vs nosys
void _exit(int)
{
    while (1);
}
#else
int _close(int)
{
    return -1;
}
int _lseek(int, int, int)
{
    return 0;
}
int _read(int, char*, int)
{
    return 0;
}
int _write(int, char *, int)
{
    return 0;
}
#endif
