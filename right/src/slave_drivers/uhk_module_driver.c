#include "i2c_addresses.h"
#include "i2c.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slave_protocol.h"
#include "main.h"
#include "peripherals/test_led.h"
#include "bool_array_converter.h"
#include "crc16.h"

uhk_module_state_t UhkModuleStates[UHK_MODULE_MAX_COUNT];

static i2c_message_t txMessage;

static uhk_module_i2c_addresses_t moduleIdsToI2cAddresses[] = {
    { // UhkModuleDriverId_LeftKeyboardHalf
        .firmwareI2cAddress   = I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE,
        .bootloaderI2cAddress = I2C_ADDRESS_LEFT_KEYBOARD_HALF_BOOTLOADER
    },
    { // UhkModuleDriverId_LeftAddon
        .firmwareI2cAddress   = I2C_ADDRESS_LEFT_ADDON_FIRMWARE,
        .bootloaderI2cAddress = I2C_ADDRESS_LEFT_ADDON_BOOTLOADER
    },
    { // UhkModuleDriverId_RightAddon
        .firmwareI2cAddress   = I2C_ADDRESS_RIGHT_ADDON_FIRMWARE,
        .bootloaderI2cAddress = I2C_ADDRESS_RIGHT_ADDON_BOOTLOADER
    },
};

static status_t tx(uint8_t i2cAddress)
{
    return I2cAsyncWriteMessage(i2cAddress, &txMessage);
}

static status_t rx(i2c_message_t *rxMessage, uint8_t i2cAddress)
{
    return I2cAsyncReadMessage(i2cAddress, rxMessage);
}

void UhkModuleSlaveDriver_Init(uint8_t uhkModuleDriverId)
{
    uhk_module_state_t *uhkModuleState = UhkModuleStates + uhkModuleDriverId;
    uhk_module_vars_t *uhkModuleSourceVars = &uhkModuleState->sourceVars;
    uhk_module_vars_t *uhkModuleTargetVars = &uhkModuleState->targetVars;

    uhkModuleSourceVars->isTestLedOn = true;
    uhkModuleTargetVars->isTestLedOn = false;

    uhkModuleSourceVars->ledPwmBrightness = MAX_PWM_BRIGHTNESS;
    uhkModuleTargetVars->ledPwmBrightness = 0;

    uhk_module_phase_t *uhkModulePhase = &uhkModuleState->phase;
    *uhkModulePhase = UhkModulePhase_RequestSync;

    uhk_module_i2c_addresses_t *uhkModuleI2cAddresses = moduleIdsToI2cAddresses + uhkModuleDriverId;
    uhkModuleState->firmwareI2cAddress = uhkModuleI2cAddresses->firmwareI2cAddress;
    uhkModuleState->bootloaderI2cAddress = uhkModuleI2cAddresses->bootloaderI2cAddress;
}

