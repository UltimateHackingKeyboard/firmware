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
        {PORTB, GPIOB, kCLOCK_PortB, 10}, // top key
        {PORTA, GPIOA, kCLOCK_PortA,  6}, // left key
        {PORTB, GPIOB, kCLOCK_PortB,  2}, // right key
        {PORTA, GPIOA, kCLOCK_PortA,  5}, // left microswitch
        {PORTB, GPIOB, kCLOCK_PortB,  7}, // trackball microswitch
        {PORTA, GPIOA, kCLOCK_PortA,  8}, // right microswitch
    },
};

int main(void)
{
    InitClock();
    InitPeripherals();
    KeyVector_Init(&keyVector);
    InitKeyScanner();

    while (1) {
        BlackberryTrackball_Update();
        __WFI();
    }
}
