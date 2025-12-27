#ifndef __MODULE_H__
#define __MODULE_H__

// Includes:

    #include "module/module_api.h"
    #include "key_vector.h"
    #include "slave_protocol.h"
    #include <stdint.h>

// Macros:

    #define TRACKPOINT_VERSION 1
    #define MANUAL_RUN false

    #define I2C_ADDRESS_MODULE_FIRMWARE I2C_ADDRESS_RIGHT_MODULE_FIRMWARE
    #define I2C_ADDRESS_MODULE_BOOTLOADER I2C_ADDRESS_RIGHT_MODULE_BOOTLOADER

    #define MODULE_PROTOCOL_VERSION 1
    #define MODULE_ID ModuleId_TrackpointRight
    #define MODULE_KEY_COUNT KEYBOARD_VECTOR_ITEMS_NUM
    #define MODULE_POINTER_COUNT 1

    #define TEST_LED_GPIO  GPIOA
    #define TEST_LED_PORT  PORTA
    #define TEST_LED_CLOCK kCLOCK_PortA
    #define TEST_LED_PIN   5

    #define PS2_DATA_GPIO  GPIOB
    #define PS2_DATA_PORT  PORTB
    #define PS2_DATA_IRQ   PORTB_IRQn
    #define PS2_DATA_CLOCK kCLOCK_PortB
    #define PS2_DATA_PIN   1

    #define PS2_CLOCK_GPIO  GPIOB
    #define PS2_CLOCK_PORT  PORTB
    #define PS2_CLOCK_IRQ   PORTB_IRQn
    #define PS2_CLOCK_CLOCK kCLOCK_PortB
    #define PS2_CLOCK_PIN   0
    #define PS2_CLOCK_IRQ_HANDLER PORTB_IRQHandler

    #define TP_RST_GPIO  GPIOA
    #define TP_RST_PORT  PORTA
    #define TP_RST_CLOCK kCLOCK_PortA
    #define TP_RST_PIN   7

    #define KEY_ARRAY_TYPE KEY_ARRAY_TYPE_VECTOR
    #define KEYBOARD_VECTOR_ITEMS_NUM 2

// Variables:

    extern key_vector_t KeyVector;
    extern pointer_delta_t PointerDelta;

// Functions:

    void Module_Init(void);
    void Module_Loop(void);
    void Module_OnScan(void);
    void Module_ModuleSpecificCommand(module_specific_command_t command);

#endif
