#include "bridge_protocol_handler.h"
#include "test_led.h"
#include "main.h"
#include "i2c_addresses.h"
#include "i2c.h"
#include "led_pwm.h"

void SetError(uint8_t error);
void SetGenericError();
void SetResponseByte(uint8_t response);

void SetError(uint8_t error) {
    BridgeTxBuffer[0] = error;
}

void SetGenericError()
{
    SetError(PROTOCOL_RESPONSE_GENERIC_ERROR);
}

// Set a single byte as the response.
void SetResponseByte(uint8_t response)
{
    BridgeTxBuffer[1] = response;
}

void BridgeProtocolHandler()
{
    uint8_t commandId = BridgeRxBuffer[0];
    switch (commandId) {
        case BRIDGE_COMMAND_GET_KEY_STATES:
            BridgeTxSize = KEYBOARD_MATRIX_COLS_NUM*KEYBOARD_MATRIX_ROWS_NUM;
            memcpy(BridgeTxBuffer, keyMatrix.keyStates, BridgeTxSize);
            break;
        case BRIDGE_COMMAND_SET_TEST_LED:
            TEST_LED_OFF();
            BridgeTxSize = 0;
            TEST_LED_SET(BridgeRxBuffer[1]);
            break;
        case BRIDGE_COMMAND_SET_LED_PWM:
            BridgeTxSize = 0;
            uint8_t brightnessPercent = BridgeRxBuffer[1];
            LedPwm_SetBrightness(brightnessPercent);
            break;
    }
}
