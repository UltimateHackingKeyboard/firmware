#include "config.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "slave_scheduler.h"
#include "led_display.h"
#include "device.h"
#include "ledmap.h"


bool LedsEnabled = true;
bool LedSleepModeActive = false;
float LedBrightnessMultiplier = 1.0f;

uint8_t KeyBacklightBrightness = 0xff;
uint8_t KeyBacklightBrightnessDefault = 0xff;
uint8_t LedDriverValues[LED_DRIVER_MAX_COUNT][LED_DRIVER_LED_COUNT_MAX];

#if DEVICE_ID == DEVICE_ID_UHK60V1
static uint8_t setShutdownModeNormalBufferIS31FL3731[] = {LED_DRIVER_REGISTER_SHUTDOWN, SHUTDOWN_MODE_NORMAL};
#endif
static uint8_t setShutdownModeNormalBufferIS31FL_3199_3737[] = {
    LED_DRIVER_REGISTER_CONFIGURATION,
    SHUTDOWN_MODE_NORMAL | (IS31FL3737B_PWM_FREQUENCY_26_7KHZ < IS31FL3737B_PWM_FREQUENCY_SHIFT)
};

static led_driver_state_t ledDriverStates[LED_DRIVER_MAX_COUNT] = {
#if DEVICE_ID == DEVICE_ID_UHK60V1
    {
        .i2cAddress = I2C_ADDRESS_IS31FL3731_RIGHT,
        .ledDriverIc = LedDriverIc_IS31FL3731,
        .frameRegisterPwmFirst = FRAME_REGISTER_PWM_FIRST_IS31FL3731,
        .ledCount = LED_DRIVER_LED_COUNT_IS31FL3731,
        .setShutdownModeNormalBufferLength = 2,
        .setShutdownModeNormalBuffer = setShutdownModeNormalBufferIS31FL3731,
        .setupLedControlRegistersCommandLength = LED_CONTROL_REGISTERS_COMMAND_LENGTH_IS31FL3731,
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
        .ledDriverIc = LedDriverIc_IS31FL3731,
        .frameRegisterPwmFirst = FRAME_REGISTER_PWM_FIRST_IS31FL3731,
        .ledCount = LED_DRIVER_LED_COUNT_IS31FL3731,
        .setShutdownModeNormalBufferLength = 2,
        .setShutdownModeNormalBuffer = setShutdownModeNormalBufferIS31FL3731,
        .setupLedControlRegistersCommandLength = LED_CONTROL_REGISTERS_COMMAND_LENGTH_IS31FL3731,
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
#elif DEVICE_ID == DEVICE_ID_UHK60V2
    {
        .i2cAddress = I2C_ADDRESS_IS31FL3737_RIGHT,
        .ledDriverIc = LedDriverIc_IS31FL3737,
        .frameRegisterPwmFirst = FRAME_REGISTER_PWM_FIRST_IS31FL3737,
        .ledCount = LED_DRIVER_LED_COUNT_IS31FL3737,
        .setShutdownModeNormalBufferLength = 2,
        .setShutdownModeNormalBuffer = setShutdownModeNormalBufferIS31FL_3199_3737,
        .setupLedControlRegistersCommandLength = LED_CONTROL_REGISTERS_COMMAND_LENGTH_IS31FL3737,
        .setupLedControlRegistersCommand = {
            FRAME_REGISTER_LED_CONTROL_FIRST,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
        }
    },
    {
        .i2cAddress = I2C_ADDRESS_IS31FL3737_LEFT,
        .ledDriverIc = LedDriverIc_IS31FL3737,
        .frameRegisterPwmFirst = FRAME_REGISTER_PWM_FIRST_IS31FL3737,
        .ledCount = LED_DRIVER_LED_COUNT_IS31FL3737,
        .setShutdownModeNormalBufferLength = 2,
        .setShutdownModeNormalBuffer = setShutdownModeNormalBufferIS31FL_3199_3737,
        .setupLedControlRegistersCommandLength = LED_CONTROL_REGISTERS_COMMAND_LENGTH_IS31FL3737,
        .setupLedControlRegistersCommand = {
            FRAME_REGISTER_LED_CONTROL_FIRST,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
            0b00111111,
        }
    },
#endif
    {
        .i2cAddress = I2C_ADDRESS_IS31FL3199_MODULE_LEFT,
        .ledDriverIc = LedDriverIc_IS31FL3199,
        .frameRegisterPwmFirst = FRAME_REGISTER_PWM_FIRST_IS31FL3199,
        .ledCount = LED_DRIVER_LED_COUNT_IS31FL3199,
        .setShutdownModeNormalBufferLength = 2,
        .setShutdownModeNormalBuffer = setShutdownModeNormalBufferIS31FL_3199_3737,
        .setupLedControlRegistersCommandLength = 0,
        .setupLedControlRegistersCommand = {}
    },
};

static uint8_t unlockCommandRegisterOnce[] = {LED_DRIVER_REGISTER_WRITE_LOCK, LED_DRIVER_WRITE_LOCK_ENABLE_ONCE};
static uint8_t setFunctionFrameBuffer[] = {LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_FUNCTION};
static uint8_t setGlobalCurrentBuffer[] = {LED_DRIVER_REGISTER_GLOBAL_CURRENT, 0xff};
static uint8_t setFrame1Buffer[] = {LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_1};
static uint8_t setFrame2Buffer[] = {LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_2};
static uint8_t setFrame4Buffer[] = {LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_4};
static uint8_t updateDataBuffer[] = {0x10, 0x00};
static uint8_t setLedBrightness[] = {0x04, 0b00110000};
static uint8_t updatePwmRegistersBuffer[PWM_REGISTER_BUFFER_LENGTH];

static void recalculateLedBrightness() {
    if (!LedsEnabled || LedSleepModeActive || SleepModeActive || LedBrightnessMultiplier == 0.0f) {
        KeyBacklightBrightness = 0;
        IconsAndLayerTextsBrightness = 0;
        AlphanumericSegmentsBrightness = 0;
    } else {
        KeyBacklightBrightness = MIN(255, KeyBacklightBrightnessDefault * LedBrightnessMultiplier);
        IconsAndLayerTextsBrightness = MIN(255, IconsAndLayerTextsBrightnessDefault * LedBrightnessMultiplier);
        AlphanumericSegmentsBrightness = MIN(255, AlphanumericSegmentsBrightnessDefault * LedBrightnessMultiplier);
    }
}

void LedSlaveDriver_DisableLeds(void)
{
    for (uint8_t ledDriverId=0; ledDriverId<=LedDriverId_Last; ledDriverId++) {
        memset(LedDriverValues[ledDriverId], 0, ledDriverStates[ledDriverId].ledCount);
    }
}

void LedSlaveDriver_UpdateLeds(void)
{
    recalculateLedBrightness();
    for (uint8_t ledDriverId=0; ledDriverId<=LedDriverId_Last; ledDriverId++) {
        memset(LedDriverValues[ledDriverId], KeyBacklightBrightness, ledDriverStates[ledDriverId].ledCount);
    }
    UpdateLayerLeds();
    LedDisplay_UpdateAll();
}

void LedSlaveDriver_Init(uint8_t ledDriverId)
{
    if (ledDriverId == ISO_KEY_LED_DRIVER_ID && IS_ISO) {
        ledDriverStates[LedDriverId_Left].setupLedControlRegistersCommand[ISO_KEY_CONTROL_REGISTER_POS] |= 1 << ISO_KEY_CONTROL_REGISTER_BIT;
    }

    led_driver_state_t *currentLedDriverState = ledDriverStates + ledDriverId;
    switch (currentLedDriverState->ledDriverIc) {
        case LedDriverIc_IS31FL3199:
            currentLedDriverState->phase = LedDriverPhase_SetShutdownModeNormal;
            break;
        case LedDriverIc_IS31FL3731:
            currentLedDriverState->phase = LedDriverPhase_SetFunctionFrame;
            break;
        case LedDriverIc_IS31FL3737:
            currentLedDriverState->phase = LedDriverPhase_UnlockCommandRegister1;
            break;
    }
    currentLedDriverState->ledIndex = 0;
}

status_t LedSlaveDriver_Update(uint8_t ledDriverId)
{
    status_t status = kStatus_Uhk_IdleSlave;
    uint8_t *ledValues = LedDriverValues[ledDriverId];
    led_driver_state_t *currentLedDriverState = ledDriverStates + ledDriverId;
    uint8_t *ledDriverPhase = &currentLedDriverState->phase;
    uint8_t ledDriverAddress = currentLedDriverState->i2cAddress;
    uint8_t ledCount = currentLedDriverState->ledCount;
    uint8_t frameRegisterPwmFirst = currentLedDriverState->frameRegisterPwmFirst;
    uint8_t *ledIndex = &currentLedDriverState->ledIndex;

    switch (*ledDriverPhase) {
        case LedDriverPhase_UnlockCommandRegister1:
            status = I2cAsyncWrite(ledDriverAddress, unlockCommandRegisterOnce, sizeof(unlockCommandRegisterOnce));
            *ledDriverPhase = LedDriverPhase_SetFunctionFrame;
            break;
        case LedDriverPhase_SetFunctionFrame:
            if (ledDriverId == LedDriverId_Left && !Slaves[SlaveId_LeftKeyboardHalf].isConnected) {
                break;
            }
            if (currentLedDriverState->ledDriverIc == LedDriverIc_IS31FL3731) {
                status = I2cAsyncWrite(ledDriverAddress, setFunctionFrameBuffer, sizeof(setFunctionFrameBuffer));
            } else {
                status = I2cAsyncWrite(ledDriverAddress, setFrame4Buffer, sizeof(setFrame4Buffer));
            }
            *ledDriverPhase = LedDriverPhase_SetShutdownModeNormal;
            break;
        case LedDriverPhase_SetShutdownModeNormal:
            status = I2cAsyncWrite(ledDriverAddress, currentLedDriverState->setShutdownModeNormalBuffer, currentLedDriverState->setShutdownModeNormalBufferLength);
            switch (currentLedDriverState->ledDriverIc) {
                case LedDriverIc_IS31FL3199:
                    *ledDriverPhase = LedDriverPhase_InitLedValues;
                    break;
                case LedDriverIc_IS31FL3731:
                    *ledDriverPhase = LedDriverPhase_SetFrame1;
                    break;
                case LedDriverIc_IS31FL3737:
                    *ledDriverPhase = LedDriverPhase_SetGlobalCurrent;
                    break;
            }
            break;
        case LedDriverPhase_SetGlobalCurrent:
            status = I2cAsyncWrite(ledDriverAddress, setGlobalCurrentBuffer, sizeof(setGlobalCurrentBuffer));
            *ledDriverPhase = LedDriverPhase_UnlockCommandRegister2;
            break;
        case LedDriverPhase_UnlockCommandRegister2:
            status = I2cAsyncWrite(ledDriverAddress, unlockCommandRegisterOnce, sizeof(unlockCommandRegisterOnce));
            *ledDriverPhase = LedDriverPhase_SetFrame1;
            break;
        case LedDriverPhase_SetFrame1:
            status = I2cAsyncWrite(ledDriverAddress, setFrame1Buffer, sizeof(setFrame1Buffer));
            *ledDriverPhase = LedDriverPhase_InitLedControlRegisters;
            break;
        case LedDriverPhase_InitLedControlRegisters:
            status = I2cAsyncWrite(ledDriverAddress, currentLedDriverState->setupLedControlRegistersCommand, currentLedDriverState->setupLedControlRegistersCommandLength);
            *ledDriverPhase = currentLedDriverState->ledDriverIc == LedDriverIc_IS31FL3731
                ? LedDriverPhase_InitLedValues
                : LedDriverPhase_UnlockCommandRegister3;
            break;
        case LedDriverPhase_UnlockCommandRegister3:
            status = I2cAsyncWrite(ledDriverAddress, unlockCommandRegisterOnce, sizeof(unlockCommandRegisterOnce));
            *ledDriverPhase = LedDriverPhase_SetFrame2;
            break;
        case LedDriverPhase_SetFrame2:
            status = I2cAsyncWrite(ledDriverAddress, setFrame2Buffer, sizeof(setFrame2Buffer));
            *ledDriverPhase = LedDriverPhase_InitLedValues;
            break;
        case LedDriverPhase_InitLedValues:
            updatePwmRegistersBuffer[0] = frameRegisterPwmFirst + *ledIndex;
            uint8_t chunkSize = MIN(ledCount - *ledIndex, PMW_REGISTER_UPDATE_CHUNK_SIZE);
            memcpy(updatePwmRegistersBuffer+1, ledValues + *ledIndex, chunkSize);
            status = I2cAsyncWrite(ledDriverAddress, updatePwmRegistersBuffer, chunkSize + 1);
            *ledIndex += chunkSize;
            if (*ledIndex >= ledCount) {
                *ledIndex = 0;
                memcpy(currentLedDriverState->targetLedValues, ledValues, ledCount);
                *ledDriverPhase = currentLedDriverState->ledDriverIc == LedDriverIc_IS31FL3199
                    ? LedDriverPhase_SetLedBrightness
                    : LedDriverPhase_UpdateChangedLedValues;
            }
            break;
        case LedDriverPhase_SetLedBrightness:
            status = I2cAsyncWrite(ledDriverAddress, setLedBrightness, sizeof(setLedBrightness));
            *ledDriverPhase = LedDriverPhase_UpdateChangedLedValues;
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
            if (!foundStartIndex && currentLedDriverState->ledDriverIc != LedDriverIc_IS31FL3199) {
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

            updatePwmRegistersBuffer[0] = frameRegisterPwmFirst + startLedIndex;
            uint8_t length = endLedIndex - startLedIndex + 1;
            memcpy(updatePwmRegistersBuffer+1, ledValues + startLedIndex, length);
            memcpy(currentLedDriverState->targetLedValues + startLedIndex, ledValues + startLedIndex, length);
            status = I2cAsyncWrite(ledDriverAddress, updatePwmRegistersBuffer, length+1);
            *ledIndex += length;
            if (*ledIndex >= ledCount) {
                *ledIndex = 0;
            }

            if (currentLedDriverState->ledDriverIc == LedDriverIc_IS31FL3199) {
                *ledDriverPhase = LedDriverPhase_UpdateData;
            }
            break;
        }
        case LedDriverPhase_UpdateData:
            status = I2cAsyncWrite(ledDriverAddress, updateDataBuffer, sizeof(updateDataBuffer));
            *ledDriverPhase = LedDriverPhase_UpdateChangedLedValues;
            break;
    }

    return status;
}
