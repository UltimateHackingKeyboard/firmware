#ifndef __USB_COMMAND_SWITCH_KEYMAP_H__
#define __USB_COMMAND_SWITCH_KEYMAP_H__

// Functions:

    void UsbCommand_SwitchKeymap(void);

// Typedefs:

    typedef enum {
        UsbStatusCode_SwitchKeymap_InvalidAbbreviationLength = 2,
        UsbStatusCode_SwitchKeymap_InvalidAbbreviation = 3,
    } usb_status_code_switch_keymap_t;

#endif
