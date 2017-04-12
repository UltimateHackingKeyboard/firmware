#include "usb_descriptor.h"
#include "composite.h"

usb_device_class_config_struct_t g_composite_device[USB_COMPOSITE_INTERFACE_COUNT] = {{
    usb_device_hid_generic_callback, (class_handle_t)NULL, &g_hid_generic_class,
}};

usb_device_class_config_list_struct_t g_composite_device_config_list = {
    g_composite_device, usb_device_callback, USB_COMPOSITE_INTERFACE_COUNT,
};
