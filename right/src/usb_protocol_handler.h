#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

#include "usb_interface_generic_hid.h"

// Macros:

    #define USB_COMMAND_JUMP_TO_BOOTLOADER 0
    #define USB_COMMAND_TEST_LED           1
    #define USB_COMMAND_LED_DRIVER         2

// Functions:

    extern void UsbProtocolHandler();

#endif
