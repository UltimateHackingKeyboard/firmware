#ifndef __MODULE_H__
#define __MODULE_H__

// Includes:

    #include "module/module_api.h"
    #include "key_matrix.h"
    #include "module/slave_protocol_handler.h"

// Macros:

    #define I2C_ADDRESS_MODULE_FIRMWARE I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE
    #define I2C_ADDRESS_MODULE_BOOTLOADER I2C_ADDRESS_LEFT_KEYBOARD_HALF_BOOTLOADER

    #define MODULE_PROTOCOL_VERSION 1
    #define MODULE_ID ModuleId_LeftKeyboardHalf
    #define MODULE_KEY_COUNT (KEYBOARD_MATRIX_ROWS_NUM * KEYBOARD_MATRIX_COLS_NUM)
    #define MODULE_POINTER_COUNT 0

    #define LED_DRIVER_UHK60V1_INTB_PORT  PORTB
    #define LED_DRIVER_UHK60V1_INTB_GPIO  GPIOB
    #define LED_DRIVER_UHK60V1_INTB_CLOCK kCLOCK_PortB
    #define LED_DRIVER_UHK60V1_INTB_PIN   2

    #define LED_DRIVER_UHK60V2_IICRST_PORT  PORTB
    #define LED_DRIVER_UHK60V2_IICRST_GPIO  GPIOB
    #define LED_DRIVER_UHK60V2_IICRST_CLOCK kCLOCK_PortB
    #define LED_DRIVER_UHK60V2_IICRST_PIN   2

    #define TEST_LED_GPIO  GPIOB
    #define TEST_LED_PORT  PORTB
    #define TEST_LED_CLOCK kCLOCK_PortB
    #define TEST_LED_PIN   13

    #define KEY_ARRAY_TYPE KEY_ARRAY_TYPE_MATRIX
    #define KEYBOARD_MATRIX_COLS_NUM 7
    #define KEYBOARD_MATRIX_ROWS_NUM 5

// Variables:

    extern key_matrix_t KeyMatrix;
    extern pointer_delta_t PointerDelta;

// Functions:

    void Module_Init(void);
    void Module_Loop(void);
    void Module_OnScan();
    void Module_ModuleSpecificCommand(module_specific_command_t command);

#endif
