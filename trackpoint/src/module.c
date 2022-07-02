#include "fsl_gpio.h"
#include "module.h"

pointer_delta_t PointerDelta;

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
    GPIO_PinInit(TP_RST_GPIO, TP_RST_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=1});
}

bool shouldReset = false;

void resetBoard()
{
    GPIO_WritePinOutput(TP_RST_GPIO, TP_RST_PIN, 0);

    for (volatile uint32_t i=0; i<10000; i++);

    GPIO_WritePinOutput(TP_RST_GPIO, TP_RST_PIN, 1);
}

void Module_ResetTrackpoint(void)
{
    shouldReset = true;
}

uint8_t phase = 0;
uint32_t transitionCount = 1;
uint8_t errno = 0;

void requestToSend()
{
    for (volatile uint32_t i=0; i<150; i++);
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
    GPIO_WritePinOutput(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 0);
    for (volatile uint32_t i=0; i<150; i++);
    GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
    GPIO_WritePinOutput(PS2_DATA_GPIO, PS2_DATA_PIN, 0);
    for (volatile uint32_t i=0; i<150; i++);
    GPIO_WritePinOutput(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 1);
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});
}


bool clockValue = 0;
bool bitValue = 0;
uint8_t bitId = 0;
uint8_t buffer;

static void reportError(uint8_t err) {
    errno |= err;
}

// Write a PS/2 byte to buffer bit by bit, and return true when finished.
static bool writeByte()
{
    static bool parityBit;

    if (clockValue) {
        // Even though we are hooked on InteruptFallingEdge, we are receiving
        // one spurious wakeup during the initiation sequence
        return false;
    }

    switch (bitId) {
        case 0 ... 7: {
            if (bitId == 0) {
                parityBit = 1;
                GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=0});
            }
            bool dataBit = buffer & (1 << bitId);
            if (dataBit) {
                parityBit = !parityBit;
            }
            GPIO_WritePinOutput(PS2_DATA_GPIO, PS2_DATA_PIN, dataBit);
            break;
        }
        case 8: {
            GPIO_WritePinOutput(PS2_DATA_GPIO, PS2_DATA_PIN, parityBit);
            break;
        }
        case 9: {
            uint8_t stopBit = 1;
            GPIO_WritePinOutput(PS2_DATA_GPIO, PS2_DATA_PIN, stopBit);
            break;
        }
        case 10: {
            GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});
            bitId = 0;
            return true;
        }
    }

    bitId++;
    return false;
}

// Read a PS/2 byte from buffer bit by bit, and return true when finished.
static bool readByte()
{
    static bool parityBit = 1;

    switch (bitId) {
        case 0: {
            buffer = 0;
            parityBit = 1;
            break;
        }
        case 1 ... 8: {
            parityBit ^= bitValue;
            buffer = buffer | (bitValue << (bitId-1));
            break;
        }
        case 9: {
            if (parityBit != bitValue) {
                reportError(4);
            }
            break;
        }
        case 10: {
             bitId = 0;
             return true;
         }
    }

    bitId++;
    return false;
}

int16_t errorCode = 0;

#define RQ_TO_SEND(AT)                  \
        case AT: {                      \
            requestToSend();            \
            phase = AT+1;               \
            break;                      \
        }

#define WRITE_BYTE(AT, BYTE)            \
        case AT: {                      \
            buffer = BYTE;              \
            if (writeByte()) {          \
                phase = AT+1;           \
            }                           \
            break;                      \
        }                               \


#define READ_BYTE(AT, BYTE)             \
        case AT: {                      \
            if (readByte()) {           \
                phase = AT+1;           \
                if (buffer != BYTE) {   \
                    phase = 1;          \
                    reportError(1);     \
                }                       \
            }                           \
            break;                      \
        }


void PS2_CLOCK_IRQ_HANDLER(void) {
    static uint8_t byte1 = 0;
    static uint16_t deltaX = 0;
    static uint16_t deltaY = 0;
    static uint16_t lastX = 0;
    static uint16_t lastY = 0;
    static uint16_t lastClock;

    GPIO_ClearPinsInterruptFlags(PS2_CLOCK_GPIO, 1U << PS2_CLOCK_PIN);

    bitValue = GPIO_ReadPinInput(PS2_CLOCK_GPIO, PS2_DATA_PIN);
    clockValue = GPIO_ReadPinInput(PS2_CLOCK_GPIO, PS2_CLOCK_PIN);

    uint16_t currentClock = SysTick->VAL;
    uint16_t diff = lastClock - currentClock;
    lastClock = currentClock;

    if ((diff < 200 || diff > 240 ) && currentClock < lastClock && bitId > 0 && phase >= 7) {
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
        // disregard first two bytes
        case 0: {
            transitionCount++;
            if (transitionCount == 22) {
                phase = 1;
            }
            break;
        }

        // Issue reset command.
        // (datasheet claims there should be 2, so we probably have wrong datasheet...)
        RQ_TO_SEND(1);
        WRITE_BYTE(2, 0xff); //Reset
        READ_BYTE(3, 0xfa); //ACK

        // Request switch to streaming, and confirm receiving ACK
        case 24:
        RQ_TO_SEND(4);
        WRITE_BYTE(5, 0xf4); //Enable
        READ_BYTE(6, 0xfa); //ACK

        // Listen for streamed data. Each packet consists of 3 bytes:
        // - status data
        // - x delta
        // - y delta
        case 7: {
            if (readByte()) {
                if ((buffer & 0xc8) == 0x08) {
                    byte1 = buffer;
                    phase = 8;
                } else {
                    // If message format does not match the expected, assume
                    // the worst - falling out of sync with clock - and reset.
                    // In case of other errors, just ignore the corrupted frame
                    // and report the errno.
                    reportError(2);
                    phase = 1;
                }
            }
            break;
        }
        case 8: {
            if (readByte()) {
                deltaX = buffer;
                phase = 9;
            }
            break;
        }
        case 9: {
            if (readByte()) {
                deltaY = buffer;
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
                } else {
                    PointerDelta.x -= lastX;
                    PointerDelta.y -= lastY;
                }
                errno = 0;
                if (shouldReset) {
                    shouldReset = false;
                    resetBoard();
                    phase = 1;

                } else {
                    phase = 7;
                }
            }
            break;
        }
    }
}

void Module_Loop(void)
{
}
