#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>
    #include "usb_interfaces/usb_interface_generic_hid.h"
#ifdef __ZEPHYR__
    #include <zephyr/bluetooth/bluetooth.h>
#else
    #include "fsl_common.h"
#endif

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
    } usb_command_id_t;

    typedef enum {
        UsbVariable_TestSwitches,
        UsbVariable_TestUsbStack,
        UsbVariable_DebounceTimePress,
        UsbVariable_DebounceTimeRelease,
        UsbVariable_UsbReportSemaphore,
        UsbVariable_StatusBuffer,
    } usb_variable_id_t;

    typedef enum {
        UsbStatusCode_Success        = 0,
        UsbStatusCode_InvalidCommand = 1,
    } usb_status_code_general_t;

// Variables:

    extern uint32_t LastUsbGetKeyboardStateRequestTimestamp;

// Functions:

#ifdef __ZEPHYR__
    extern bool CommandProtocolTx(const uint8_t* data, size_t size);

    void SetUsbTxBufferBleAddress(uint32_t offset, const bt_addr_le_t* addr);
    extern bt_addr_le_t GetUsbRxBufferBleAddress(uint32_t offset);
#endif
    void UsbProtocolHandler(void);

    uint8_t GetUsbRxBufferUint8(uint32_t offset);
    uint16_t GetUsbRxBufferUint16(uint32_t offset);
    uint32_t GetUsbRxBufferUint32(uint32_t offset);

    void SetUsbTxBufferUint8(uint32_t offset, uint8_t value);
    void SetUsbTxBufferUint16(uint32_t offset, uint16_t value);
    void SetUsbTxBufferUint32(uint32_t offset, uint32_t value);

#endif
