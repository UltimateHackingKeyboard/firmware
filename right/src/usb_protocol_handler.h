#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

    #include "fsl_common.h"
    #include "usb_interfaces/usb_interface_generic_hid.h"

// Macros:

    #define USB_STATUS_CODE_SIZE 1

// Typedefs:

    typedef enum {
        UsbCommandId_GetProperty                =  0,
        UsbCommandId_Reenumerate                =  1,
        UsbCommandId_SetTestLed                 =  2,
        UsbCommandId_WriteUserConfig            =  8,
        UsbCommandId_ApplyConfig                =  9,
        UsbCommandId_SetLedPwmBrightness        = 10,
        UsbCommandId_GetAdcValue                = 11,
        UsbCommandId_LaunchEepromTransferLegacy = 12,
        UsbCommandId_ReadHardwareConfig         = 13,
        UsbCommandId_WriteHardwareConfig        = 14,
        UsbCommandId_ReadUserConfig             = 15,
        UsbCommandId_GetKeyboardState           = 16,
        UsbCommandId_GetDebugBuffer             = 17,
        UsbCommandId_JumpToModuleBootloader     = 18,
        UsbCommandId_SendKbootCommandToModule   = 19,
    } usb_command_id_t;

    typedef enum {
        UsbStatusCode_Success        = 0,
        UsbStatusCode_InvalidCommand = 1,
    } usb_status_code_general_t;

// Functions:

    void UsbProtocolHandler(void);

    uint8_t GetUsbRxBufferUint8(uint32_t offset);
    uint16_t GetUsbRxBufferUint16(uint32_t offset);
    uint32_t GetUsbRxBufferUint32(uint32_t offset);

    void SetUsbTxBufferUint8(uint32_t offset, uint8_t value);
    void SetUsbTxBufferUint16(uint32_t offset, uint16_t value);
    void SetUsbTxBufferUint32(uint32_t offset, uint32_t value);

#endif
