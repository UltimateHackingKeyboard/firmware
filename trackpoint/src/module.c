#include "fsl_gpio.h"
#include "module.h"
#include <stdint.h>
#include <stdbool.h>

#define TRACKPOINT_VERSION 1
#define MANUAL_RUN false

#if TRACKPOINT_VERSION == 1
#define NORESET 1
#elif TRACKPOINT_VERSION == 2
#define NORESET 0
#endif

pointer_delta_t PointerDelta;

static bool shouldReset = false;
static bool shouldRun = false;
static uint8_t resetTimer = 0;

typedef enum {
    Phase_Begin = 0,
    Phase_WriteReset = 1,
    Phase_AckReset = 2,
    Phase_WriteEnable = 3,
    Phase_AckEnable = 4,
    Phase_ReadByte1 = 5,
    Phase_ReadByte2 = 6,
    Phase_ReadByte3 = 7,
    Phase_Idle = 13,
} trackpoint_phase_t;

key_vector_t KeyVector = {
    .itemNum = KEYBOARD_VECTOR_ITEMS_NUM,
    .items = (key_vector_pin_t[]) {
        {PORTB, GPIOB, kCLOCK_PortB,  5}, // left microswitch
        {PORTA, GPIOA, kCLOCK_PortA,  12}, // right microswitch
    },
    .keyStates = {0}
};

void Module_Init(void)
{
    KeyVector_Init(&KeyVector);

    CLOCK_EnableClock(PS2_CLOCK_CLOCK);
    PORT_SetPinConfig(PS2_CLOCK_PORT, PS2_CLOCK_PIN,
                      &(port_pin_config_t){/*.pullSelect=kPORT_PullDown,*/ .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});

    PORT_SetPinInterruptConfig(PS2_CLOCK_PORT, PS2_CLOCK_PIN, kPORT_InterruptFallingEdge);
    EnableIRQ(PS2_CLOCK_IRQ);
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});

    CLOCK_EnableClock(PS2_DATA_CLOCK);
    PORT_SetPinConfig(PS2_DATA_PORT, PS2_DATA_PIN,
                      &(port_pin_config_t){/*.pullSelect=kPORT_PullDown,*/ .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});

    CLOCK_EnableClock(TP_RST_CLOCK);
    PORT_SetPinConfig(TP_RST_PORT, TP_RST_PIN, &(port_pin_config_t){.pullSelect=kPORT_PullDown, .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(TP_RST_GPIO, TP_RST_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=NORESET});
}

static void resetBoard()
{
    resetTimer = 50;
    GPIO_PinWrite(TP_RST_GPIO, TP_RST_PIN, 1-NORESET);
}

static bool runOrIdle() {
    return (!(MANUAL_RUN) || shouldRun);
}

uint8_t phase = Phase_Begin;
uint32_t transitionCount = 1;
uint8_t errno = 0;

void requestToSend()
{
    for (volatile uint32_t i=0; i<150; i++);
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
    GPIO_PinWrite(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 0);
    for (volatile uint32_t i=0; i<150; i++);
    GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
    GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, 0);
    for (volatile uint32_t i=0; i<150; i++);
    GPIO_PinWrite(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 1);
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});
}


uint8_t result;

typedef struct {
    bool bitValue;
    bool clockValue;
    bool parityBit;
    bool writingInProgress;
    uint8_t buffer;
    uint8_t bitId;

} ATTR_PACKED ps2_driver_state_t;

ps2_driver_state_t state = {
    .bitValue = 0,
    .clockValue = 0,
    .parityBit = 0,
    .writingInProgress = false,
    .buffer = 0,
    .bitId = 0,
};

static void reportError(uint8_t err) {
    errno |= err;
}

