#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
//#include "usb_device_msc.h"
#include "usb_device_hid.h"
#include "usb_device_ch9.h"
#include "usb_descriptor.h"

#include "composite.h"

#include "fsl_device_registers.h"

#include <stdio.h>
#include <stdlib.h>

usb_device_class_config_struct_t g_composite_device[USB_COMPOSITE_INTERFACE_COUNT] = {
#if ((USB_DEVICE_CONFIG_HID) && (USB_DEVICE_CONFIG_HID > 0U))
    {
        usb_device_hid_generic_callback, (class_handle_t)NULL, &g_hid_generic_class,
    },
#endif
#if USB_DEVICE_CONFIG_MSC
    {
        usb_device_msc_callback, (class_handle_t)NULL, &g_msc_class,
    }
#endif
#if ((USB_DEVICE_CONFIG_HID == 0) && (USB_DEVICE_CONFIG_MSC == 0))
    {
        NULL, NULL, NULL,
    }
#endif

};

usb_device_class_config_list_struct_t g_composite_device_config_list = {
    g_composite_device, usb_device_callback, USB_COMPOSITE_INTERFACE_COUNT,
};
