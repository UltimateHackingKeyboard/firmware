#ifndef __USB_COMMAND_GET_DEBUG_INFO_H__
#define __USB_COMMAND_GET_DEBUG_INFO_H__

// Includes:

    #include "usb_interfaces/usb_interface_generic_hid.h"

// Variables:

    extern uint8_t UsbDebugInfo[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

// Functions:

    void UsbCommand_GetDebugInfo(void);

#endif
