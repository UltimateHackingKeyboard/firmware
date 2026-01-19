#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>
    #include "usb_interfaces/usb_interface_generic_hid.h"
    #include "buffer.h"
#ifdef __ZEPHYR__
    #include <zephyr/bluetooth/bluetooth.h>
#else
    #include "fsl_common.h"
#endif

// Macros:

    #define USB_STATUS_CODE_SIZE 1

    #define GetUsbRxBufferUint8(OFFSET) (GetBufferUint8(GenericHidOutBuffer, OFFSET))
    #define GetUsbRxBufferUint16(OFFSET) (GetBufferUint16(GenericHidOutBuffer, OFFSET))
    #define GetUsbRxBufferUint32(OFFSET) (GetBufferUint32(GenericHidOutBuffer, OFFSET))

    #define SetUsbTxBufferUint8(OFFSET, VALUE) (SetBufferUint8(GenericHidInBuffer, OFFSET, VALUE))
    #define SetUsbTxBufferUint16(OFFSET, VALUE) (SetBufferUint16(GenericHidInBuffer, OFFSET, VALUE))
    #define SetUsbTxBufferUint32(OFFSET, VALUE) (SetBufferUint32(GenericHidInBuffer, OFFSET, VALUE))

    #define GetUsbRxBufferBleAddress(OFFSET) (GetBufferBleAddress(GenericHidOutBuffer, OFFSET))
    #define SetUsbTxBufferBleAddress(OFFSET, VALUE) (SetBufferBleAddress(GenericHidInBuffer, OFFSET, VALUE))

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
        UsbCommandId_LaunchStorageTransfer    = 0x08,

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

        UsbCommandId_DrawOled                 = 0x15,

        UsbCommandId_GetPairingData           = 0x16,
        UsbCommandId_SetPairingData           = 0x17,
        UsbCommandId_PairPeripheral           = 0x18,
        UsbCommandId_PairCentral              = 0x19,
        UsbCommandId_UnpairAll                = 0x1a,
        UsbCommandId_IsPaired                 = 0x1b,
        UsbCommandId_EnterPairingMode         = 0x1c,
        UsbCommandId_EraseBleSettings         = 0x1d,
        UsbCommandId_ExecShellCommand         = 0x1e,
        UsbCommandId_ReadOled                 = 0x1f,
        UsbCommandId_SetUhk60LedState         = 0x20,
    } usb_command_id_t;

    typedef enum {
        UsbVariable_TestSwitches              = 0x00,
        UsbVariable_TestUsbStack              = 0x01,
        UsbVariable_DebounceTimePress         = 0x02,
        UsbVariable_DebounceTimeRelease       = 0x03,
        UsbVariable_UsbReportSemaphore        = 0x04,
        UsbVariable_StatusBuffer              = 0x05,
        UsbVariable_LedAudioRegisters         = 0x06,
        UsbVariable_ShellEnabled              = 0x07,
        UsbVariable_ShellBuffer               = 0x08,
        UsbVariable_FirmwareVersionCheckEnabled = 0x09,
        UsbVariable_LedOverride               = 0x0a,
    } usb_variable_id_t;

    typedef enum {
        UsbStatusCode_Success        = 0,
        UsbStatusCode_InvalidCommand = 1,
        UsbStatusCode_Busy = 2,
    } usb_status_code_general_t;

// Variables:

    extern uint32_t LastUsbGetKeyboardStateRequestTimestamp;

// Functions:

#ifdef __ZEPHYR__
    bt_addr_le_t GetBufferBleAddress(const uint8_t *GenericHidOutBuffer, uint32_t offset);
    void SetBufferBleAddress(uint8_t *GenericHidInBuffer, uint32_t offset, const bt_addr_le_t* addr);
#endif
    void UsbProtocolHandler(uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
