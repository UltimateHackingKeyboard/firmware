#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

    #include "fsl_common.h"
    #include "usb_interfaces/usb_interface_generic_hid.h"

// Macros:

    #define USB_STATUS_CODE_SIZE 1

// Typedefs:

    typedef enum {
        UsbCommandId_GetDeviceProperty        = 0x00,

        UsbCommandId_Reenumerate              = 0x01,
        UsbCommandId_JumpToModuleBootloader   = 0x02,
        UsbCommandId_SendKbootCommandToModule = 0x03,

        UsbCommandId_ReadConfig               = 0x04,
        UsbCommandId_WriteHardwareConfig      = 0x05,
        UsbCommandId_WriteStagingUserConfig   = 0x06,
        UsbCommandId_ApplyConfig              = 0x07,
        UsbCommandId_LaunchEepromTransfer     = 0x08,

        UsbCommandId_GetDeviceState           = 0x09,
        UsbCommandId_SetTestLed               = 0x0a,
        UsbCommandId_GetDebugBuffer           = 0x0b,
        UsbCommandId_GetAdcValue              = 0x0c,
        UsbCommandId_SetLedPwmBrightness      = 0x0d,
        UsbCommandId_GetModuleProperty        = 0x0e,
        UsbCommandId_GetSlaveI2cErrors        = 0x0f,
        UsbCommandId_SetI2cBaudRate           = 0x10,
        UsbCommandId_SwitchKeymap             = 0x11,
        UsbCommandId_GetVariable              = 0x12,
        UsbCommandId_SetVariable              = 0x13,
        UsbCommandId_ExecMacroCommand         = 0x14,
    } usb_command_id_t;

    typedef enum {
        UsbVariable_TestSwitches,
        UsbVariable_TestUsbStack,
        UsbVariable_DebounceTimePress,
        UsbVariable_DebounceTimeRelease,
        UsbVariable_UsbReportSemaphore,
    } usb_variable_id_t;

    typedef enum {
        UsbStatusCode_Success        = 0,
        UsbStatusCode_InvalidCommand = 1,
    } usb_status_code_general_t;

// Variables:

    extern uint32_t LastUsbGetKeyboardStateRequestTimestamp;

// Functions:

    void UsbProtocolHandler(void);

    uint8_t GetUsbRxBufferUint8(uint32_t offset);
    uint16_t GetUsbRxBufferUint16(uint32_t offset);
    uint32_t GetUsbRxBufferUint32(uint32_t offset);

    void SetUsbTxBufferUint8(uint32_t offset, uint8_t value);
    void SetUsbTxBufferUint16(uint32_t offset, uint16_t value);
    void SetUsbTxBufferUint32(uint32_t offset, uint32_t value);

#endif
