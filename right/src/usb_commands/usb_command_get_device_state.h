#ifndef __USB_COMMAND_GET_KEYBOARD_STATE_H__
#define __USB_COMMAND_GET_KEYBOARD_STATE_H__

// Includes:

    #include <stdint.h>

// Typedefs:

typedef enum {
    GetDeviceStateByte2_HalvesMerged = 1 << 0,
    GetDeviceStateByte2_PairingInProgress = 1 << 1,
    GetDeviceStateByte2_NewPairedDevice = 1 << 2,
    GetDeviceStateByte2_ZephyrLog = 1 << 3,

} usb_command_get_device_state_byte2_mask_t;

    typedef enum {
        UhkErrorState_Fine = 0,
        UhkErrorState_Warn = 1,
        UhkErrorState_Error = 2,
    } uhk_error_state_t;

// Functions:

    void UsbCommand_GetKeyboardState(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
