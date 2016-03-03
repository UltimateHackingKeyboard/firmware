#ifndef __USB_DEVICE_CONFIG_H__
#define __USB_DEVICE_CONFIG_H__

// KHCI instance count
#define USB_DEVICE_CONFIG_KHCI 1

// Device instance count, the sum of KHCI and EHCI instance counts
#define USB_DEVICE_CONFIG_NUM 1

// HID instance count
#define USB_DEVICE_CONFIG_HID 3

// Whether the device is self-powered: 1 supported, 0 not supported
#define USB_DEVICE_CONFIG_SELF_POWER 1

// Whether device remote wakeup supported: 1 supported, 0 not supported
#define USB_DEVICE_CONFIG_REMOTE_WAKEUP 0

// How many endpoints are supported in the stack
#define USB_DEVICE_CONFIG_ENDPOINTS 6

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