static bool writeByte2(uint8_t byte)
{
    if (!state.writingInProgress) {
        requestToSend();
        state.writingInProgress = true;
        state.buffer = byte;
        state.bitId = 0;
        return false;
    }

    if (state.clockValue) {
        // Even though we are hooked on InteruptFallingEdge, we are receiving
        // one spurious wakeup during the initiation sequence
        return false;
    }

    switch (state.bitId) {
        case 0 ... 7: {
            if (state.bitId == 0) {
                state.parityBit = 1;
                GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
            }
            bool dataBit = state.buffer & (1 << state.bitId);
            if (dataBit) {
                state.parityBit = !state.parityBit;
            }
            GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, dataBit);
            break;
        }
        case 8: {
            GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, state.parityBit);
            break;
        }
        case 9: {
            uint8_t stopBit = 1;
            GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, stopBit);
            break;
        }
        case 10: {
            GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});
            state.bitId = 0;
            state.writingInProgress = false;
            return true;
        }
    }

    state.bitId++;
    return false;
}

// Read a PS/2 byte from buffer bit by bit, and return true when finished.
static bool readByte(uint8_t* out)
{
    switch (state.bitId) {
        case 0: {
            state.buffer = 0;
            state.parityBit = 1;
            break;
        }
        case 1 ... 8: {
            state.parityBit ^= state.bitValue;
            state.buffer = state.buffer | (state.bitValue << (state.bitId-1));
            break;
        }
        case 9: {
            *out = state.buffer;
            if (state.parityBit != state.bitValue) {
                reportError(4);
            }
            break;
        }
        case 10: {
             state.bitId = 0;
             return true;
         }
    }

    state.bitId++;
    return false;
}

#define AXIS_COUNT 2
#define WINDOW_LENGTH 16
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define DRIFT_RESET_PERIOD 1000
#define TRACKPOINT_UPDATE_PERIOD 10
#define DRIFT_TOLERANCE 1

static uint16_t window[AXIS_COUNT][WINDOW_LENGTH];
static uint8_t windowIndex = 0;
static uint16_t windowSum[AXIS_COUNT];

void recognizeDrifts(int16_t x, int16_t y)
{
    uint16_t deltas[AXIS_COUNT] = {ABS(x), ABS(y)};

    // compute average speed across the window
    for (uint8_t axis=0; axis<AXIS_COUNT; axis++) {
        windowSum[axis] -= window[axis][windowIndex];
        window[axis][windowIndex] = deltas[axis];
        windowSum[axis] += window[axis][windowIndex];
    }

    windowIndex = (windowIndex + 1) % WINDOW_LENGTH;

    // check whether current speed matches remembered "drift" speed
    static uint16_t supposedDrift[AXIS_COUNT] = {0, 0};
    static uint16_t driftLength = 0;
    bool drifting = true;
    for (uint8_t axis=0; axis<AXIS_COUNT; axis++) {
        if (ABS(windowSum[axis] - supposedDrift[axis]) > DRIFT_TOLERANCE + 1) {
            drifting = false;
        }
    }

    if (windowSum[0] < 1 && windowSum[1] < 1) {
        drifting = false;
    }

    // handle drift detection logic
    if (drifting) {
        driftLength++;
        if (driftLength > DRIFT_RESET_PERIOD / TRACKPOINT_UPDATE_PERIOD) {
            driftLength = 0;
            shouldReset = true;
        }
    } else {
        driftLength = 0;
        for (uint8_t axis=0; axis<AXIS_COUNT; axis++) {
            supposedDrift[axis] = windowSum[axis];
        }
    }
}

