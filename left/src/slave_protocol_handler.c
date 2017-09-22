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

void SetError(uint8_t error);
void SetGenericError(void);
void SetResponseByte(uint8_t response);

void SetError(uint8_t error) {
    SlaveTxBuffer[0] = error;
}

void SetGenericError(void)
{
    SetError(PROTOCOL_RESPONSE_GENERIC_ERROR);
}

// Set a single byte as the response.
void SetResponseByte(uint8_t response)
{
    SlaveTxBuffer[1] = response;
}

void SlaveProtocolHandler(void)
{
    uint8_t commandId = SlaveRxBuffer[0];
    switch (commandId) {
        case SlaveCommand_RequestKeyStates:
            SlaveTxSize = KEY_STATE_BUFFER_SIZE;
            BoolBytesToBits(keyMatrix.keyStates, SlaveTxBuffer, LEFT_KEYBOARD_HALF_KEY_COUNT);
            CRC16_AppendToMessage(SlaveTxBuffer, KEY_STATE_SIZE);
            break;
        case SlaveCommand_SetTestLed:
            SlaveTxSize = 0;
//            bool isLedOn = SlaveRxBuffer[1];
//            TEST_LED_SET(isLedOn);
            break;
        case SlaveCommand_SetLedPwmBrightness:
            SlaveTxSize = 0;
            uint8_t brightnessPercent = SlaveRxBuffer[1];
            LedPwm_SetBrightness(brightnessPercent);
            break;
        case SlaveCommand_JumpToBootloader:
            JumpToBootloader();
            break;
    }
}
