#ifndef __USB_COMMAND_GET_DEBUG_BUFFER_H__
#define __USB_COMMAND_GET_DEBUG_BUFFER_H__

// Includes:

    #include "usb_interfaces/usb_interface_generic_hid.h"

// Variables:

    extern uint8_t DebugBuffer[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

// Functions:

    void UsbCommand_GetDebugBuffer(void);

    void SetDebugBufferUint8(uint32_t offset, uint8_t value);
    void SetDebugBufferUint16(uint32_t offset, uint16_t value);
    void SetDebugBufferUint32(uint32_t offset, uint32_t value);

#endif