status_t UhkModuleSlaveDriver_Update(uint8_t uhkModuleDriverId)
{
    status_t status = kStatus_Uhk_IdleSlave;
    uhk_module_state_t *uhkModuleState = UhkModuleStates + uhkModuleDriverId;
    uhk_module_vars_t *uhkModuleSourceVars = &uhkModuleState->sourceVars;
    uhk_module_vars_t *uhkModuleTargetVars = &uhkModuleState->targetVars;
    uhk_module_phase_t *uhkModulePhase = &uhkModuleState->phase;
    uint8_t i2cAddress = uhkModuleState->firmwareI2cAddress;
    i2c_message_t *rxMessage = &uhkModuleState->rxMessage;

    switch (*uhkModulePhase) {

        // Sync communication
        case UhkModulePhase_RequestSync:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_Sync;
            txMessage.length = 2;
            status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveSync;
            break;
        case UhkModulePhase_ReceiveSync:
            status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessSync;
            break;
        case UhkModulePhase_ProcessSync: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            bool isSyncValid = memcmp(rxMessage->data, SlaveSyncString, SLAVE_SYNC_STRING_LENGTH) == 0;
            status = kStatus_Uhk_NoTransfer;
            *uhkModulePhase = isSyncValid && isMessageValid
                ? UhkModulePhase_RequestProtocolVersion
                : UhkModulePhase_RequestSync;
            break;
        }

        // Get protocol version
        case UhkModulePhase_RequestProtocolVersion:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_ProtocolVersion;
            txMessage.length = 2;
            status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveProtocolVersion;
            break;
        case UhkModulePhase_ReceiveProtocolVersion:
            status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessProtocolVersion;
            break;
        case UhkModulePhase_ProcessProtocolVersion: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            if (isMessageValid) {
                uhkModuleState->protocolVersion = rxMessage->data[0];
            }
            status = kStatus_Uhk_NoTransfer;
            *uhkModulePhase = isMessageValid ? UhkModulePhase_RequestModuleId : UhkModulePhase_RequestProtocolVersion;
            break;
        }

        // Get module id
        case UhkModulePhase_RequestModuleId:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_ModuleId;
            txMessage.length = 2;
            status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveModuleId;
            break;
        case UhkModulePhase_ReceiveModuleId:
            status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessModuleId;
            break;
        case UhkModulePhase_ProcessModuleId: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            if (isMessageValid) {
                uhkModuleState->moduleId = rxMessage->data[0];
            }
            status = kStatus_Uhk_NoTransfer;
            *uhkModulePhase = isMessageValid ? UhkModulePhase_RequestModuleFeatures : UhkModulePhase_RequestModuleId;
            break;
        }

        // Get module features
        case UhkModulePhase_RequestModuleFeatures:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_Features;
            txMessage.length = 2;
            status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveModuleFeatures;
            break;
        case UhkModulePhase_ReceiveModuleFeatures:
            status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessModuleFeatures;
            break;
        case UhkModulePhase_ProcessModuleFeatures: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            if (isMessageValid) {
                memcpy(&uhkModuleState->features, rxMessage->data, sizeof(uhk_module_features_t));
            }
            status = kStatus_Uhk_NoTransfer;
            *uhkModulePhase = isMessageValid ? UhkModulePhase_RequestKeyStates : UhkModulePhase_RequestModuleFeatures;
            break;
        }

        // Get key states
        case UhkModulePhase_RequestKeyStates:
            txMessage.data[0] = SlaveCommand_RequestKeyStates;
            txMessage.length = 1;
            status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveKeystates;
            break;
        case UhkModulePhase_ReceiveKeystates:
            status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessKeystates;
            break;
        case UhkModulePhase_ProcessKeystates:
            if (CRC16_IsMessageValid(rxMessage)) {
                uint8_t slotId = uhkModuleDriverId + 1;
                BoolBitsToBytes(rxMessage->data, CurrentKeyStates[slotId], uhkModuleState->features.keyCount);
            }
            status = kStatus_Uhk_NoTransfer;
            *uhkModulePhase = UhkModulePhase_JumpToBootloader;
            break;

        // Jump to bootloader
        case UhkModulePhase_JumpToBootloader:
            if (uhkModuleState->jumpToBootloader) {
                txMessage.data[0] = SlaveCommand_JumpToBootloader;
                txMessage.length = 1;
                status = tx(i2cAddress);
                uhkModuleState->jumpToBootloader = false;
            } else {
                status = kStatus_Uhk_NoTransfer;
            }
            *uhkModulePhase = UhkModulePhase_SetTestLed;
            break;

        // Set test LED
        case UhkModulePhase_SetTestLed:
            if (uhkModuleSourceVars->isTestLedOn == uhkModuleTargetVars->isTestLedOn) {
                status = kStatus_Uhk_NoTransfer;
            } else {
                txMessage.data[0] = SlaveCommand_SetTestLed;
                txMessage.data[1] = uhkModuleSourceVars->isTestLedOn;
                txMessage.length = 2;
                status = tx(i2cAddress);
                uhkModuleTargetVars->isTestLedOn = uhkModuleSourceVars->isTestLedOn;
            }
            *uhkModulePhase = UhkModulePhase_SetLedPwmBrightness;
            break;

        // Set PWM brightness
        case UhkModulePhase_SetLedPwmBrightness:
            if (uhkModuleSourceVars->ledPwmBrightness == uhkModuleTargetVars->ledPwmBrightness) {
                status = kStatus_Uhk_NoTransfer;
            } else {
                txMessage.data[0] = SlaveCommand_SetLedPwmBrightness;
                txMessage.data[1] = uhkModuleSourceVars->ledPwmBrightness;
                txMessage.length = 2;
                status = tx(i2cAddress);
                uhkModuleTargetVars->ledPwmBrightness = uhkModuleSourceVars->ledPwmBrightness;
            }
            *uhkModulePhase = UhkModulePhase_RequestKeyStates;
            break;
    }

    return status;
}

void UhkModuleSlaveDriver_Disconnect(uint8_t uhkModuleDriverId)
{
    if (uhkModuleDriverId == SlaveId_LeftKeyboardHalf) {
        Slaves[SlaveId_LeftLedDriver].isConnected = false;
    }
    UhkModuleStates[uhkModuleDriverId].moduleId = 0;
}
