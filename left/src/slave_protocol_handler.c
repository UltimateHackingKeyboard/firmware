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

i2c_message_t RxMessage;
i2c_message_t TxMessage;

void SetError(uint8_t error);
void SetGenericError(void);
void SetResponseByte(uint8_t response);

void SetError(uint8_t error) {
    TxMessage.data[0] = error;
}

void SetGenericError(void)
{
    SetError(PROTOCOL_RESPONSE_GENERIC_ERROR);
}

// Set a single byte as the response.
void SetResponseByte(uint8_t response)
{
    TxMessage.data[1] = response;
}

void SlaveRxHandler(void)
{
    if (!CRC16_IsMessageValid(&RxMessage)) {
        TxMessage.length = 0;
        return;
    }

    uint8_t commandId = RxMessage.data[0];
    switch (commandId) {
        case SlaveCommand_JumpToBootloader:
            JumpToBootloader();
            break;
        case SlaveCommand_SetTestLed:
            TxMessage.length = 0;
            bool isLedOn = RxMessage.data[1];
            TEST_LED_SET(isLedOn);
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
                case SlaveProperty_ModuleId: {
                    TxMessage.data[0] = MODULE_ID;
                    TxMessage.length = 1;
                    break;
                }
                case SlaveProperty_ProtocolVersion: {
                    TxMessage.data[0] = MODULE_PROTOCOL_VERSION;
                    TxMessage.length = 1;
                    break;
                }
                case SlaveProperty_Features: {
                    uhk_module_features_t *moduleFeatures = (uhk_module_features_t*)&TxMessage.data;
                    moduleFeatures->keyCount = MODULE_KEY_COUNT;
                    moduleFeatures->hasPointer = MODULE_HAS_POINTER;
                    TxMessage.length = sizeof(uhk_module_features_t);
                    break;
                }
            }
            break;
        }
        case SlaveCommand_RequestKeyStates:
            BoolBytesToBits(keyMatrix.keyStates, TxMessage.data, MODULE_KEY_COUNT);
            TxMessage.length = BOOL_BYTES_TO_BITS_COUNT(MODULE_KEY_COUNT);
            break;
    }

    CRC16_UpdateMessageChecksum(&TxMessage);
}
