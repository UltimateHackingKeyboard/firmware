#ifndef __USB_DESCRIPTOR_H__
#define __USB_DESCRIPTOR_H__

#include "bootloader_common.h"
#include "usb_device_config.h"

#define USB_BCD_VERSION (0x0200)

#define USB_DEVICE_DESCRIPTOR_LENGTH 18

#define USB_CONFIGURE_DESCRIPTOR_LENGTH 41

#define USB_HID_REPORT_DESC_SIZE 76
#define USB_HID_GENERIC_DESCRIPTOR_LENGTH 32
#define USB_CONFIGURE_ONLY_DESCRIPTOR_LENGTH 9
#define USB_INTERFACE_DESCRIPTOR_LENGTH 9
#define USB_HID_DESCRIPTOR_LENGTH 9
#define USB_ENDPOINT_DESCRIPTOR_LENGTH 7
#define USB_MSC_DISK_REPORT_DESCRIPTOR_LENGTH 63
#define USB_IAD_DESC_SIZE 8

#define USB_CONFIGURE_COUNT 1
#define USB_STRING_COUNT 4
#define USB_LANGUAGE_COUNT 1

#define USB_HID_CONFIG_INDEX (USB_CONFIGURE_ONLY_DESCRIPTOR_LENGTH)

#define USB_HID_GENERIC_INTERFACE_INDEX 0

#define USB_HID_GENERIC_CONFIGURE_INDEX 1
#define USB_HID_GENERIC_INTERFACE_COUNT 1

#define USB_HID_GENERIC_IN_BUFFER_LENGTH 8
#define USB_HID_GENERIC_OUT_BUFFER_LENGTH 8
#define USB_HID_GENERIC_ENDPOINT_COUNT 2
#define USB_HID_GENERIC_ENDPOINT_IN 1
#define USB_HID_GENERIC_ENDPOINT_OUT 2

#define USB_HID_GENERIC_CLASS 0x03
#define USB_HID_GENERIC_SUBCLASS 0x00
#define USB_HID_GENERIC_PROTOCOL 0x00

#define FS_HID_GENERIC_INTERRUPT_OUT_PACKET_SIZE 64
#define FS_HID_GENERIC_INTERRUPT_OUT_INTERVAL 0x01

#define FS_HID_GENERIC_INTERRUPT_IN_PACKET_SIZE 64
#define FS_HID_GENERIC_INTERRUPT_IN_INTERVAL 0x01

#define USB_COMPOSITE_INTERFACE_COUNT USB_HID_GENERIC_INTERFACE_COUNT

#define USB_COMPOSITE_CONFIGURE_INDEX 1

#define USB_STRING_DESCRIPTOR_HEADER_LENGTH 0x02
#define USB_STRING_DESCRIPTOR_0_LENGTH 0x02
#define USB_STRING_DESCRIPTOR_1_LENGTH 56
#define USB_STRING_DESCRIPTOR_2_LENGTH 32
#define USB_STRING_DESCRIPTOR_3_LENGTH 44
#define USB_STRING_DESCRIPTOR_ERROR_LENGTH 32

#define USB_CONFIGURE_DRAWN 0x32

enum _usb_descriptor_index {
    kUsbDescriptorIndex_VidLow = 8,
    kUsbDescriptorIndex_VidHigh = 9,
    kUsbDescriptorIndex_PidLow = 10,
    kUsbDescriptorIndex_PidHigh = 11
};

typedef struct _usb_hid_config_descriptor {
    usb_descriptor_interface_t interface;   /* Interface descriptor */
    usb_descriptor_interface_t hid_report;  /* Interface descriptor */
    usb_descriptor_endpoint_t endpoint_in;  /* Endpoint descriptor */
    usb_descriptor_endpoint_t endpoint_out; /* Endpoint descriptor */
} usb_hid_config_descriptor_t;

extern usb_device_class_struct_t g_hid_generic_class;

usb_status_t usb_device_get_device_descriptor(
    usb_device_handle handle, usb_device_get_device_descriptor_struct_t *device_descriptor);
usb_status_t usb_device_get_configuration_descriptor(
    usb_device_handle handle, usb_device_get_configuration_descriptor_struct_t *configuration_descriptor);
usb_status_t usb_device_get_string_descriptor(
    usb_device_handle handle, usb_device_get_string_descriptor_struct_t *string_descriptor);
usb_status_t usb_device_get_hid_descriptor(
    usb_device_handle handle, usb_device_get_hid_descriptor_struct_t *hid_descriptor);
usb_status_t usb_device_get_hid_report_descriptor(
    usb_device_handle handle, usb_device_get_hid_report_descriptor_struct_t *hid_report_descriptor);
usb_status_t usb_device_get_hid_physical_descriptor(
    usb_device_handle handle, usb_device_get_hid_physical_descriptor_struct_t *hid_physical_descriptor);


extern uint8_t g_device_descriptor[];
extern usb_language_list_t g_language_list;
extern usb_language_list_t *g_language_ptr;

#endif
