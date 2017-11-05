#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

    #include "fsl_common.h"
    #include "usb_interfaces/usb_interface_generic_hid.h"

// Macros:

    #define GET_USB_BUFFER_UINT8(offset)  (*(uint8_t*)(GenericHidInBuffer+(offset)))
    #define GET_USB_BUFFER_UINT16(offset) (*(uint16_t*)(GenericHidInBuffer+(offset)))
    #define GET_USB_BUFFER_UINT32(offset) (*(uint32_t*)(GenericHidInBuffer+(offset)))

    #define SET_USB_BUFFER_UINT8(offset, value) (*(uint8_t*)(GenericHidOutBuffer+(offset)) = (value))
    #define SET_USB_BUFFER_UINT16(offset, value) (*(uint16_t*)(GenericHidOutBuffer+(offset)) = (value))
    #define SET_USB_BUFFER_UINT32(offset, value) (*(uint32_t*)(GenericHidOutBuffer+(offset)) = (value))

// Typedefs:

    typedef enum {
        UsbCommandId_GetProperty                =  0,
        UsbCommandId_Reenumerate                =  1,
        UsbCommandId_SetTestLed                 =  2,
        UsbCommandId_WriteUserConfiguration     =  8,
        UsbCommandId_ApplyConfig                =  9,
        UsbCommandId_SetLedPwmBrightness        = 10,
        UsbCommandId_GetAdcValue                = 11,
        UsbCommandId_LaunchEepromTransferLegacy = 12,
        UsbCommandId_ReadHardwareConfiguration  = 13,
        UsbCommandId_WriteHardwareConfiguration = 14,
        UsbCommandId_ReadUserConfiguration      = 15,
        UsbCommandId_GetKeyboardState           = 16,
        UsbCommandId_GetDebugInfo               = 17,
        UsbCommandId_JumpToSlaveBootloader      = 18,
        UsbCommandId_SendKbootCommand           = 19,
    } usb_command_id_t;

    typedef enum {
        UsbStatusCode_Success        = 0,
        UsbStatusCode_InvalidCommand = 1,
    } usb_status_code_general_t;

// Functions:

    void UsbProtocolHandler(void);

#endif
