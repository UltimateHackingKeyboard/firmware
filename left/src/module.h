#ifndef __MODULE_H__
#define __MODULE_H__

// Includes:

    #include "key_matrix.h"

// Macros:

    #define I2C_ADDRESS_MODULE_FIRMWARE I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE
    #define I2C_ADDRESS_MODULE_BOOTLOADER I2C_ADDRESS_LEFT_KEYBOARD_HALF_BOOTLOADER

    #define MODULE_PROTOCOL_VERSION 1
    #define MODULE_ID ModuleId_LeftKeyboardHalf
    #define MODULE_KEY_COUNT (KEYBOARD_MATRIX_ROWS_NUM * KEYBOARD_MATRIX_COLS_NUM)
    #define MODULE_POINTER_COUNT 0

    #define TEST_LED_GPIO  GPIOB
    #define TEST_LED_PORT  PORTB
    #define TEST_LED_CLOCK kCLOCK_PortB
    #define TEST_LED_PIN   13

    #define KEYBOARD_MATRIX_COLS_NUM 7
    #define KEYBOARD_MATRIX_ROWS_NUM 5

// Variables:

    extern key_matrix_t keyMatrix;

#endif
