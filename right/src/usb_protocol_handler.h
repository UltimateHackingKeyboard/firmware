#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

#include "usb_interfaces/usb_interface_generic_hid.h"

// Macros:

    #define PROTOCOL_RESPONSE_SUCCESS       0
    #define PROTOCOL_RESPONSE_GENERIC_ERROR 1

    #define WRITE_LED_DRIVER_RESPONSE_INVALID_ADDRESS      1
    #define WRITE_LED_DRIVER_RESPONSE_INVALID_PAYLOAD_SIZE 2
    #define WRITE_EEPROM_RESPONSE_INVALID_PAYLOAD_SIZE 1
    #define UPLOAD_CONFIG_INVALID_PAYLOAD_SIZE 1

// Typedefs:

    typedef enum {
        UsbCommand_GetSystemProperty    =  0,
        UsbCommand_Reenumerate          =  1,
        UsbCommand_SetTestLed           =  2,
        UsbCommand_WriteLedDriver       =  3,
        UsbCommand_WriteEeprom          =  5,
        UsbCommand_ReadEeprom           =  6,
        UsbCommand_ReadMergeSensor      =  7,
        UsbCommand_UploadConfig         =  8,
        UsbCommand_ApplyConfig          =  9,
        UsbCommand_SetLedPwm            = 10,
        UsbCommand_GetAdcValue          = 11,
        UsbCommand_LaunchEepromTransfer = 12,
    } usb_command_t;

    typedef enum {
        SystemProperty_UsbProtocolVersionId    = 0,
        SystemProperty_BridgeProtocolVersionId = 1,
        SystemProperty_DataModelVersionId      = 2,
        SystemProperty_FirmwareVersionId       = 3,
    } system_property_t;

// Functions:

    extern void usbProtocolHandler();

#endif
