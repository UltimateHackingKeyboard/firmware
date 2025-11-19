#ifndef __USB_COMPATIBILITY_HEADER__
#define __USB_COMPATIBILITY_HEADER__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "usb_interfaces/usb_interface_basic_keyboard.h"
    #include "usb_interfaces/usb_interface_media_keyboard.h"
    #include "usb_interfaces/usb_interface_system_keyboard.h"
    #include "usb_interfaces/usb_interface_mouse.h"
    #include "connections.h"

// Macros:

// Typedefs:

    typedef struct {
        bool capsLock;
        bool numLock;
        bool scrollLock;
    } ATTR_PACKED keyboard_led_state_t;

// Variables:

    extern keyboard_led_state_t KeyboardLedsState;

// Functions:

    // report sending
    void UsbCompatibility_SendKeyboardReport(const usb_basic_keyboard_report_t* report);
    void UsbCompatibility_SendMouseReport(const usb_mouse_report_t* report) ;
    void UsbCompatibility_SendConsumerReport(const usb_media_keyboard_report_t* mediaReport, const usb_system_keyboard_report_t* systemReport);
    void UsbCompatibility_SendConsumerReport2(const uint8_t* report);

    // num lock, caps lock, scroll lock state handling
    void UsbCompatibility_UpdateKeyboardLedsState();
    void UsbCompatibility_SetCurrentKeyboardLedsState(keyboard_led_state_t state);
    void UsbCompatibility_SetKeyboardLedsState(connection_id_t connectionId, bool capsLock, bool numLock, bool scrollLock);

    float VerticalScrollMultiplier(void);
    float HorizontalScrollMultiplier(void);

#endif // __USB_HEADER__
