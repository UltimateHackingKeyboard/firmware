#include "attributes.h"
#include "fsl_gpio.h"
#include "slave_protocol.h"
#include "module.h"
#include <stdint.h>
#include <stdbool.h>
#include "uart.h"
#include "module/init_peripherals.h"

// TODO: After sorting out pullup values and refactors, it turns out that the v2 trackpoint works even with the v1 driver.
// However, the v2 driver is better abstracted, so if we want to unify the code, we should probably do so towards the v2 driver.

#if TRACKPOINT_VERSION == 1
#define NORESET 1
#elif TRACKPOINT_VERSION == 2
#define NORESET 0
#endif

// Drift macros
#define AXIS_COUNT 2
#define WINDOW_LENGTH 16
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define DRIFT_RESET_PERIOD 1000
#define TRACKPOINT_UPDATE_PERIOD 10
#define DRIFT_TOLERANCE 1

pointer_delta_t PointerDelta;

#if TRACKPOINT_VERSION == 1
typedef enum {
    Phase_ReadSelfTestResults = 0,
    Phase_WriteReset = 1,
    Phase_ReadResetAck,
    Phase_WriteEnable,
    Phase_ReadEnableAck,
    Phase_ReadByte1,
    Phase_ReadByte2,
    Phase_ReadByte3,
    Phase_Idle,
    Phase_Begin = Phase_ReadSelfTestResults,
    Phase_ResetBegin = Phase_WriteReset,
} trackpoint_phase_t;
#elif TRACKPOINT_VERSION == 2
typedef enum {
    Phase_WriteReset,
    Phase_ReadResetAck,
    Phase_ReadSelfTest1,
    Phase_ReadSelfTest2,
    Phase_WriteSetStreamingMode,
    Phase_ReadSetStreamingAck,
    Phase_WriteEnableStreaming,
    Phase_ReadEnableStreamingAck,
    Phase_ReadByte1,
    Phase_ReadByte2,
    Phase_ReadByte3,
    Phase_Idle,
    Phase_ResetBegin = Phase_WriteReset,
    Phase_Begin = Phase_ReadSelfTest1,
} trackpoint_phase_t;
#endif

typedef struct {
    uint16_t window[AXIS_COUNT][WINDOW_LENGTH];
    uint16_t windowSum[AXIS_COUNT];
    uint8_t windowIndex;

    uint16_t supposedDrift[AXIS_COUNT];
    uint16_t driftLength;
} ATTR_PACKED trackpoint_drift_state_t;

typedef struct {
    trackpoint_drift_state_t drift;
    uint32_t lastClock;
    uint32_t diffUs;
    uint16_t deltaX;
    uint16_t deltaY;
    uint16_t lastX;
    uint16_t lastY;
    uint8_t byte1;
    uint8_t errno;
    uint8_t serrno;
    uint8_t buffer;
    uint8_t bitId;
    uint8_t phase;
    uint8_t resetTimer;
    uint8_t waitCounter;
    bool bitValue;
    bool clockValue;
    bool parityBit;
    bool writingInProgress;
    bool shouldReset;
    bool shouldRun;
} ATTR_PACKED ps2_driver_state_t;

ps2_driver_state_t ps2State = { 0 };

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

    uint8_t pull = TRACKPOINT_VERSION == 1 ? kPORT_PullDisable : kPORT_PullUp;

    if (TRACKPOINT_VERSION == 1)
    CLOCK_EnableClock(PS2_CLOCK_CLOCK);
    PORT_SetPinConfig(PS2_CLOCK_PORT, PS2_CLOCK_PIN,
                      &(port_pin_config_t){.pullSelect=pull, .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});

    PORT_SetPinInterruptConfig(PS2_CLOCK_PORT, PS2_CLOCK_PIN, kPORT_InterruptFallingEdge);
    EnableIRQ(PS2_CLOCK_IRQ);
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});

    CLOCK_EnableClock(PS2_DATA_CLOCK);
    PORT_SetPinConfig(PS2_DATA_PORT, PS2_DATA_PIN,
                      &(port_pin_config_t){.pullSelect=pull, .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});

    CLOCK_EnableClock(TP_RST_CLOCK);
    PORT_SetPinConfig(TP_RST_PORT, TP_RST_PIN, &(port_pin_config_t){.pullSelect=kPORT_PullDown, .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(TP_RST_GPIO, TP_RST_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=NORESET});
}

static void resetBoard(ps2_driver_state_t* state)
{
    state->resetTimer = 250;
    GPIO_PinWrite(TP_RST_GPIO, TP_RST_PIN, 1-NORESET);
}

static bool runOrIdle(ps2_driver_state_t* state) {
    return (!(MANUAL_RUN) || state->shouldRun);
}

