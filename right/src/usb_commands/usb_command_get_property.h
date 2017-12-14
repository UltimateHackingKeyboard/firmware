#ifndef __USB_COMMAND_GET_PROPERTY_H__
#define __USB_COMMAND_GET_PROPERTY_H__

// Typedefs:

    typedef enum {
        DevicePropertyId_DeviceProtocolVersion = 0,
        DevicePropertyId_ProtocolVersions      = 1,
        DevicePropertyId_ConfigSizes           = 2,
        DevicePropertyId_HardwareConfigSize    = 4,
        DevicePropertyId_UserConfigSize        = 5,
    } system_property_t;

    typedef enum {
        UsbStatusCode_GetProperty_InvalidProperty = 2,
    } usb_status_code_get_property_t;

// Functions:

    void UsbCommand_GetProperty(void);

#endif
