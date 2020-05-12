#include "config.h"
#include "slave_drivers/is31fl37xx_driver.h"
#include "slave_scheduler.h"
#include "led_display.h"
#include "device.h"

uint8_t KeyBacklightBrightness = 0xff;
uint8_t LedDriverValues[LED_DRIVER_MAX_COUNT][LED_DRIVER_LED_COUNT_MAX];

static led_driver_state_t ledDriverStates[LED_DRIVER_MAX_COUNT] = {
    {
        .i2cAddress = I2C_ADDRESS_IS31FL3731_RIGHT,
        .ledCount = LED_DRIVER_LED_COUNT_IS31FL3731,
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
        .i2cAddress = I2C_ADDRESS_IS31FL3731_LEFT,
        .ledCount = LED_DRIVER_LED_COUNT_IS31FL3731,
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

static uint8_t setFunctionFrameBuffer[] = {LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_FUNCTION};
static uint8_t setShutdownModeNormalBuffer[] = {LED_DRIVER_REGISTER_SHUTDOWN, SHUTDOWN_MODE_NORMAL};
static uint8_t setFrame1Buffer[] = {LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_1};
static uint8_t updatePwmRegistersBuffer[PWM_REGISTER_BUFFER_LENGTH];

void LedSlaveDriver_DisableLeds(void)
{
    for (uint8_t ledDriverId=0; ledDriverId<=LedDriverId_Last; ledDriverId++) {
        memset(LedDriverValues[ledDriverId], 0, ledDriverStates[ledDriverId].ledCount);
    }
}

void LedSlaveDriver_UpdateLeds(void)
{
    for (uint8_t ledDriverId=0; ledDriverId<=LedDriverId_Last; ledDriverId++) {
        memset(LedDriverValues[ledDriverId], KeyBacklightBrightness, ledDriverStates[ledDriverId].ledCount);
    }

    LedDisplay_UpdateAll();
}

void LedSlaveDriver_Init(uint8_t ledDriverId)
{
    if (ledDriverId == ISO_KEY_LED_DRIVER_ID && IS_ISO) {
        ledDriverStates[LedDriverId_Left].setupLedControlRegistersCommand[ISO_KEY_CONTROL_REGISTER_POS] |= 1 << ISO_KEY_CONTROL_REGISTER_BIT;
    }

    led_driver_state_t *currentLedDriverState = ledDriverStates + ledDriverId;
    currentLedDriverState->phase = LedDriverPhase_SetFunctionFrame;
    currentLedDriverState->ledIndex = 0;
    memset(LedDriverValues[ledDriverId], KeyBacklightBrightness, currentLedDriverState->ledCount);

    if (ledDriverId == LedDriverId_Left) {
        LedDisplay_UpdateAll();
    }
}

status_t LedSlaveDriver_Update(uint8_t ledDriverId)
{
    status_t status = kStatus_Uhk_IdleSlave;
    uint8_t *ledValues = LedDriverValues[ledDriverId];
    led_driver_state_t *currentLedDriverState = ledDriverStates + ledDriverId;
    uint8_t *ledDriverPhase = &currentLedDriverState->phase;
    uint8_t ledDriverAddress = currentLedDriverState->i2cAddress;
    uint8_t ledCount = currentLedDriverState->ledCount;
    uint8_t *ledIndex = &currentLedDriverState->ledIndex;

    switch (*ledDriverPhase) {
        case LedDriverPhase_SetFunctionFrame:
            if (ledDriverId == LedDriverId_Left && !Slaves[SlaveId_LeftKeyboardHalf].isConnected) {
                break;
            }
            status = I2cAsyncWrite(ledDriverAddress, setFunctionFrameBuffer, sizeof(setFunctionFrameBuffer));
            *ledDriverPhase = LedDriverPhase_SetShutdownModeNormal;
            break;
        case LedDriverPhase_SetShutdownModeNormal:
            status = I2cAsyncWrite(ledDriverAddress, setShutdownModeNormalBuffer, sizeof(setShutdownModeNormalBuffer));
            *ledDriverPhase = LedDriverPhase_SetFrame1;
            break;
        case LedDriverPhase_SetFrame1:
            status = I2cAsyncWrite(ledDriverAddress, setFrame1Buffer, sizeof(setFrame1Buffer));
            *ledDriverPhase = LedDriverPhase_InitLedControlRegisters;
            break;
        case LedDriverPhase_InitLedControlRegisters:
            status = I2cAsyncWrite(ledDriverAddress, currentLedDriverState->setupLedControlRegistersCommand, LED_CONTROL_REGISTERS_COMMAND_LENGTH);
            *ledDriverPhase = LedDriverPhase_InitLedValues;
            break;
        case LedDriverPhase_InitLedValues:
            updatePwmRegistersBuffer[0] = FRAME_REGISTER_PWM_FIRST + *ledIndex;
            uint8_t chunkSize = MIN(ledCount - *ledIndex, PMW_REGISTER_UPDATE_CHUNK_SIZE);
            memcpy(updatePwmRegistersBuffer+1, ledValues + *ledIndex, chunkSize);
            status = I2cAsyncWrite(ledDriverAddress, updatePwmRegistersBuffer, chunkSize + 1);
            *ledIndex += chunkSize;
            if (*ledIndex >= ledCount) {
                *ledIndex = 0;
                memcpy(currentLedDriverState->targetLedValues, ledValues, ledCount);
                *ledDriverPhase = LedDriverPhase_UpdateChangedLedValues;
            }
            break;
        case LedDriverPhase_UpdateChangedLedValues: {
            uint8_t *targetLedValues = currentLedDriverState->targetLedValues;

            uint8_t lastLedChunkStartIndex = ledCount - PMW_REGISTER_UPDATE_CHUNK_SIZE;
            uint8_t startLedIndex = *ledIndex > lastLedChunkStartIndex ? lastLedChunkStartIndex : *ledIndex;

            uint8_t count;
            for (count=0; count<ledCount; count++) {
                if (ledValues[startLedIndex] != targetLedValues[startLedIndex]) {
                    break;
                }

                if (++startLedIndex >= ledCount) {
                    startLedIndex = 0;
                }
            }

            bool foundStartIndex = count < ledCount;
            if (!foundStartIndex) {
                *ledIndex = 0;
                break;
            }

            uint8_t maxChunkSize = MIN(ledCount - startLedIndex, PMW_REGISTER_UPDATE_CHUNK_SIZE);
            uint8_t maxEndLedIndex = startLedIndex + maxChunkSize - 1;
            uint8_t endLedIndex = startLedIndex;
            for (uint8_t index=startLedIndex; index<=maxEndLedIndex; index++) {
                if (ledValues[index] != targetLedValues[index]) {
                    endLedIndex = index;
                }
            }

            updatePwmRegistersBuffer[0] = FRAME_REGISTER_PWM_FIRST + startLedIndex;
            uint8_t length = endLedIndex - startLedIndex + 1;
            memcpy(updatePwmRegistersBuffer+1, ledValues + startLedIndex, length);
            memcpy(currentLedDriverState->targetLedValues + startLedIndex, ledValues + startLedIndex, length);
            status = I2cAsyncWrite(ledDriverAddress, updatePwmRegistersBuffer, length+1);
            *ledIndex += length;
            if (*ledIndex >= ledCount) {
                *ledIndex = 0;
            }
            break;
        }
    }

    return status;
}
