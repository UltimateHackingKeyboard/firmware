#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

    #include "fsl_common.h"
    #include "usb_interfaces/usb_interface_generic_hid.h"

// Macros:

    #define USB_STATUS_CODE_SIZE 1

// Typedefs:

    typedef enum {
        UsbCommandId_GetProperty                = 0x00,
        UsbCommandId_Reenumerate                = 0x01,
        UsbCommandId_SetTestLed                 = 0x02,
        UsbCommandId_WriteUserConfig            = 0x08,
        UsbCommandId_ApplyConfig                = 0x09,
        UsbCommandId_SetLedPwmBrightness        = 0x0A,
        UsbCommandId_GetAdcValue                = 0x0B,
        UsbCommandId_LaunchEepromTransferLegacy = 0x0C,
        UsbCommandId_ReadHardwareConfig         = 0x0D,
        UsbCommandId_WriteHardwareConfig        = 0x0E,
        UsbCommandId_ReadUserConfig             = 0x0F,
        UsbCommandId_GetKeyboardState           = 0x10,
        UsbCommandId_GetDebugBuffer             = 0x11,
        UsbCommandId_JumpToModuleBootloader     = 0x12,
        UsbCommandId_SendKbootCommandToModule   = 0x13,
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
