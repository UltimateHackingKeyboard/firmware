#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

    #include "usb_interfaces/usb_interface_generic_hid.h"

// Typedefs:

    typedef enum {
        UsbCommand_GetSystemProperty          =  0,
        UsbCommand_Reenumerate                =  1,
        UsbCommand_SetTestLed                 =  2,
        UsbCommand_WriteLedDriver             =  3,
        UsbCommand_ReadMergeSensor            =  7,
        UsbCommand_WriteUserConfiguration     =  8,
        UsbCommand_ApplyConfig                =  9,
        UsbCommand_SetLedPwm                  = 10,
        UsbCommand_GetAdcValue                = 11,
        UsbCommand_LaunchEepromTransfer       = 12,
        UsbCommand_ReadHardwareConfiguration  = 13,
        UsbCommand_WriteHardwareConfiguration = 14,
        UsbCommand_ReadUserConfiguration      = 15,
        UsbCommand_GetKeyboardState           = 16,
        UsbCommand_GetDebugInfo               = 17,
        UsbCommand_JumpToSlaveBootloader           = 18,
    } usb_command_t;

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

// Functions:

    void UsbProtocolHandler(void);
    void ApplyConfig(void);

#endif