void PS2_CLOCK_IRQ_HANDLER(void)
{
    static uint8_t byte1 = 0;
    static uint16_t deltaX = 0;
    static uint16_t deltaY = 0;
    static uint16_t lastX = 0;
    static uint16_t lastY = 0;
    static uint16_t lastClock;

    GPIO_PortClearInterruptFlags(PS2_CLOCK_GPIO, 1U << PS2_CLOCK_PIN);

    state.bitValue = GPIO_PinRead(PS2_CLOCK_GPIO, PS2_DATA_PIN);
    state.clockValue = GPIO_PinRead(PS2_CLOCK_GPIO, PS2_CLOCK_PIN);

    uint16_t currentClock = SysTick->VAL;
    uint16_t diff = lastClock - currentClock;
    lastClock = currentClock;

    if ((diff < 200 || diff > 240 ) && currentClock < lastClock && state.bitId > 0 && phase >= 7) {
        // In current configuration, SysTick value is internal clock divided by
        // 16, and *de*creasing by one by one. CPU clock frequency is 48Mhz.
        // Observed PS2 period is 220 SysTick ticks. This means one tick every
        // 73 us. This gives 13.6 kHz on PS2 clock.
        //
        // If timing is bad, perform rest of the handler as normally, but mark
        // it as corrupted.
        reportError(16);
    }

    switch (phase) {
        case Phase_Begin: {
            transitionCount++;
            if (transitionCount == 22) {
                phase = Phase_WriteReset;
            }
            break;
        }
        case Phase_WriteReset: {
             if (!runOrIdle()) {
                 phase = Phase_Idle;
                 break;
             }
             if(writeByte2(0xff)) {
                 transitionCount = 0;
                 phase = Phase_AckReset;
             }
            break;
        }
        case Phase_AckReset: {
            if (readByte(&result)) {
                phase = Phase_WriteEnable;
            }
            break;
        }
        case Phase_WriteEnable: {
            if (writeByte2(0xf4)) {
                phase = Phase_AckEnable;
            }
            break;
        }
        case Phase_AckEnable: {
            if (readByte(&result)) { // read ACK
                phase = Phase_ReadByte1;
                if ( result != 0xfa ) {
                    reportError(1);
                }
            }
            break;
        }
        case Phase_ReadByte1: {
            if (readByte(&result)) {
                if ((result & 0xc8) == 0x08) {
                    byte1 = result;
                    phase = Phase_ReadByte2;
                } else {
                    // If message format does not match the expected, assume
                    // the worst - falling out of sync with clock - and reset.
                    // In case of other errors, just ignore the corrupted frame
                    // and report the errno.
                    reportError(2);
                    phase = Phase_WriteReset;
                }
            }
            break;
        }
        case Phase_ReadByte2: {
            if (readByte(&result)) {
                deltaX = result;
                phase = Phase_ReadByte3;
            }
            break;
        }
        case Phase_ReadByte3: {
            if (readByte(&result)) {
                deltaY = result;
                if (byte1 & (1 << 4)) {
                    deltaX |= 0xff00;
                }
                if (byte1 & (1 << 5)) {
                    deltaY |= 0xff00;
                }
                if ( errno == 0 ) {
                    PointerDelta.x -= deltaX;
                    PointerDelta.y -= deltaY;
                    lastX = deltaX;
                    lastY = deltaY;

                    recognizeDrifts(deltaX, deltaY);
                } else {
                    // If there was error then current data is not relevant.
                    // We use the last delta as an approximation.
                    PointerDelta.x -= lastX;
                    PointerDelta.y -= lastY;
                }
                errno = 0;
                if (shouldReset) {
                    shouldReset = false;
                    resetBoard();
                    phase = Phase_WriteReset;
                } else {
                    phase = Phase_ReadByte1;
                }
            }
            break;
        }

        case Phase_Idle: {
             if (shouldRun) {
                 shouldRun = false;
                 requestToSend();
                 phase = Phase_WriteReset;
             }
             break;
         }
    }

    PointerDelta.debugInfo.phase = phase;

    SDK_ISR_EXIT_BARRIER;
}

void Module_Loop(void)
{
}

void Module_OnScan(void)
{
    // finish reset sequence
    if (resetTimer > 0 && --resetTimer == 0) {
        GPIO_PinWrite(TP_RST_GPIO, TP_RST_PIN, NORESET);
    }

}

void Module_ModuleSpecificCommand(module_specific_command_t command)
{
    switch (command) {
        case ModuleSpecificCommand_ResetTrackpoint:
            shouldReset = true;
            shouldRun = false;
            phase = Phase_WriteReset;
            resetBoard();
            break;
        case ModuleSpecificCommand_RunTrackpoint:
            shouldRun = true;
            shouldReset = false;
            phase = Phase_WriteReset;
            requestToSend();
            break;
    }
}
