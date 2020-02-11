#include "fsl_gpio.h"
#include "module.h"
#include "module/test_led.h"

pointer_delta_t PointerDelta;

key_vector_t keyVector = {
    .itemNum = KEYBOARD_VECTOR_ITEMS_NUM,
    .items = (key_vector_pin_t[]) {
        {PORTB, GPIOB, kCLOCK_PortB,  5}, // left microswitch
        {PORTA, GPIOA, kCLOCK_PortA,  12}, // right microswitch
    },
};

void Module_Init(void)
{
    KeyVector_Init(&keyVector);

    CLOCK_EnableClock(PS2_CLOCK_CLOCK);
    PORT_SetPinConfig(PS2_CLOCK_PORT, PS2_CLOCK_PIN,
                      &(port_pin_config_t){/*.pullSelect=kPORT_PullDown,*/ .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});

    PORT_SetPinInterruptConfig(PS2_CLOCK_PORT, PS2_CLOCK_PIN, kPORT_InterruptEitherEdge);
    EnableIRQ(PS2_CLOCK_IRQ);
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput, .outputLogic=0});

    CLOCK_EnableClock(PS2_DATA_CLOCK);
    PORT_SetPinConfig(PS2_DATA_PORT, PS2_DATA_PIN,
                      &(port_pin_config_t){/*.pullSelect=kPORT_PullDown,*/ .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});

}

uint8_t phase = 0;
bool clockState;
uint32_t transitionCount = 1;
uint32_t upTransitionCount = 0;
uint32_t downTransitionCount = 0;

void requestToSend()
{
    for (volatile uint32_t i=0; i<150; i++);
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput});
    GPIO_WritePinOutput(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 0);
    for (volatile uint32_t i=0; i<150; i++);
    GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput});
    GPIO_WritePinOutput(PS2_DATA_GPIO, PS2_DATA_PIN, 0);
    for (volatile uint32_t i=0; i<150; i++);
    GPIO_WritePinOutput(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 1);
    GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});
}

uint8_t bitId = 0;
uint8_t buffer;

bool writeNextBit()
{
    static bool parityBit;
    bool isFinished = false;

    if (bitId == 10 && clockState == 0) {
        GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});
        isFinished = true;
        return isFinished;
    }

    if (clockState == 1) {
        return isFinished;
    }

    switch (bitId) {
        case 0 ... 7: {
            if (bitId == 0) {
                parityBit = 1;
                GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput});
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
    }

    bitId++;
    return isFinished;
}

bool readNextBit()
{
    bool isFinished = false;

    if (bitId == 11 && clockState == 0) {
        isFinished = true;
        return isFinished;
    }

    if (clockState == 1) {
        return isFinished;
    }

    switch (bitId) {
        case 0: {
            buffer = 0;
            break;
        }
        case 1 ... 8: {
            buffer <<= GPIO_ReadPinInput(PS2_DATA_GPIO, PS2_DATA_PIN);
            break;
        }
    }

    bitId++;
    return isFinished;
}

void PS2_CLOCK_IRQ_HANDLER(void) {
    static int8_t byte1 = 0;
    static int16_t deltaX = 0;
    static int16_t deltaY = 0;

    GPIO_ClearPinsInterruptFlags(PS2_CLOCK_GPIO, 1U << PS2_CLOCK_PIN);

    transitionCount++;
    clockState = GPIO_ReadPinInput(PS2_CLOCK_GPIO, PS2_CLOCK_PIN);
    if (clockState) {
        upTransitionCount++;
    } else {
        downTransitionCount++;
    }

    switch (phase) {
        case 0: {
            if (transitionCount == 44) {
                phase = 1;
            }
            break;
        }
        case 1: {
            requestToSend();
            bitId = 0;
            buffer = 0xff;
            phase = 2;
            break;
        }
        case 2: {
            if (writeNextBit()) {
                transitionCount = 0;
                phase = 3;
            }
            break;
        }
        case 3: {
            if (transitionCount == 66) {
                phase = 4;
            }
            break;
        }
        case 4: {
            requestToSend();
            bitId = 0;
            buffer = 0xf4;
            phase = 5;
        }
        case 5: {
            if (writeNextBit()) {
                bitId = 0;
                phase = 6;
            }
            break;
        }
        case 6: {
            if (readNextBit()) { // read ACK
                bitId = 0;
                phase = 7;
            }
            break;
        }
        case 7: {
            if (readNextBit()) {
                byte1 = buffer;
                bitId = 0;
                phase = 8;
            }
            break;
        }
        case 8: {
            if (readNextBit()) {
                deltaX = buffer;
                bitId = 0;
                phase = 9;
            }
            break;
        }
        case 9: {
            if (readNextBit()) {
                deltaY = buffer;
                if (byte1 & (1 << 3)) {
                    deltaX *= -1;
                }
                if (byte1 & (1 << 2)) {
                    deltaY *= -1;
                }
                PointerDelta.x = deltaX;
                PointerDelta.y = deltaY;
                bitId = 0;
                phase = 7;
                TestLed_Toggle();
            }
            break;
        }
    }
}

void Module_Loop(void)
{
}
