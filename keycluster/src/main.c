#include "module/init_clock.h"
#include "init_peripherals.h"
#include "bootloader.h"
#include <stdio.h>
#include "key_scanner.h"
#include "module.h"
#include "blackberry_trackball.h"

DEFINE_BOOTLOADER_CONFIG_AREA(I2C_ADDRESS_MODULE_BOOTLOADER)

int main(void)
{
    InitClock();
    InitPeripherals();
    Module_Init();
    InitKeyScanner();

    while (1) {
        BlackberryTrackball_Update();
        __WFI();
    }
}
