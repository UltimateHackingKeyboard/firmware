#ifndef _COMPOSITE_APP_H
#define _COMPOSITE_APP_H

#include "keyboard.h"

/* Macros: */

    #define COMPOSITE_CFG_MAX         2
    #define HID_KEYBOARD_INTERFACE_INDEX 0
    #define HID_MOUSE_INTERFACE_INDEX    1
    #define CONTROLLER_ID             USB_CONTROLLER_KHCI_0

/* Type defines: */

    typedef struct composite_device_struct {
            composite_handle_t        composite_device;
            keyboard_global_variable_struct_t hid_keyboard;
            hid_mouse_struct_t        hid_mouse;
            composite_config_struct_t composite_device_config_callback;
            class_config_struct_t     composite_device_config_list[COMPOSITE_CFG_MAX];
    } composite_device_struct_t;

/* Function prototypes */

    extern composite_device_struct_t g_composite_device;

#endif
