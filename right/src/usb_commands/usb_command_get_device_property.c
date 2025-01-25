#include <string.h>

#ifdef __ZEPHYR__
    #include "device.h"
    #include "flash.h"
    #include <zephyr/bluetooth/bluetooth.h>
    #include "bt_conn.h"
    #include "bt_pair.h"
#else
    #include "eeprom.h"
    #include "fsl_common.h"
    #include "fsl_i2c.h"
    #include "i2c.h"
    #include "init_peripherals.h"
    #include "slave_drivers/kboot_driver.h"
#endif

#include <stdbool.h>
#include <stdint.h>
#include "slave_protocol.h"
#include "timer.h"
#include "usb_commands/usb_command_get_new_pairings.h"
#include "usb_commands/usb_command_get_device_property.h"
#include "usb_protocol_handler.h"
#include "utils.h"
#include "versioning.h"

uint16_t configSizes[] = {HARDWARE_CONFIG_SIZE, USER_CONFIG_SIZE};

void UsbCommand_GetDeviceProperty(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    uint8_t propertyId = GetUsbRxBufferUint8(1);
    uint8_t *dest = GenericHidInBuffer + 1;

    switch (propertyId) {
    case DevicePropertyId_DeviceProtocolVersion:
        memcpy(GenericHidInBuffer + 1, (uint8_t *)&deviceProtocolVersion,
            sizeof(deviceProtocolVersion));
        break;
    case DevicePropertyId_ProtocolVersions:
        memcpy(dest, (uint8_t *)&firmwareVersion, sizeof(firmwareVersion));
        dest += sizeof(firmwareVersion);
        memcpy(dest, (uint8_t *)&deviceProtocolVersion, sizeof(deviceProtocolVersion));
        dest += sizeof(deviceProtocolVersion);
        memcpy(dest, (uint8_t *)&moduleProtocolVersion, sizeof(moduleProtocolVersion));
        dest += sizeof(moduleProtocolVersion);
        memcpy(dest, (uint8_t *)&userConfigVersion, sizeof(userConfigVersion));
        dest += sizeof(userConfigVersion);
        memcpy(dest, (uint8_t *)&hardwareConfigVersion, sizeof(hardwareConfigVersion));
        dest += sizeof(hardwareConfigVersion);
        memcpy(dest, (uint8_t *)&smartMacrosVersion, sizeof(smartMacrosVersion));
        break;
    case DevicePropertyId_ConfigSizes:
        memcpy(GenericHidInBuffer + 1, (uint8_t *)&configSizes, sizeof(configSizes));
        break;
#ifndef __ZEPHYR__
    case DevicePropertyId_CurrentKbootCommand:
        GenericHidInBuffer[1] = KbootDriverState.command;
        break;
    case DevicePropertyId_I2cMainBusBaudRate:
        GenericHidInBuffer[1] = I2C_MAIN_BUS_BASEADDR->F;
        SetUsbTxBufferUint32(2, I2cMainBusRequestedBaudRateBps);
        SetUsbTxBufferUint32(6, I2cMainBusActualBaudRateBps);
        break;
#endif
    case DevicePropertyId_Uptime:
        SetUsbTxBufferUint32(1, CurrentTime);
        break;
    case DevicePropertyId_GitTag:
        Utils_SafeStrCopy(((char *)GenericHidInBuffer) + 1, gitTag, USB_GENERIC_HID_IN_BUFFER_LENGTH - 1);
        break;
    case DevicePropertyId_GitRepo:
        Utils_SafeStrCopy(
            ((char *)GenericHidInBuffer) + 1, gitRepo, USB_GENERIC_HID_IN_BUFFER_LENGTH - 1);
        break;
    case DevicePropertyId_BuiltFirmwareChecksumByModuleId: {
        uint8_t moduleId = GetUsbRxBufferUint8(2);
        const char *checksum = NULL;
        switch (DEVICE_ID) {
            case DEVICE_ID_UHK60V1_RIGHT:
            case DEVICE_ID_UHK60V2_RIGHT:
                if (moduleId == ModuleId_RightKeyboardHalf) {
                    checksum = DeviceMD5Checksums[DEVICE_ID];
                } else if (moduleId < ModuleId_ModuleCount) {
                    checksum = ModuleMD5Checksums[moduleId];
                } else {
                    SetUsbTxBufferUint8(0, UsbStatusCode_GetDeviceProperty_InvalidArgument);
                }
                break;
            case DEVICE_ID_UHK80_LEFT:
            case DEVICE_ID_UHK80_RIGHT:
            case DEVICE_ID_UHK_DONGLE:
                switch (moduleId) {
                    case ModuleId_LeftKeyboardHalf:
                        checksum = DeviceMD5Checksums[DeviceId_Uhk80_Left];
                        break;
                    case ModuleId_RightKeyboardHalf:
                        checksum = DeviceMD5Checksums[DeviceId_Uhk80_Right];
                        break;
                    case ModuleId_Dongle:
                        checksum = DeviceMD5Checksums[DeviceId_Uhk_Dongle];
                        break;
                    default:
                        checksum = ModuleMD5Checksums[moduleId];
                        break;
                }
                break;
        }
        Utils_SafeStrCopy(((char *)GenericHidInBuffer) + 1, checksum, MD5_CHECKSUM_LENGTH + 1);
    } break;
    case DevicePropertyId_BleAddress: {
#ifdef __ZEPHYR__
        bt_addr_le_t addr;
        size_t count = 1;
        bt_id_get(&addr, &count);
        memcpy(GenericHidInBuffer + 1, addr.a.val, sizeof(addr.a.val));
#endif
    } break;
    case DevicePropertyId_PairedRightPeerBleAddress: {
#ifdef __ZEPHYR__
        memcpy(GenericHidInBuffer + 1, Peers[PeerIdRight].addr.a.val, sizeof(Peers[PeerIdRight].addr.a.val));
#endif
    } break;
    case DevicePropertyId_PairingStatus: {
#ifdef __ZEPHYR__
        if (BtPair_OobPairingInProgress) {
            SetUsbTxBufferUint8(1, PairingStatus_InProgress);
        } else {
            SetUsbTxBufferUint8(1, BtPair_LastPairingSucceeded ? PairingStatus_Success : PairingStatus_Failed);
        }
#endif
    } break;
    case DevicePropertyId_NewPairings:
#ifdef __ZEPHYR__
        UsbCommand_GetNewPairings(GetUsbRxBufferUint8(2), GenericHidOutBuffer, GenericHidInBuffer);
#endif
        break;
    default:
        SetUsbTxBufferUint8(0, UsbStatusCode_GetDeviceProperty_InvalidProperty);
        break;
    }
}
