#ifndef __USB_DEVICE_CONFIG_H__
#define __USB_DEVICE_CONFIG_H__

/*! @brief KHCI instance count */
#define USB_DEVICE_CONFIG_KHCI 1

/*! @brief Device instance count, the sum of KHCI and EHCI instance counts*/
#define USB_DEVICE_CONFIG_NUM 1

/*! @brief HID instance count */
#define USB_DEVICE_CONFIG_HID 3

/*! @brief Whether device is self power. 1U supported, 0U not supported */
#define USB_DEVICE_CONFIG_SELF_POWER 1

/*! @brief Whether device remote wakeup supported. 1U supported, 0U not supported */
#define USB_DEVICE_CONFIG_REMOTE_WAKEUP 0

/*! @brief How many endpoints are supported in the stack. */
#define USB_DEVICE_CONFIG_ENDPOINTS 6

/*! @brief The MAX buffer length for the KHCI DMA workaround.*/
#define USB_DEVICE_CONFIG_KHCI_DMA_ALIGN_BUFFER_LENGTH 64

/*! @brief Whether handle the USB KHCI bus error. */
#define USB_DEVICE_CONFIG_KHCI_ERROR_HANDLING 0

/*! @brief Whether the keep alive feature enabled. */
#define USB_DEVICE_CONFIG_KEEP_ALIVE_MODE 0

/*! @brief Whether the transfer buffer is cache-enabled or not. */
#define USB_DEVICE_CONFIG_BUFFER_PROPERTY_CACHEABLE 0

/*! @brief Whether the low power mode is enabled or not. */
#define USB_DEVICE_CONFIG_LOW_POWER_MODE 0

/*! @brief Whether the device detached feature is enabled or not. */
#define USB_DEVICE_CONFIG_DETACH_ENABLE 0

#endif
