#ifndef __USB_COMMAND_GET_KEYBOARD_STATE_H__
#define __USB_COMMAND_GET_KEYBOARD_STATE_H__

// Typedefs:

typedef enum {
    UhkErrorState_Fine,
    UhkErrorState_Warn,
    UhkErrorState_Error,
} uhk_error_state_t;

// Functions:

    void UsbCommand_GetKeyboardState(void);

#endif
