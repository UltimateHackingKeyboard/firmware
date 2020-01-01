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
