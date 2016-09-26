#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

#include "usb_interface_generic_hid.h"

// Macros:

    #define USB_COMMAND_GET_PROTOCOL_VERSION 0
    #define USB_COMMAND_JUMP_TO_BOOTLOADER   1
    #define USB_COMMAND_TEST_LED             2
    #define USB_COMMAND_WRITE_LED_DRIVER     3
    #define USB_COMMAND_READ_LED_DRIVER      4

// Functions:

    extern void UsbProtocolHandler();

#endif
