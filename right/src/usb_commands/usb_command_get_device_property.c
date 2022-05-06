#include "fsl_common.h"
#include "usb_commands/usb_command_get_device_property.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "versions.h"
#include "slave_drivers/kboot_driver.h"
#include "i2c.h"
#include "init_peripherals.h"
#include "fsl_i2c.h"
#include "timer.h"
#include "utils.h"
#include "versions.h"

version_t deviceProtocolVersion = {
    DEVICE_PROTOCOL_MAJOR_VERSION,
    DEVICE_PROTOCOL_MINOR_VERSION,
    DEVICE_PROTOCOL_PATCH_VERSION,
};

version_t protocolVersions[] =
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
    },
    {
        SMART_MACROS_MAJOR_VERSION,
        SMART_MACROS_MINOR_VERSION,
        SMART_MACROS_PATCH_VERSION,
    }
};

uint16_t configSizes[] = {HARDWARE_CONFIG_SIZE, USER_CONFIG_SIZE};

void UsbCommand_GetDeviceProperty(void)
{
    uint8_t propertyId = GetUsbRxBufferUint8(1);

    switch (propertyId) {
        case DevicePropertyId_DeviceProtocolVersion:
            memcpy(GenericHidInBuffer+1, (uint8_t*)&deviceProtocolVersion, sizeof(deviceProtocolVersion));
            break;
        case DevicePropertyId_ProtocolVersions:
            memcpy(GenericHidInBuffer+1, (uint8_t*)&protocolVersions, sizeof(protocolVersions));
            break;
        case DevicePropertyId_ConfigSizes:
            memcpy(GenericHidInBuffer+1, (uint8_t*)&configSizes, sizeof(configSizes));
            break;
        case DevicePropertyId_CurrentKbootCommand:
            GenericHidInBuffer[1] = KbootDriverState.command;
            break;
        case DevicePropertyId_I2cMainBusBaudRate:
            GenericHidInBuffer[1] = I2C_MAIN_BUS_BASEADDR->F;
            SetUsbTxBufferUint32(2, I2cMainBusRequestedBaudRateBps);
            SetUsbTxBufferUint32(6, I2cMainBusActualBaudRateBps);
            break;
        case DevicePropertyId_Uptime:
            SetUsbTxBufferUint32(1, CurrentTime);
            break;
        case DevicePropertyId_GitTag:
            Utils_SafeStrCopy(((char*)GenericHidInBuffer) + 1, GIT_TAG, sizeof(GenericHidInBuffer)-1);
            break;
        case DevicePropertyId_GitRepo:
            Utils_SafeStrCopy(((char*)GenericHidInBuffer) + 1, GIT_REPO, sizeof(GenericHidInBuffer)-1);
            break;
        default:
            SetUsbTxBufferUint8(0, UsbStatusCode_GetDeviceProperty_InvalidProperty);
            break;
    }
}
