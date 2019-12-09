#include "slave_protocol_handler.h"
#include "test_led.h"
#include "main.h"
#include "i2c_addresses.h"
#include "i2c.h"
#include "led_pwm.h"
#include "slave_protocol.h"
#include "main.h"
#include "init_peripherals.h"
#include "bool_array_converter.h"
#include "bootloader.h"
#include "module.h"
#include "versions.h"
#include "trackball.h"

i2c_message_t RxMessage;
i2c_message_t TxMessage;

static version_t moduleProtocolVersion = {
    MODULE_PROTOCOL_MAJOR_VERSION,
    MODULE_PROTOCOL_MINOR_VERSION,
    MODULE_PROTOCOL_PATCH_VERSION,
};

static version_t firmwareVersion = {
    FIRMWARE_MAJOR_VERSION,
    FIRMWARE_MINOR_VERSION,
    FIRMWARE_PATCH_VERSION,
};

void SlaveRxHandler(void)
{
    if (!CRC16_IsMessageValid(&RxMessage)) {
        TxMessage.length = 0;
        return;
    }

    uint8_t commandId = RxMessage.data[0];
    switch (commandId) {
        case SlaveCommand_JumpToBootloader:
            NVIC_SystemReset();
            break;
        case SlaveCommand_SetTestLed:
            TxMessage.length = 0;
            bool isLedOn = RxMessage.data[1];
            TestLed_Set(isLedOn);
            break;
        case SlaveCommand_SetLedPwmBrightness:
            TxMessage.length = 0;
            uint8_t brightnessPercent = RxMessage.data[1];
            LedPwm_SetBrightness(brightnessPercent);
            break;
    }
}

void SlaveTxHandler(void)
{
    uint8_t commandId = RxMessage.data[0];
    switch (commandId) {
        case SlaveCommand_RequestProperty: {
            uint8_t propertyId = RxMessage.data[1];
            switch (propertyId) {
                case SlaveProperty_Sync: {
                    memcpy(TxMessage.data, SlaveSyncString, SLAVE_SYNC_STRING_LENGTH);
                    TxMessage.length = SLAVE_SYNC_STRING_LENGTH;
                    break;
                }
                case SlaveProperty_ModuleProtocolVersion: {
                    memcpy(TxMessage.data, &moduleProtocolVersion, sizeof(version_t));
                    TxMessage.length = sizeof(version_t);
                    break;
                }
                case SlaveProperty_FirmwareVersion: {
                    memcpy(TxMessage.data, &firmwareVersion, sizeof(version_t));
                    TxMessage.length = sizeof(version_t);
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
            }
            break;
        }
        case SlaveCommand_RequestKeyStates:
            BoolBytesToBits(keyVector.keyStates, TxMessage.data, MODULE_KEY_COUNT);
            uint8_t messageLength = BOOL_BYTES_TO_BITS_COUNT(MODULE_KEY_COUNT);
            if (MODULE_POINTER_COUNT) {
                pointer_delta_t *pointerDelta = (pointer_delta_t*)(TxMessage.data + messageLength);
                pointerDelta->x = BlackBerryTrackball_PointerDelta.x;
                pointerDelta->y = BlackBerryTrackball_PointerDelta.y;
                BlackBerryTrackball_PointerDelta.x = 0;
                BlackBerryTrackball_PointerDelta.y = 0;
                messageLength += sizeof(pointer_delta_t);
            }
            TxMessage.length = messageLength;
            break;
    }

    CRC16_UpdateMessageChecksum(&TxMessage);
}
