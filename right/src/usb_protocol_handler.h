#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

    #include "usb_interfaces/usb_interface_generic_hid.h"

// Typedefs:

    typedef enum {
        UsbCommandId_GetProperty                =  0,
        UsbCommandId_Reenumerate                =  1,
        UsbCommandId_SetTestLed                 =  2,
        UsbCommandId_WriteLedDriver             =  3,
        UsbCommandId_ReadMergeSensor            =  7,
        UsbCommandId_WriteUserConfiguration     =  8,
        UsbCommandId_ApplyConfig                =  9,
        UsbCommandId_SetLedPwm                  = 10,
        UsbCommandId_GetAdcValue                = 11,
        UsbCommandId_LaunchEepromTransferLegacy       = 12,
        UsbCommandId_ReadHardwareConfiguration  = 13,
        UsbCommandId_WriteHardwareConfiguration = 14,
        UsbCommandId_ReadUserConfiguration      = 15,
        UsbCommandId_GetKeyboardState           = 16,
        UsbCommandId_GetDebugInfo               = 17,
        UsbCommandId_JumpToSlaveBootloader      = 18,
        UsbCommandId_SendKbootCommand           = 19,
    } usb_command_id_t;

    typedef enum {
        UsbResponse_Success      = 0,
        UsbResponse_GenericError = 1,
    } usb_response_t;

    typedef enum {
        ConfigTransferResponse_LengthTooLarge    = 1,
        ConfigTransferResponse_BufferOutOfBounds = 2,
    } config_transfer_response_t;

    typedef enum {
        JumpToBootloaderError_InvalidModuleDriverId = 1,
    } jump_to_bootloader_error_t;

// Variables:

    extern uint8_t UsbDebugInfo[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

// Functions:

    void UsbProtocolHandler(void);
    void SetUsbError(uint8_t error);
    void SetUsbResponseByte(uint8_t response);
    void SetUsbResponseWord(uint16_t response);

#endif
