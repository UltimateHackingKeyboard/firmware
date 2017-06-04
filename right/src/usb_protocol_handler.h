#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

#include "usb_interfaces/usb_interface_generic_hid.h"

// Macros:

    #define PROTOCOL_RESPONSE_SUCCESS       0
    #define PROTOCOL_RESPONSE_GENERIC_ERROR 1

    #define USB_COMMAND_GET_SYSTEM_PROPERTY 0
        #define SYSTEM_PROPERTY_USB_PROTOCOL_VERSION_ID      0
        #define SYSTEM_PROPERTY_BRIDGE_PROTOCOL_VERSION_ID   1
        #define SYSTEM_PROPERTY_DATA_MODEL_VERSION_ID        2
        #define SYSTEM_PROPERTY_FIRMWARE_VERSION_ID          3
    #define USB_COMMAND_REENUMERATE  1
    #define USB_COMMAND_SET_TEST_LED            2
    #define USB_COMMAND_WRITE_LED_DRIVER    3
        #define WRITE_LED_DRIVER_RESPONSE_INVALID_ADDRESS      1
        #define WRITE_LED_DRIVER_RESPONSE_INVALID_PAYLOAD_SIZE 2
    #define USB_COMMAND_WRITE_EEPROM        5
        #define WRITE_EEPROM_RESPONSE_INVALID_PAYLOAD_SIZE 1
    #define USB_COMMAND_READ_EEPROM     6
    #define USB_COMMAND_READ_MERGE_SENSOR   7
    #define USB_COMMAND_UPLOAD_CONFIG       8
        #define UPLOAD_CONFIG_INVALID_PAYLOAD_SIZE 1
    #define USB_COMMAND_APPLY_CONFIG        9
    #define USB_COMMAND_SET_LED_PWM        10
    #define USB_COMMAND_GET_ADC_VALUE      11

// Functions:

    extern void usbProtocolHandler();

#endif
