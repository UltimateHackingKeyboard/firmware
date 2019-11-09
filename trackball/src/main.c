#include "main.h"
#include "slave/init_clock.h"
#include "init_peripherals.h"
#include "bootloader.h"
#include <stdio.h>
#include "key_scanner.h"
#include "module.h"
#include "blackberry_trackball.h"

DEFINE_BOOTLOADER_CONFIG_AREA(I2C_ADDRESS_MODULE_BOOTLOADER)

key_vector_t keyVector = {
    .itemNum = KEYBOARD_VECTOR_ITEMS_NUM,
    .items = (key_vector_pin_t[]) {
        {PORTA, GPIOA, kCLOCK_PortA, 3}, // left button
        {PORTA, GPIOA, kCLOCK_PortA, 5}, // right button
    },
};

int main(void)
{
    InitClock();
    InitPeripherals();
    KeyVector_Init(&keyVector);
    InitKeyScanner();

    while (1) {
//        BlackberryTrackball_Update();
        __WFI();
    }
}
