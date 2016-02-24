#ifndef __KEYBOARD_DESCRIPTORS_H__
#define __KEYBOARD_DESCRIPTORS_H__

// Macros:

    #define USB_KEYBOARD_INTERFACE_ALTERNATE_SETTING (0U)

// Function prototypes:

    extern usb_device_class_struct_t UsbKeyboardClass;
    extern uint8_t UsbKeyboardReportDescriptor[USB_DESCRIPTOR_LENGTH_HID_KEYBOARD_REPORT];

#endif
