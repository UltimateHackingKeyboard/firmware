#ifndef __USB_COMMAND_GET_DEVICE_PROPERTY_H__
#define __USB_COMMAND_GET_DEVICE_PROPERTY_H__

// Typedefs:

    typedef enum {
        DevicePropertyId_DeviceProtocolVersion = 0,
        DevicePropertyId_ProtocolVersions      = 1,
        DevicePropertyId_ConfigSizes           = 2,
        DevicePropertyId_CurrentKbootCommand   = 3,
        DevicePropertyId_I2cMainBusBaudRate    = 4,
        DevicePropertyId_Uptime                = 5,
        DevicePropertyId_GitTag                = 6,
        DevicePropertyId_GitRepo               = 7,
    } device_property_t;

    typedef enum {
        UsbStatusCode_GetDeviceProperty_InvalidProperty = 2,
    } usb_status_code_get_device_property_t;

// Functions:

    void UsbCommand_GetDeviceProperty(void);

#endif
