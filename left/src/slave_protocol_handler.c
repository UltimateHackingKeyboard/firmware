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

i2c_message_t rxMessage;
i2c_message_t txMessage;

void SetError(uint8_t error);
void SetGenericError(void);
void SetResponseByte(uint8_t response);

void SetError(uint8_t error) {
    txMessage.data[0] = error;
}

void SetGenericError(void)
{
    SetError(PROTOCOL_RESPONSE_GENERIC_ERROR);
}

// Set a single byte as the response.
void SetResponseByte(uint8_t response)
{
    txMessage.data[1] = response;
}

void SlaveRxHandler(void)
{
    uint8_t commandId = rxMessage.data[0];
    switch (commandId) {
        case SlaveCommand_SetTestLed:
            txMessage.length = 0;
            bool isLedOn = rxMessage.data[1];
            TEST_LED_SET(isLedOn);
            break;
        case SlaveCommand_SetLedPwmBrightness:
            txMessage.length = 0;
            uint8_t brightnessPercent = rxMessage.data[1];
            LedPwm_SetBrightness(brightnessPercent);
            break;
        case SlaveCommand_JumpToBootloader:
            JumpToBootloader();
            break;
    }
}

void SlaveTxHandler(void)
{
    uint8_t commandId = rxMessage.data[0];
    switch (commandId) {
        case SlaveCommand_RequestKeyStates:
            BoolBytesToBits(keyMatrix.keyStates, txMessage.data, LEFT_KEYBOARD_HALF_KEY_COUNT);
            txMessage.length = KEY_STATE_SIZE;
            CRC16_UpdateMessageChecksum(&txMessage);
            break;
    }
}
