#include "main.h"
#include "slave/init_clock.h"
#include "init_peripherals.h"
#include "bootloader.h"
#include <stdio.h>
#include "key_scanner.h"

DEFINE_BOOTLOADER_CONFIG_AREA(I2C_ADDRESS_LEFT_MODULE_BOOTLOADER)

key_matrix_t keyMatrix = {
    .colNum = KEYBOARD_MATRIX_COLS_NUM,
    .rowNum = KEYBOARD_MATRIX_ROWS_NUM,
    .cols = (key_matrix_pin_t[]) {
        {PORTB, GPIOB, kCLOCK_PortB, 11},
        {PORTA, GPIOA, kCLOCK_PortA,  6},
    },
    .rows = (key_matrix_pin_t[]) {
        {PORTB, GPIOB, kCLOCK_PortB,  7},
        {PORTB, GPIOB, kCLOCK_PortB, 10},
    }
};

int main(void)
{
    InitClock();
    InitPeripherals();
    KeyMatrix_Init(&keyMatrix);
    InitKeyScanner();

    while (1) {
        __WFI();
    }
}
