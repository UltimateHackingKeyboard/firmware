#ifndef __USB_COMMAND_GET_KEYBOARD_STATE_H__
#define __USB_COMMAND_GET_KEYBOARD_STATE_H__

// Typedefs:

    typedef enum {
        UhkErrorState_Fine = 0,
        UhkErrorState_Warn = 1,
        UhkErrorState_Error = 2,
    } uhk_error_state_t;

// Functions:

    void UsbCommand_GetKeyboardState(void);

#endif
