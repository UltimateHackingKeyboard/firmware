#include "slave_protocol_handler.h"
#include "atomicity.h"
#include "bool_array_converter.h"
#include "bootloader.h"
#include "i2c_addresses.h"
#include "module.h"
#include "module/i2c.h"
#include "module/init_peripherals.h"
#include "module/led_pwm.h"
#include "module/test_led.h"
#include "slave_protocol.h"
#include "versioning.h"
#include <string.h>
#include "atomicity.h"
#include <stdint.h>
#include <stdbool.h>

i2c_message_t RxMessage;
i2c_message_t TxMessage;

bool SlaveProtocol_FreeUpdateAllowed = false;

bool IsI2cRxTransaction(uint8_t commandId) {
    switch (commandId) {
        case SlaveCommand_JumpToBootloader:
        case SlaveCommand_SetTestLed:
        case SlaveCommand_SetLedPwmBrightness:
        case SlaveCommand_ModuleSpecificCommand:
            return true;
        case SlaveCommand_RequestProperty:
        case SlaveCommand_RequestKeyStates:
            return false;
    }
    return true;
}

void SlaveRxHandler(const uint8_t* data)
{
    uint8_t commandId = data[0];
    switch (commandId) {
    case SlaveCommand_JumpToBootloader:
        NVIC_SystemReset();
        break;
    case SlaveCommand_SetTestLed:
        TxMessage.length = 0;
        bool isLedOn = data[1];
        TestLed_Set(isLedOn);
        break;
    case SlaveCommand_SetLedPwmBrightness:
        TxMessage.length = 0;
        uint8_t brightnessPercent = data[1];
        LedPwm_SetBrightness(brightnessPercent);
        break;
    case SlaveCommand_ModuleSpecificCommand: {
        Module_ModuleSpecificCommand(data[1]);
        break;
    }
    }
}

void SlaveTxHandler(const uint8_t* rxData)
{
    uint8_t commandId = rxData[0];
    switch (commandId) {
    case SlaveCommand_RequestProperty: {
        SlaveProtocol_FreeUpdateAllowed = false;
        uint8_t propertyId = rxData[1];
        switch (propertyId) {
        case SlaveProperty_Sync: {
            memcpy(TxMessage.data, SlaveSyncString, SLAVE_SYNC_STRING_LENGTH);
            TxMessage.length = SLAVE_SYNC_STRING_LENGTH;
            break;
        }
        case SlaveProperty_ModuleProtocolVersion: {
            memcpy(TxMessage.data, &moduleProtocolVersion, sizeof(moduleProtocolVersion));
            TxMessage.length = sizeof(moduleProtocolVersion);
            break;
        }
        case SlaveProperty_FirmwareVersion: {
            memcpy(TxMessage.data, &firmwareVersion, sizeof(firmwareVersion));
            TxMessage.length = sizeof(firmwareVersion);
            break;
        }
        case SlaveProperty_ModuleId: {
            TxMessage.data[0] = MODULE_ID;
            TxMessage.length = 1;
            break;
        }
        case SlaveProperty_KeyCount: {
            TxMessage.data[0] = MODULE_KEY_COUNT;
            TxMessage.length = 1;
            break;
        }
        case SlaveProperty_PointerCount: {
            TxMessage.data[0] = MODULE_POINTER_COUNT;
            TxMessage.length = 1;
            break;
        }
        case SlaveProperty_GitTag: {
            size_t len = MIN(strlen(gitTag) + 1, sizeof(TxMessage.data));
            memcpy(TxMessage.data, gitTag, len);
            TxMessage.data[len - 1] = '\0';
            TxMessage.length = len;
            break;
        }
        case SlaveProperty_GitRepo: {
            size_t len = MIN(strlen(gitRepo) + 1, sizeof(TxMessage.data));
            memcpy(TxMessage.data, gitRepo, len);
            TxMessage.data[len - 1] = '\0';
            TxMessage.length = len;
            break;
        }
        case SlaveProperty_FirmwareChecksum: {
            memcpy(TxMessage.data, ModuleMD5Checksums[MODULE_ID], MD5_CHECKSUM_LENGTH);
            TxMessage.length = MD5_CHECKSUM_LENGTH;
            break;
        }
        }
        break;
    }
    case SlaveCommand_RequestKeyStates: {
#if KEY_ARRAY_TYPE == KEY_ARRAY_TYPE_VECTOR
        BoolBytesToBits(KeyVector.keyStates, TxMessage.data, MODULE_KEY_COUNT);
#elif KEY_ARRAY_TYPE == KEY_ARRAY_TYPE_MATRIX
        BoolBytesToBits(KeyMatrix.keyStates, TxMessage.data, MODULE_KEY_COUNT);
#endif
        uint8_t messageLength = BOOL_BYTES_TO_BITS_COUNT(MODULE_KEY_COUNT);
        if (MODULE_POINTER_COUNT) {
            pointer_delta_t *pointerDelta = (pointer_delta_t *)(TxMessage.data + messageLength);
            DISABLE_IRQ();
            // Gcc compiles those int16_t assignments as sequences of
            // single-byte instructions, therefore we need to make the
            // sequence atomic in order to prevent race conditions.
            // (This handler can be interrupted by sensor interrupts.)
            pointerDelta->x = PointerDelta.x;
            pointerDelta->y = PointerDelta.y;
            pointerDelta->debugInfo = PointerDelta.debugInfo;
            PointerDelta.x = 0;
            PointerDelta.y = 0;
            ENABLE_IRQ();
            messageLength += sizeof(pointer_delta_t);
        }
        SlaveProtocol_FreeUpdateAllowed = true;
        TxMessage.length = messageLength;
        break;
    }
    }

    if (!MODULE_OVER_UART) {
        CRC16_UpdateMessageChecksum(&TxMessage);
    }
}


