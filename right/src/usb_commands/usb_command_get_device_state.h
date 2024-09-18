#ifndef __USB_COMMAND_GET_KEYBOARD_STATE_H__
#define __USB_COMMAND_GET_KEYBOARD_STATE_H__

// Typedefs:

typedef enum {
    GetDeviceStateByte2_HalvesMerged = 1 << 0,
    GetDeviceStateByte2_PairingInProgress = 1 << 1,

} usb_command_get_device_state_byte2_mask_t;

    typedef enum {
        UhkErrorState_Fine = 0,
        UhkErrorState_Warn = 1,
        UhkErrorState_Error = 2,
    } uhk_error_state_t;

// Functions:

    void UsbCommand_GetKeyboardState(void);

#endif