ATTR_UNUSED static uint32_t transitionCount = 1;

void simpleSignalData() {
        GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
        GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, 0);
        for (volatile uint32_t i=0; i<30; i++);
        GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, 1);
        for (volatile uint32_t i=0; i<10; i++);
}

void simpleSignalDataShort() {
        GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
        GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, 0);
        for (volatile uint32_t i=0; i<10; i++);
        GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, 1);
        for (volatile uint32_t i=0; i<10; i++);
}


void simpleSignalClock() {
        GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
        GPIO_PinWrite(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 0);
        for (volatile uint32_t i=0; i<30; i++);
        GPIO_PinWrite(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 1);
        for (volatile uint32_t i=0; i<10; i++);
}

void simpleSignalClockShort() {
        GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
        GPIO_PinWrite(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 0);
        for (volatile uint32_t i=0; i<10; i++);
        GPIO_PinWrite(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 1);
        for (volatile uint32_t i=0; i<10; i++);
}

static void requestToSend(ps2_driver_state_t* state)
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


    state->writingInProgress = true;
    state->bitId = 0;
}

static void reportError(ps2_driver_state_t* state, uint8_t err) {
    state->errno |= err;
    state->serrno |= err;
}

static bool writeByte(ps2_driver_state_t* state, uint8_t byte)
{
    if (!state->writingInProgress) {
        requestToSend(state);
        return false;
    }

    if (state->clockValue) {
        // Even though we are hooked on InteruptFallingEdge, we are receiving
        // one spurious wakeup during the initiation sequence with v1 trackpoint
        return false;
    }

    switch (state->bitId) {
        case 0 ... 7: {
            if (state->bitId == 0) {
                state->parityBit = 1;
                state->buffer = byte;
                GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
            }
            bool dataBit = state->buffer & (1 << state->bitId);
            if (dataBit) {
                state->parityBit = !state->parityBit;
            }
            GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, dataBit);
            break;
        }
        case 8: {
            GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, state->parityBit);
            break;
        }
        case 9: {
            uint8_t stopBit = 1;
            GPIO_PinWrite(PS2_DATA_GPIO, PS2_DATA_PIN, stopBit);
            break;
        }
        case 10: {
            GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});
            state->bitId = 0;
            state->writingInProgress = false;
            return true;
        }
    }

    state->bitId++;
    return false;
}

// Read a PS/2 byte from buffer bit by bit, and return true when finished.
static bool readByte(ps2_driver_state_t* state)
{
    if ((state->diffUs < 50 || state->diffUs > 100 ) && state->bitId > 0 && state->phase >= Phase_ReadByte1) {
        // If timing is bad, perform rest of the handler as normally, but mark the value as corrupted.
        reportError(state, 4);
    }


    switch (state->bitId) {
        case 0: {
            state->buffer = 0;
            state->parityBit = 1;
            break;
        }
        case 1 ... 8: {
            state->parityBit ^= state->bitValue;
            state->buffer = state->buffer | (state->bitValue << (state->bitId-1));
            break;
        }
        case 9: {
            if (state->parityBit != state->bitValue) {
                reportError(state, 16);
            }
            break;
        }
        case 10: {
            if (state->bitValue != 1) {
                reportError(state, 32);
            }
             state->bitId = 0;
             return true;
         }
    }

    state->bitId++;
    return false;
}

void recognizeDrifts(ps2_driver_state_t* state, int16_t x, int16_t y)
{
    trackpoint_drift_state_t* d = &state->drift;

    uint16_t deltas[AXIS_COUNT] = {ABS(x), ABS(y)};

    // compute average speed across the window
    for (uint8_t axis=0; axis<AXIS_COUNT; axis++) {
        d->windowSum[axis] -= d->window[axis][d->windowIndex];
        d->window[axis][d->windowIndex] = deltas[axis];
        d->windowSum[axis] += d->window[axis][d->windowIndex];
    }

    d->windowIndex = (d->windowIndex + 1) % WINDOW_LENGTH;

    // check whether current speed matches remembered "drift" speed
    bool drifting = true;
    for (uint8_t axis=0; axis<AXIS_COUNT; axis++) {
        if (ABS(d->windowSum[axis] - d->supposedDrift[axis]) > DRIFT_TOLERANCE + 1) {
            drifting = false;
        }
    }

    if (d->windowSum[0] < 1 && d->windowSum[1] < 1) {
        drifting = false;
    }

    // handle drift detection logic
    if (drifting) {
        d->driftLength++;
        if (d->driftLength > DRIFT_RESET_PERIOD / TRACKPOINT_UPDATE_PERIOD) {
            d->driftLength = 0;
            state->shouldReset = true;
        }
    } else {
        d->driftLength = 0;
        for (uint8_t axis=0; axis<AXIS_COUNT; axis++) {
            d->supposedDrift[axis] = d->windowSum[axis];
        }
    }
}

