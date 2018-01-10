#ifndef __USB_COMMAND_GET_DEVICE_PROPERTY_H__
#define __USB_COMMAND_GET_DEVICE_PROPERTY_H__

// Typedefs:

    typedef enum {
        DevicePropertyId_DeviceProtocolVersion = 0,
        DevicePropertyId_ProtocolVersions      = 1,
        DevicePropertyId_ConfigSizes           = 2,
        DevicePropertyId_CurrentKbootCommand   = 3,
    } system_property_t;

    typedef enum {
        UsbStatusCode_GetDeviceProperty_InvalidProperty = 2,
    } usb_status_code_get_device_property_t;

// Functions:

    void UsbCommand_GetDeviceProperty(void);

#endif
