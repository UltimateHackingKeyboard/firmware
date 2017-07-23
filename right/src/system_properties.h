#ifndef __SYSTEM_PROPERTIES_H__
#define __SYSTEM_PROPERTIES_H__

// Macros:

    #define SYSTEM_PROPERTY_USB_PROTOCOL_VERSION      1
    #define SYSTEM_PROPERTY_BRIDGE_PROTOCOL_VERSION   1
    #define SYSTEM_PROPERTY_DATA_MODEL_VERSION        1
    #define SYSTEM_PROPERTY_FIRMWARE_VERSION          1

// Typedefs:

    typedef enum {
        SystemPropertyId_UsbProtocolVersion    = 0,
        SystemPropertyId_BridgeProtocolVersion = 1,
        SystemPropertyId_DataModelVersion      = 2,
        SystemPropertyId_FirmwareVersion       = 3,
        SystemPropertyId_HardwareConfigSize    = 4,
        SystemPropertyId_UserConfigSize        = 5,
    } system_property_t;

#endif
