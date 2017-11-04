#include "fsl_common.h"
#include "usb_commands/usb_command_get_property.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"

void UsbCommand_GetProperty(void)
{
    uint8_t propertyId = GenericHidInBuffer[1];

    switch (propertyId) {
        case SystemPropertyId_UsbProtocolVersion:
            SetUsbResponseByte(SYSTEM_PROPERTY_USB_PROTOCOL_VERSION);
            break;
        case SystemPropertyId_BridgeProtocolVersion:
            SetUsbResponseByte(SYSTEM_PROPERTY_BRIDGE_PROTOCOL_VERSION);
            break;
        case SystemPropertyId_DataModelVersion:
            SetUsbResponseByte(SYSTEM_PROPERTY_DATA_MODEL_VERSION);
            break;
        case SystemPropertyId_FirmwareVersion:
            SetUsbResponseByte(SYSTEM_PROPERTY_FIRMWARE_VERSION);
            break;
        case SystemPropertyId_HardwareConfigSize:
            SetUsbResponseWord(HARDWARE_CONFIG_SIZE);
            break;
        case SystemPropertyId_UserConfigSize:
            SetUsbResponseWord(USER_CONFIG_SIZE);
            break;
        default:
            SetUsbError(1);
            break;
    }
}