ATTR_UNUSED static void writeByteTransition(ps2_driver_state_t* state, uint8_t byte, trackpoint_phase_t backward, trackpoint_phase_t forward) {
    if (writeByte(state, byte)) {
        if (state->errno == 0) {
            state->phase = forward;
        } else {
            state->errno = 0;
            state->phase = backward;
            requestToSend(state);
        }
    }
}

ATTR_UNUSED static bool readByteTransition(ps2_driver_state_t* state, uint8_t byte, trackpoint_phase_t backward, trackpoint_phase_t forward, bool kickstart) {
    if (readByte(state)) {
        if (state->buffer == byte && state->errno == 0) {
            state->phase = forward;
            if (kickstart) {
                requestToSend(state);
            }
            return true;
        } else {
            state->errno = 0;
            state->phase = backward;
            requestToSend(state);
        }
    }
    return false;
}

static void processDeltas(ps2_driver_state_t* state) {
    if (state->byte1 & (1 << 4)) {
        state->deltaX |= 0xff00;
    }
    if (state->byte1 & (1 << 5)) {
        state->deltaY |= 0xff00;
    }
    if ( state->serrno == 0 ) {
        PointerDelta.x -= state->deltaX;
        PointerDelta.y -= state->deltaY;
        state->lastX = state->deltaX;
        state->lastY = state->deltaY;

        recognizeDrifts(state, state->deltaX, state->deltaY);
    } else {
        // If there was error then current data is not relevant.
        // We use the last delta as an approximation.
        PointerDelta.x -= state->lastX;
        PointerDelta.y -= state->lastY;
    }
    state->serrno = 0;
    if (state->shouldReset) {
        state->shouldReset = false;
        resetBoard(state);
        state->phase = Phase_ResetBegin;
    } else {
        state->phase = Phase_ReadByte1;
    }
    if (MODULE_OVER_UART) {
        ModuleUart_RequestKeyStatesUpdate();
    }
}

static void stateMachineTrackpoint1(ps2_driver_state_t* state) {
#if TRACKPOINT_VERSION == 1
    switch (state->phase) {
        case Phase_Begin: {
            transitionCount++;
            if (transitionCount == 22) {
                state->phase = Phase_WriteReset;
            }
            break;
        }
        case Phase_WriteReset: {
             if (!runOrIdle(state)) {
                 state->phase = Phase_Idle;
                 break;
             }
             if(writeByte(state, 0xff)) {
                 transitionCount = 0;
                 state->phase = Phase_ReadResetAck;
             }
            break;
        }
        case Phase_ReadResetAck: {
            if (readByte(state)) {
                state->phase = Phase_WriteEnable;
            }
            break;
        }
        case Phase_WriteEnable: {
            if (writeByte(state, 0xf4)) {
                state->phase = Phase_ReadEnableAck;
            }
            break;
        }
        case Phase_ReadEnableAck: {
            if (readByte(state)) { // read ACK
                state->phase = Phase_ReadByte1;
                if ( state->buffer != 0xfa ) {
                    reportError(state, 1);
                }
            }
            break;
        }
        case Phase_ReadByte1: {
            if (readByte(state)) {
                if ((state->buffer & 0xc8) == 0x08) {
                    state->byte1 = state->buffer;
                    state->phase = Phase_ReadByte2;
                } else {
                    // If message format does not match the expected, assume
                    // the worst - falling out of sync with clock - and reset.
                    // In case of other errors, just ignore the corrupted frame
                    // and report the errno.
                    reportError(state, 2);
                    state->phase = Phase_WriteReset;
                }
            }
            break;
        }
        case Phase_ReadByte2: {
            if (readByte(state)) {
                state->deltaX = state->buffer;
                state->phase = Phase_ReadByte3;
            }
            break;
        }
        case Phase_ReadByte3: {
            if (readByte(state)) {
                state->deltaY = state->buffer;
                processDeltas(state);
            }
            break;
        }

        case Phase_Idle: {
             break;
         }
    }
#endif
}

