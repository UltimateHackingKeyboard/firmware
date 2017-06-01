#include "slave_drivers/slave_driver_led_driver.h"
#include "slave_scheduler.h"

led_driver_state_t ledDriverStates[LED_DRIVER_MAX_COUNT] = {
    {
        .i2cAddress = I2C_ADDRESS_LED_DRIVER_RIGHT,
        .setupLedControlRegistersCommand = {
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
        }
    },
    {
        .i2cAddress = I2C_ADDRESS_LED_DRIVER_LEFT,
        .setupLedControlRegistersCommand = {
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
        }
    },
};

uint8_t setFunctionFrameBuffer[] = {LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_FUNCTION};
uint8_t setShutdownModeNormalBuffer[] = {LED_DRIVER_REGISTER_SHUTDOWN, SHUTDOWN_MODE_NORMAL};
uint8_t setFrame1Buffer[] = {LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_1};
uint8_t updatePwmRegistersBuffer[PWM_REGISTER_BUFFER_LENGTH];

void LedSlaveDriver_Init(uint8_t ledDriverId) {
    led_driver_state_t *currentLedDriverState = ledDriverStates + ledDriverId;
    currentLedDriverState->phase = LedDriverPhase_SetFunctionFrame;
    currentLedDriverState->ledIndex = 0;
    ledDriverStates[LedDriverId_Left].setupLedControlRegistersCommand[7] |= 0b00000010; // Enable the LED of the ISO key.
    SetLeds(0xff);
}

void LedSlaveDriver_Update(uint8_t ledDriverId) {
    led_driver_state_t *currentLedDriverState = ledDriverStates + ledDriverId;
    uint8_t *ledDriverPhase = &currentLedDriverState->phase;
    uint8_t ledDriverAddress = currentLedDriverState->i2cAddress;
    uint8_t *ledIndex = &currentLedDriverState->ledIndex;

    switch (*ledDriverPhase) {
        case LedDriverPhase_SetFunctionFrame:
            if (!Slaves[SlaveId_LeftKeyboardHalf].isConnected) {
                break;
            }
            I2cAsyncWrite(ledDriverAddress, setFunctionFrameBuffer, sizeof(setFunctionFrameBuffer));
            *ledDriverPhase = LedDriverPhase_SetShutdownModeNormal;
            break;
        case LedDriverPhase_SetShutdownModeNormal:
            I2cAsyncWrite(ledDriverAddress, setShutdownModeNormalBuffer, sizeof(setShutdownModeNormalBuffer));
            *ledDriverPhase = LedDriverPhase_SetFrame1;
            break;
        case LedDriverPhase_SetFrame1:
            I2cAsyncWrite(ledDriverAddress, setFrame1Buffer, sizeof(setFrame1Buffer));
            *ledDriverPhase = LedDriverPhase_InitLedControlRegisters;
            break;
        case LedDriverPhase_InitLedControlRegisters:
            I2cAsyncWrite(ledDriverAddress, currentLedDriverState->setupLedControlRegistersCommand, LED_CONTROL_REGISTERS_COMMAND_LENGTH);
            *ledDriverPhase = LedDriverPhase_Initialized;
            break;
        case LedDriverPhase_Initialized:
            updatePwmRegistersBuffer[0] = FRAME_REGISTER_PWM_FIRST + *ledIndex;
            memcpy(updatePwmRegistersBuffer+1, currentLedDriverState->ledValues + *ledIndex, PMW_REGISTER_UPDATE_CHUNK_SIZE);
            I2cAsyncWrite(ledDriverAddress, updatePwmRegistersBuffer, PWM_REGISTER_BUFFER_LENGTH);
            *ledIndex += PMW_REGISTER_UPDATE_CHUNK_SIZE;
            if (*ledIndex >= LED_DRIVER_LED_COUNT) {
                ledIndex = 0;
            }
            break;
    }
}

void SetLeds(uint8_t ledBrightness)
{
    for (uint8_t i=0; i<LED_DRIVER_MAX_COUNT; i++) {
        memset(&ledDriverStates[i].ledValues, ledBrightness, LED_DRIVER_LED_COUNT);
    }
}
