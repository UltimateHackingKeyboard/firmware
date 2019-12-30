#ifndef __MODULE_H__
#define __MODULE_H__

// Includes:

    #include "module/module.h"
    #include "key_vector.h"
    #include "slave_protocol.h"

// Macros:

    #define I2C_ADDRESS_MODULE_FIRMWARE I2C_ADDRESS_LEFT_MODULE_FIRMWARE
    #define I2C_ADDRESS_MODULE_BOOTLOADER I2C_ADDRESS_LEFT_MODULE_BOOTLOADER

    #define MODULE_PROTOCOL_VERSION 1
    #define MODULE_ID ModuleId_KeyClusterLeft
    #define MODULE_KEY_COUNT KEYBOARD_VECTOR_ITEMS_NUM
    #define MODULE_POINTER_COUNT 1

    #define TEST_LED_GPIO  GPIOB
    #define TEST_LED_PORT  PORTB
    #define TEST_LED_CLOCK kCLOCK_PortB
    #define TEST_LED_PIN   1

    #define KEY_ARRAY_TYPE KEY_ARRAY_TYPE_VECTOR
    #define KEYBOARD_VECTOR_ITEMS_NUM 6

// Variables:

    extern key_vector_t keyVector;
    extern pointer_delta_t PointerDelta;

// Functions:

    void Module_Init(void);
    void Module_Loop(void);

#endif
