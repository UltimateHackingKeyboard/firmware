#include "fsl_common.h"
#include "usb_commands/usb_command_get_property.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"

void UsbCommand_GetProperty(void)
{
    uint8_t propertyId = GetUsbRxBufferUint8(1);

    switch (propertyId) {
        case SystemPropertyId_UsbProtocolVersion:
            SetUsbTxBufferUint8(1, SYSTEM_PROPERTY_USB_PROTOCOL_VERSION);
            break;
        case SystemPropertyId_BridgeProtocolVersion:
            SetUsbTxBufferUint8(1, SYSTEM_PROPERTY_BRIDGE_PROTOCOL_VERSION);
            break;
        case SystemPropertyId_DataModelVersion:
            SetUsbTxBufferUint8(1, SYSTEM_PROPERTY_DATA_MODEL_VERSION);
            break;
        case SystemPropertyId_FirmwareVersion:
            SetUsbTxBufferUint8(1, SYSTEM_PROPERTY_FIRMWARE_VERSION);
            break;
        case SystemPropertyId_HardwareConfigSize:
            SetUsbTxBufferUint16(1, HARDWARE_CONFIG_SIZE);
            break;
        case SystemPropertyId_UserConfigSize:
            SetUsbTxBufferUint16(1, USER_CONFIG_SIZE);
            break;
        default:
            SetUsbTxBufferUint8(0, UsbStatusCode_GetProperty_InvalidProperty);
            break;
    }
}
