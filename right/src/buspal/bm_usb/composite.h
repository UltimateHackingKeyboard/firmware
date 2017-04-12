#ifndef __USB_DEVICE_COMPOSITE_H__
#define __USB_DEVICE_COMPOSITE_H__

#include "hid_bootloader.h"
#include "usb_device_config.h"

#define CONTROLLER_ID kUSB_ControllerKhci0
#define USB_DEVICE_INTERRUPT_PRIORITY 4

typedef struct _usb_device_composite_struct {
    usb_device_handle device_handle;      // USB device handle.
    usb_hid_generic_struct_t hid_generic; // HID device structure
    uint8_t speed;                        // Speed of USB device. USB_SPEED_FULL/USB_SPEED_LOW/USB_SPEED_HIGH.
    uint8_t attach;                       // A flag to indicate whether a usb device is attached. 1: attached, 0: not attached
    uint8_t current_configuration;        // Current configuration value.
    uint8_t current_interface_alternate_setting[USB_COMPOSITE_INTERFACE_COUNT]; // Current alternate setting value for each interface.
} usb_device_composite_struct_t;

extern usb_status_t usb_device_callback(usb_device_handle handle, uint32_t event, void *param);
extern usb_status_t usb_device_hid_generic_init(usb_device_composite_struct_t *device_composite);
extern usb_status_t usb_device_hid_generic_callback(class_handle_t handle, uint32_t event, void *param);
extern usb_status_t usb_device_hid_generic_set_configure(class_handle_t handle, uint8_t configure);
extern usb_status_t usb_device_hid_generic_set_interface(class_handle_t handle, uint8_t interface, uint8_t alternate_setting);
extern usb_status_t usb_device_hid_generic_deinit(usb_device_composite_struct_t *device_composite);

extern usb_device_class_config_list_struct_t g_composite_device_config_list;

#endif