static void stateMachineTrackpoint2(ps2_driver_state_t* state) {
#if TRACKPOINT_VERSION == 2
    switch(state->phase) {
        case Phase_WriteReset:
            if (!runOrIdle(state)) {
                state->phase = Phase_Idle;
                break;
            }
            writeByteTransition(state, 0xff, Phase_WriteReset, Phase_ReadResetAck);
            break;
        case Phase_ReadResetAck:
            readByteTransition(state, 0xfa, Phase_WriteReset, Phase_ReadSelfTest1, false);
            break;
        case Phase_ReadSelfTest1:
            readByteTransition(state, 0xaa, Phase_WriteReset, Phase_ReadSelfTest2, false);
            break;
        case Phase_ReadSelfTest2:
            readByteTransition(state, 0x00, Phase_WriteReset, Phase_WriteSetStreamingMode, true);
            break;
        case Phase_WriteSetStreamingMode:
            writeByteTransition(state, 0xea, Phase_WriteSetStreamingMode, Phase_ReadSetStreamingAck);
            break;
        case Phase_ReadSetStreamingAck:
            readByteTransition(state, 0xfa, Phase_WriteSetStreamingMode, Phase_WriteEnableStreaming, true);
            break;
        case Phase_WriteEnableStreaming:
            writeByteTransition(state, 0xf4, Phase_WriteEnableStreaming, Phase_ReadEnableStreamingAck);
            break;
        case Phase_ReadEnableStreamingAck:
            readByteTransition(state, 0xfa, Phase_WriteEnableStreaming, Phase_ReadByte1, false);
            break;
        /**
         * ### Initialization complete; Beginning normal operation ###
         */
        case Phase_ReadByte1:
            if (readByte(state)) {
                if ((state->buffer & 0xc8) == 0x08) {
                    state->byte1 = state->buffer;
                    state->phase = Phase_ReadByte2;
                    state->errno = 0;
                    state->serrno = 0;
                } else {
                    state->phase = Phase_ReadByte1;
                }
            }
            break;
        case Phase_ReadByte2:
            if (readByte(state)) {
                state->deltaX = state->buffer;
                state->phase = Phase_ReadByte3;
            }
            break;

        case Phase_ReadByte3: {
            if (readByte(state)) {
                state->deltaY = state->buffer;
                processDeltas(state);
            }
            break;
        }

        /**
         * ### Auxiliary states
         */
        case Phase_Idle:
            break;
    }
#endif
}

void PS2_CLOCK_IRQ_HANDLER(void)
{
    ps2_driver_state_t* state = &ps2State;

    GPIO_PortClearInterruptFlags(PS2_CLOCK_GPIO, 1U << PS2_CLOCK_PIN);

    state->bitValue = GPIO_PinRead(PS2_CLOCK_GPIO, PS2_DATA_PIN);
    state->clockValue = GPIO_PinRead(PS2_CLOCK_GPIO, PS2_CLOCK_PIN);

    uint32_t now = SysTick->VAL;
    uint32_t diff = ((state->lastClock - now) & 0xFFFFFF)/3;
    state->lastClock = now;
    state->diffUs = diff;

    if (TRACKPOINT_VERSION == 1) {
        stateMachineTrackpoint1(state);
    } else if (TRACKPOINT_VERSION == 2) {
        stateMachineTrackpoint2(state);
    }

    SDK_ISR_EXIT_BARRIER;
}

ATTR_UNUSED static void writeNumber(uint8_t num) {
    simpleSignalData();

    if (num <= 4) {
        for (int j = 0; j < num; j++) {
            simpleSignalClockShort();
        }
    } else {
        uint8_t digit = 128;

        while (digit > num) {
            digit /= 2;
        }

        while (digit > 0) {
            if (num >= digit) {
                simpleSignalClock();
                num -= digit;
            } else {
                simpleSignalClockShort();
            }
            digit /= 2;
        }
    }
}

ATTR_UNUSED static void writeOutDebug() {
    // for (int i = 0; i < DEBUG_CNT; i++) {
    //     int dbg = debugOut[i];
    //     debugOut[i] = 0;
    //     writeNumber(dbg);
    // }
    // debugPos = 0;
}

void Module_Loop(void)
{
}

void Module_OnScan(void)
{
    // finish reset sequence
    if (ps2State.resetTimer > 0 && --ps2State.resetTimer == 0) {
        GPIO_PinWrite(TP_RST_GPIO, TP_RST_PIN, NORESET);
    }
}

static void resetState(ps2_driver_state_t* state) {
    state->phase = Phase_ResetBegin;
    state->bitId = 0;
    state->writingInProgress = false;
    state->errno = 0;
}

void Module_ModuleSpecificCommand(module_specific_command_t command)
{
    switch (command) {
        case ModuleSpecificCommand_ResetTrackpoint:
            ps2State.shouldReset = true;
            ps2State.shouldRun = false;
            resetState(&ps2State);
            resetBoard(&ps2State);
            break;
        case ModuleSpecificCommand_RunTrackpoint:
            ps2State.shouldRun = true;
            ps2State.shouldReset = false;
            resetState(&ps2State);
            requestToSend(&ps2State);
            break;
        case ModuleSpecificCommand_TrackpointSignalData:
            simpleSignalData();
            break;
        case ModuleSpecificCommand_TrackpointSignalClock:
            simpleSignalClock();
            break;
    }
}
