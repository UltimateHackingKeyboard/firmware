#ifndef __USB_DEVICE_CONFIG_H__
#define __USB_DEVICE_CONFIG_H__

// KHCI instance count
#define USB_DEVICE_CONFIG_KHCI 1

#include "usb_interfaces/usb_interface_generic_hid.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "usb_interfaces/usb_interface_gamepad.h"

// Device instance count, the sum of KHCI and EHCI instance counts
#define USB_DEVICE_CONFIG_NUM 1

// HID instance count
#define USB_DEVICE_CONFIG_HID ( \
    USB_GENERIC_HID_INTERFACE_COUNT +\
    USB_BASIC_KEYBOARD_INTERFACE_COUNT + \
    USB_MEDIA_KEYBOARD_INTERFACE_COUNT + \
    USB_SYSTEM_KEYBOARD_INTERFACE_COUNT + \
    USB_MOUSE_INTERFACE_COUNT + \
0)

// Whether the device is self-powered: 1 supported, 0 not supported
#define USB_DEVICE_CONFIG_SELF_POWER 0

// Whether device remote wakeup supported: 1 supported, 0 not supported
#define USB_DEVICE_CONFIG_REMOTE_WAKEUP 1

// The number of control endpoints, which is always 1
#define USB_CONTROL_ENDPOINT_COUNT 1

// How many endpoints are supported in the stack
#define USB_DEVICE_CONFIG_ENDPOINTS ( \
    USB_CONTROL_ENDPOINT_COUNT + \
    USB_GENERIC_HID_ENDPOINT_COUNT + \
    USB_BASIC_KEYBOARD_ENDPOINT_COUNT + \
    USB_MEDIA_KEYBOARD_ENDPOINT_COUNT + \
    USB_SYSTEM_KEYBOARD_ENDPOINT_COUNT + \
    USB_MOUSE_ENDPOINT_COUNT + \
    USB_GAMEPAD_ENDPOINT_COUNT + \
0)

// The maximum buffer length for the KHCI DMA workaround
#define USB_DEVICE_CONFIG_KHCI_DMA_ALIGN_BUFFER_LENGTH 64

// Whether handle the USB KHCI bus error
#define USB_DEVICE_CONFIG_KHCI_ERROR_HANDLING 0

// Whether the keep alive feature enabled
#define USB_DEVICE_CONFIG_KEEP_ALIVE_MODE 0

// Whether the transfer buffer is cache-enabled or not
#define USB_DEVICE_CONFIG_BUFFER_PROPERTY_CACHEABLE 0

// Whether the low power mode is enabled or not
#define USB_DEVICE_CONFIG_LOW_POWER_MODE 0

// Whether the device detached feature is enabled or not
#define USB_DEVICE_CONFIG_DETACH_ENABLE 0

#endif
