#include "bridge_slave_led_driver.h"
#include "led_driver.h"

#define BUFFER_SIZE (LED_DRIVER_LED_COUNT + 1)

uint8_t ledsBuffer[BUFFER_SIZE] = {FRAME_REGISTER_PWM_FIRST};
uint8_t ledDriverStates[2] = {0};
uint8_t buffer[LED_DRIVER_BUFFER_LENGTH];

uint8_t initLedControlRegistersMessage = {FRAME_REGISTER_LED_CONTROL_FIRST, };

uint8_t ledControlBufferLeft[] = {
    FRAME_REGISTER_LED_CONTROL_FIRST,
    0b01111111, // key row 1
    0b00111111, // display row 1
    0b01011111, // keys row 2
    0b00111111, // display row 2
    0b01011111, // keys row 3
    0b00111111, // display row 3
    0b01111101, // keys row 4
    0b00011111, // display row 4
    0b00101111, // keys row 5
    0b00011111, // display row 5
    0b00000000, // keys row 6
    0b00011111, // display row 6
    0b00000000, // keys row 7
    0b00011111, // display row 7
    0b00000000, // keys row 8
    0b00011111, // display row 8
    0b00000000, // keys row 9
    0b00011111, // display row 9
};

uint8_t ledControlBufferRight[] = {
    FRAME_REGISTER_LED_CONTROL_FIRST,
    0b01111111, // key row 1
    0b00000000, // no display
    0b01111111, // keys row 2
    0b00000000, // no display
    0b01111111, // keys row 3
    0b00000000, // no display
    0b01111111, // keys row 4
    0b00000000, // no display
    0b01111010, // keys row 5
    0b00000000, // no display
    0b00000000, // keys row 6
    0b00000000, // no display
    0b00000000, // keys row 7
    0b00000000, // no display
    0b00000000, // keys row 8
    0b00000000, // no display
    0b00000000, // keys row 9
    0b00000000, // no display
};

uint8_t setFunctionFrameBuffer[] = {LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_FUNCTION};
uint8_t setShutdownModeNormalBuffer[] = {LED_DRIVER_REGISTER_SHUTDOWN, SHUTDOWN_MODE_NORMAL};
uint8_t setFrame1Buffer[] = {LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_1};

bool BridgeSlaveLedDriverHandler(uint8_t ledDriverId) {
    uint8_t *ledDriverState = ledDriverStates + ledDriverId;
    uint8_t ledDriverAddress = ledDriverId ? I2C_ADDRESS_LED_DRIVER_LEFT : I2C_ADDRESS_LED_DRIVER_RIGHT;
    uint8_t *ledControlBuffer = ledDriverId ? ledControlBufferLeft : ledControlBufferRight;

    switch (*ledDriverState) {
        case LedDriverState_SetFunctionFrame:
            I2cAsyncWrite(ledDriverAddress, setFunctionFrameBuffer, sizeof(setFunctionFrameBuffer));
            *ledDriverState = LedDriverState_SetShutdownModeNormal;
            break;
        case LedDriverState_SetShutdownModeNormal:
            I2cAsyncWrite(ledDriverAddress, setShutdownModeNormalBuffer, sizeof(setShutdownModeNormalBuffer));
            *ledDriverState = LedDriverState_SetFrame1;
            break;
        case LedDriverState_SetFrame1:
            I2cAsyncWrite(ledDriverAddress, setFrame1Buffer, sizeof(setFrame1Buffer));
            *ledDriverState = LedDriverState_InitLedControlRegisters;
            break;
        case LedDriverState_InitLedControlRegisters:
            I2cAsyncWrite(ledDriverAddress, ledControlBuffer, sizeof(ledControlBufferLeft));
            *ledDriverState = LedDriverState_Initialized;
        break;
          case LedDriverState_Initialized:
            I2cAsyncWrite(I2C_ADDRESS_LED_DRIVER_LEFT, ledsBuffer, BUFFER_SIZE);
            break;
        }
    return true;
}

void SetLeds(uint8_t ledBrightness)
{
    memset(ledsBuffer+1, ledBrightness, LED_DRIVER_LED_COUNT);
}
