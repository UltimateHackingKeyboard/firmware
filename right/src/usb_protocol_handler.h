#ifndef __USB_PROTOCOL_HANDLER_H__
#define __USB_PROTOCOL_HANDLER_H__

// Includes:

    #include "fsl_common.h"
    #include "usb_interfaces/usb_interface_generic_hid.h"

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
    } usb_response_t;

    typedef enum {
        UsbStatusCode_TransferConfig_LengthTooLarge    = 2,
        UsbStatusCode_TransferConfig_BufferOutOfBounds = 3,
    } config_transfer_response_t;

    typedef enum {
        UsbStatusCode_JumpToSlaveBootloader_InvalidModuleDriverId = 2,
    } jump_to_bootloader_error_t;

// Functions:

    void UsbProtocolHandler(void);
    void SetUsbStatusCode(uint8_t status);
    void SetUsbResponseByte(uint8_t response);
    void SetUsbResponseWord(uint16_t response);

#endif
