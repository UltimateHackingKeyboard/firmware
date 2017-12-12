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
        UsbCommandId_JumpToModuleBootloader     = 0x02, // was 0x12
        UsbCommandId_SendKbootCommandToModule   = 0x03, // was 0x13

        UsbCommandId_ReadConfig                 = 0x04, // was 0x0d and 0x0f
        UsbCommandId_WriteHardwareConfig        = 0x0E,
        UsbCommandId_WriteUserConfig            = 0x08,
        UsbCommandId_ApplyConfig                = 0x09,
        UsbCommandId_LaunchEepromTransferLegacy = 0x0C,

        UsbCommandId_GetKeyboardState           = 0x10,
        UsbCommandId_SetTestLed                 = 0x14, //was 0x02
        UsbCommandId_GetDebugBuffer             = 0x11,
        UsbCommandId_GetAdcValue                = 0x0B,
        UsbCommandId_SetLedPwmBrightness        = 0x0A,
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
