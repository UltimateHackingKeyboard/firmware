#include "fsl_common.h"
#include "usb_commands/usb_command_get_property.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "versions.h"

version_t deviceProtocolVersion = {
    DEVICE_PROTOCOL_MAJOR_VERSION,
    DEVICE_PROTOCOL_MINOR_VERSION,
    DEVICE_PROTOCOL_PATCH_VERSION,
};

version_t everyProtocolVersion[] =
{
    {
        FIRMWARE_MAJOR_VERSION,
        FIRMWARE_MINOR_VERSION,
        FIRMWARE_PATCH_VERSION,
    },
    {
        DEVICE_PROTOCOL_MAJOR_VERSION,
        DEVICE_PROTOCOL_MINOR_VERSION,
        DEVICE_PROTOCOL_PATCH_VERSION,
    },
    {
        MODULE_PROTOCOL_MAJOR_VERSION,
        MODULE_PROTOCOL_MINOR_VERSION,
        MODULE_PROTOCOL_PATCH_VERSION,
    },
    {
        USER_CONFIG_MAJOR_VERSION,
        USER_CONFIG_MINOR_VERSION,
        USER_CONFIG_PATCH_VERSION,
    },
    {
        HARDWARE_CONFIG_MAJOR_VERSION,
        HARDWARE_CONFIG_MINOR_VERSION,
        HARDWARE_CONFIG_PATCH_VERSION,
    }
};

void UsbCommand_GetProperty(void)
{
    uint8_t propertyId = GetUsbRxBufferUint8(1);

    switch (propertyId) {
        case DevicePropertyId_DeviceProtocolVersion:
            memcpy(GenericHidOutBuffer+1, (uint8_t*)&deviceProtocolVersion, sizeof(deviceProtocolVersion));
            break;
        case DevicePropertyId_EveryProtocolVersion:
            memcpy(GenericHidOutBuffer+1, (uint8_t*)&everyProtocolVersion, sizeof(everyProtocolVersion));
            break;
        case DevicePropertyId_HardwareConfigSize:
            SetUsbTxBufferUint16(1, HARDWARE_CONFIG_SIZE);
            break;
        case DevicePropertyId_UserConfigSize:
            SetUsbTxBufferUint16(1, USER_CONFIG_SIZE);
            break;
        default:
            SetUsbTxBufferUint8(0, UsbStatusCode_GetProperty_InvalidProperty);
            break;
    }
}
