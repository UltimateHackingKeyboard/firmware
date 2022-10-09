#include "i2c_addresses.h"
#include "i2c.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slave_protocol.h"
#include "peripherals/test_led.h"
#include "bool_array_converter.h"
#include "crc16.h"
#include "key_states.h"
#include "usb_report_updater.h"
#include "utils.h"
#include "keymap.h"
#include "debug.h"

uhk_module_state_t UhkModuleStates[UHK_MODULE_MAX_SLOT_COUNT];

static bool shouldResetTrackpoint = false;

uint8_t UhkModuleSlaveDriver_SlotIdToDriverId(uint8_t slotId)
{
    return slotId-1;
}

uint8_t UhkModuleSlaveDriver_DriverIdToSlotId(uint8_t uhkModuleDriverId)
{
    return uhkModuleDriverId+1;
}

void UhkModuleSlaveDriver_ResetTrackpoint()
{
    shouldResetTrackpoint = true;
}

static uint8_t keyStatesBuffer[MAX_KEY_COUNT_PER_MODULE];
static i2c_message_t txMessage;

static uhk_module_i2c_addresses_t moduleIdsToI2cAddresses[] = {
    { // UhkModuleDriverId_LeftKeyboardHalf
        .firmwareI2cAddress   = I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE,
        .bootloaderI2cAddress = I2C_ADDRESS_LEFT_KEYBOARD_HALF_BOOTLOADER
    },
    { // UhkModuleDriverId_LeftModule
        .firmwareI2cAddress   = I2C_ADDRESS_LEFT_MODULE_FIRMWARE,
        .bootloaderI2cAddress = I2C_ADDRESS_LEFT_MODULE_BOOTLOADER
    },
    { // UhkModuleDriverId_RightModule
        .firmwareI2cAddress   = I2C_ADDRESS_RIGHT_MODULE_FIRMWARE,
        .bootloaderI2cAddress = I2C_ADDRESS_RIGHT_MODULE_BOOTLOADER
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

    uhkModuleState->pointerDelta.x = 0;
    uhkModuleState->pointerDelta.y = 0;
}

// When module is swapped, we need to reload its Keymap once we know its
// moduleId. However, this triggers macro events, and we don't want to trigger
// the same macro multiple times if we can detect that another module is also
// going through the initialization sequence.
static void reloadKeymapIfNeeded()
{
    bool someoneElseWillDoTheJob = false;
    for (uint8_t i = 0; i < UHK_MODULE_MAX_SLOT_COUNT; i++) {
        uhk_module_state_t *uhkModuleState = UhkModuleStates + i;
        uhk_slave_t* slave = Slaves + i;

        someoneElseWillDoTheJob |= uhkModuleState->moduleId == 0 && slave->isConnected;
    }

    if (!someoneElseWillDoTheJob) {
        SwitchKeymapById(CurrentKeymapIndex);
    }
}

slave_result_t UhkModuleSlaveDriver_Update(uint8_t uhkModuleDriverId)
{
    slave_result_t res = { .status = kStatus_Uhk_IdleSlave, .hold = false };

    uhk_module_state_t *uhkModuleState = UhkModuleStates + uhkModuleDriverId;
    uhk_module_vars_t *uhkModuleSourceVars = &uhkModuleState->sourceVars;
    uhk_module_vars_t *uhkModuleTargetVars = &uhkModuleState->targetVars;
    uhk_module_phase_t *uhkModulePhase = &uhkModuleState->phase;
    uint8_t i2cAddress = uhkModuleState->firmwareI2cAddress;
    i2c_message_t *rxMessage = &uhkModuleState->rxMessage;

    switch (*uhkModulePhase) {

        // Jump to bootloader
        case UhkModulePhase_JumpToBootloader:
            txMessage.data[0] = SlaveCommand_JumpToBootloader;
            txMessage.length = 1;
            res.status = tx(i2cAddress);
            break;

        // Sync communication
        case UhkModulePhase_RequestSync:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_Sync;
            txMessage.length = 2;
            res.status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveSync;
            break;
        case UhkModulePhase_ReceiveSync:
            res.status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessSync;
            break;
        case UhkModulePhase_ProcessSync: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            bool isSyncValid = memcmp(rxMessage->data, SlaveSyncString, SLAVE_SYNC_STRING_LENGTH) == 0;
            res.status = kStatus_Uhk_IdleCycle;
            *uhkModulePhase = isSyncValid && isMessageValid
                ? UhkModulePhase_RequestModuleProtocolVersion
                : UhkModulePhase_RequestSync;
            break;
        }

        // Get module protocol version
        case UhkModulePhase_RequestModuleProtocolVersion:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_ModuleProtocolVersion;
            txMessage.length = 2;
            res.status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveModuleProtocolVersion;
            break;
        case UhkModulePhase_ReceiveModuleProtocolVersion:
            res.status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessModuleProtocolVersion;
            break;
        case UhkModulePhase_ProcessModuleProtocolVersion: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            if (isMessageValid) {
                memcpy(&uhkModuleState->moduleProtocolVersion, rxMessage->data, sizeof(version_t));
            }
            res.status = kStatus_Uhk_IdleCycle;
            *uhkModulePhase = isMessageValid ? UhkModulePhase_RequestFirmwareVersion : UhkModulePhase_RequestModuleProtocolVersion;
            break;
        }

        // Get firmware version
        case UhkModulePhase_RequestFirmwareVersion:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_FirmwareVersion;
            txMessage.length = 2;
            res.status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveFirmwareVersion;
            break;
        case UhkModulePhase_ReceiveFirmwareVersion:
            res.status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessFirmwareVersion;
            break;
        case UhkModulePhase_ProcessFirmwareVersion: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            if (isMessageValid) {
                memcpy(&uhkModuleState->firmwareVersion, rxMessage->data, sizeof(version_t));
            }
            res.status = kStatus_Uhk_IdleCycle;
            *uhkModulePhase = isMessageValid ? UhkModulePhase_RequestModuleId : UhkModulePhase_RequestFirmwareVersion;
            break;
        }

        // Get module id
        case UhkModulePhase_RequestModuleId:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_ModuleId;
            txMessage.length = 2;
            res.status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveModuleId;
            break;
        case UhkModulePhase_ReceiveModuleId:
            res.status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessModuleId;
            break;
        case UhkModulePhase_ProcessModuleId: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            if (isMessageValid) {
                uhkModuleState->moduleId = rxMessage->data[0];
                reloadKeymapIfNeeded();
            }
            res.status = kStatus_Uhk_IdleCycle;
            *uhkModulePhase = isMessageValid ? UhkModulePhase_RequestModuleKeyCount : UhkModulePhase_RequestModuleId;
            break;
        }

        // Get module key count
        case UhkModulePhase_RequestModuleKeyCount:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_KeyCount;
            txMessage.length = 2;
            res.status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveModuleKeyCount;
            break;
        case UhkModulePhase_ReceiveModuleKeyCount:
            res.status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessModuleKeyCount;
            break;
        case UhkModulePhase_ProcessModuleKeyCount: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            if (isMessageValid) {
                uhkModuleState->keyCount = rxMessage->data[0];
            }
            res.status = kStatus_Uhk_IdleCycle;
            *uhkModulePhase = isMessageValid ? UhkModulePhase_RequestModulePointerCount : UhkModulePhase_RequestModuleKeyCount;
            break;
        }

        // Get module pointer count
        case UhkModulePhase_RequestModulePointerCount:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_PointerCount;
            txMessage.length = 2;
            res.status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveModulePointerCount;
            break;
        case UhkModulePhase_ReceiveModulePointerCount:
            res.status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessModulePointerCount;
            break;
        case UhkModulePhase_ProcessModulePointerCount: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            if (isMessageValid) {
                uhkModuleState->pointerCount = rxMessage->data[0];
            }
            res.status = kStatus_Uhk_IdleCycle;

            if (!isMessageValid) {
                *uhkModulePhase = UhkModulePhase_RequestModulePointerCount;
            } else if (VERSION_AT_LEAST(uhkModuleState->moduleProtocolVersion, 4, 2, 0)) {
                *uhkModulePhase = UhkModulePhase_RequestGitTag;
            } else {
                uhkModuleState->gitTag[0] = '\0';
                uhkModuleState->gitRepo[0] = '\0';
                *uhkModulePhase = UhkModulePhase_RequestKeyStates;
            }
            break;
        }

        // Get module git tag
        case UhkModulePhase_RequestGitTag:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_GitTag;
            txMessage.length = 2;
            res.status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveGitTag;
            break;
        case UhkModulePhase_ReceiveGitTag:
            res.status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessGitTag;
            break;
        case UhkModulePhase_ProcessGitTag: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            if (isMessageValid) {
                Utils_SafeStrCopy(uhkModuleState->gitTag, (const char*)rxMessage->data, sizeof(uhkModuleState->gitTag));
            }
            res.status = kStatus_Uhk_IdleCycle;
            *uhkModulePhase = isMessageValid ? UhkModulePhase_RequestGitRepo : UhkModulePhase_RequestGitTag;
            break;
        }

        // Get module git repo
        case UhkModulePhase_RequestGitRepo:
            txMessage.data[0] = SlaveCommand_RequestProperty;
            txMessage.data[1] = SlaveProperty_GitRepo;
            txMessage.length = 2;
            res.status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_ReceiveGitRepo;
            break;
        case UhkModulePhase_ReceiveGitRepo:
            res.status = rx(rxMessage, i2cAddress);
            *uhkModulePhase = UhkModulePhase_ProcessGitRepo;
            break;
        case UhkModulePhase_ProcessGitRepo: {
            bool isMessageValid = CRC16_IsMessageValid(rxMessage);
            if (isMessageValid) {
                Utils_SafeStrCopy(uhkModuleState->gitRepo, (const char*)rxMessage->data, sizeof(uhkModuleState->gitRepo));
            }
            res.status = kStatus_Uhk_IdleCycle;
            *uhkModulePhase = isMessageValid ? UhkModulePhase_RequestKeyStates : UhkModulePhase_RequestGitRepo;
            break;
        }

        // Update loop start
        // Get key states
        case UhkModulePhase_RequestKeyStates:
            txMessage.data[0] = SlaveCommand_RequestKeyStates;
            txMessage.length = 1;
            res.status = tx(i2cAddress);
            res.hold = true;
            *uhkModulePhase = UhkModulePhase_ReceiveKeystates;
            break;
        case UhkModulePhase_ReceiveKeystates:
            res.status = rx(rxMessage, i2cAddress);
            res.hold = true;
            *uhkModulePhase = UhkModulePhase_ProcessKeystates;
            break;
        case UhkModulePhase_ProcessKeystates:
            if (CRC16_IsMessageValid(rxMessage)) {
                uint8_t slotId = UhkModuleSlaveDriver_DriverIdToSlotId(uhkModuleDriverId);
                BoolBitsToBytes(rxMessage->data, keyStatesBuffer, uhkModuleState->keyCount);
                for (uint8_t keyId=0; keyId < uhkModuleState->keyCount; keyId++) {
                    KeyStates[slotId][keyId].hardwareSwitchState = keyStatesBuffer[keyId];
                }
                if (uhkModuleState->pointerCount) {
                    uint8_t keyStatesLength = BOOL_BYTES_TO_BITS_COUNT(uhkModuleState->keyCount);
                    pointer_delta_t *pointerDelta = (pointer_delta_t*)(rxMessage->data + keyStatesLength);
                    uhkModuleState->pointerDelta.x += pointerDelta->x;
                    uhkModuleState->pointerDelta.y += pointerDelta->y;
                }
            }
            res.status = kStatus_Uhk_IdleCycle;
            res.hold = true;
            *uhkModulePhase = UhkModulePhase_SetTestLed;
            break;

        // Set test LED
        case UhkModulePhase_SetTestLed:
            if (uhkModuleSourceVars->isTestLedOn == uhkModuleTargetVars->isTestLedOn) {
                res.status = kStatus_Uhk_IdleCycle;
                res.hold = true;
            } else {
                txMessage.data[0] = SlaveCommand_SetTestLed;
                txMessage.data[1] = uhkModuleSourceVars->isTestLedOn;
                txMessage.length = 2;
                res.status = tx(i2cAddress);
                res.hold = true;
                uhkModuleTargetVars->isTestLedOn = uhkModuleSourceVars->isTestLedOn;
            }
            *uhkModulePhase = UhkModulePhase_SetLedPwmBrightness;
            break;

        // Set PWM brightness
        case UhkModulePhase_SetLedPwmBrightness:
            if (uhkModuleSourceVars->ledPwmBrightness == uhkModuleTargetVars->ledPwmBrightness) {
                res.status = kStatus_Uhk_IdleCycle;
                res.hold = false;
            } else {
                txMessage.data[0] = SlaveCommand_SetLedPwmBrightness;
                txMessage.data[1] = uhkModuleSourceVars->ledPwmBrightness;
                txMessage.length = 2;
                res.status = tx(i2cAddress);
                res.hold = false;
                uhkModuleTargetVars->ledPwmBrightness = uhkModuleSourceVars->ledPwmBrightness;
            }
            if (shouldResetTrackpoint && uhkModuleDriverId == UhkModuleDriverId_RightModule) {
                *uhkModulePhase = UhkModulePhase_ResetTrackpoint;
            } else {
                *uhkModulePhase = UhkModulePhase_RequestKeyStates;
            }
            break;

        // Other commands
        // Force reset
        case UhkModulePhase_ResetTrackpoint:
            shouldResetTrackpoint = false;
            txMessage.data[0] = SlaveCommand_ModuleSpecificCommand;
            txMessage.data[1] = ModuleSpecificCommand_ResetTrackpoint;
            txMessage.length = 2;
            res.status = tx(i2cAddress);
            *uhkModulePhase = UhkModulePhase_RequestKeyStates;
            break;
    }

    return res;
}

void UhkModuleSlaveDriver_Disconnect(uint8_t uhkModuleDriverId)
{
    if (uhkModuleDriverId == SlaveId_LeftKeyboardHalf) {
        Slaves[SlaveId_LeftLedDriver].isConnected = false;
    }

    uhk_module_state_t *uhkModuleState = UhkModuleStates + uhkModuleDriverId;
    uhkModuleState->moduleId = 0;
    uint8_t slotId = UhkModuleSlaveDriver_DriverIdToSlotId(uhkModuleDriverId);

    if (IS_VALID_MODULE_SLOT(slotId)) {
        memset(KeyStates[slotId], 0, MAX_KEY_COUNT_PER_MODULE * sizeof(key_state_t));
    }
}
