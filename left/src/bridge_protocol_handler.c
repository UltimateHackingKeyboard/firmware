#include "bridge_protocol_handler.h"
#include "test_led.h"
#include "main.h"
#include "i2c_addresses.h"
#include "i2c.h"
#include "led_pwm.h"
#include "bridge_protocol.h"
#include "main.h"
#include "init_peripherials.h"

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
        case BridgeCommand_GetKeyStates:
            BridgeTxSize = KEYBOARD_MATRIX_COLS_NUM*KEYBOARD_MATRIX_ROWS_NUM;
            memcpy(BridgeTxBuffer, keyMatrix.keyStates, BridgeTxSize);
            break;
        case BridgeCommand_SetTestLed:
            BridgeTxSize = 0;
            bool isLedOn = BridgeRxBuffer[1];
            TEST_LED_SET(isLedOn);
            break;
        case BridgeCommand_SetLedPwmBrightness:
            BridgeTxSize = 0;
            uint8_t brightnessPercent = BridgeRxBuffer[1];
            LedPwm_SetBrightness(brightnessPercent);
            break;
        case BridgeCommand_SetDisableKeyMatrixScanState:
            BridgeTxSize = 0;
            DisableKeyMatrixScanState = BridgeRxBuffer[1];
            break;
        case BridgeCommand_SetDisableLedSdb:
            BridgeTxSize = 0;
            GPIO_WritePinOutput(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, BridgeRxBuffer[1] ? 0 : 1);
            break;
    }
}
