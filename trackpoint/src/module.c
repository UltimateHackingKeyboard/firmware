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

void PS2_CLOCK_IRQ_HANDLER(void) {
    GPIO_ClearPinsInterruptFlags(PS2_CLOCK_GPIO, 1U << PS2_CLOCK_PIN);

    transitionCount++;
    if (GPIO_ReadPinInput(PS2_CLOCK_GPIO, PS2_CLOCK_PIN)) {
        upTransitionCount++;
    } else {
        downTransitionCount++;
    }

    bool isLedOn = (transitionCount / 44) % 2;
    TestLed_Set(isLedOn);

    switch (phase) {
        case 0: {
            if (transitionCount == 44) {
                phase = 1;
            }
            break;
        }
        case 1: {
            requestToSend();
            phase = 2;
            break;
        }
        case 2: {
            GPIO_WritePinOutput(PS2_DATA_GPIO, PS2_DATA_PIN, 1);
            GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});
            upTransitionCount = 0;
            phase = 3;
            break;
        }
        case 3: {
            if (upTransitionCount == 9) {
                GPIO_WritePinOutput(PS2_DATA_GPIO, PS2_DATA_PIN, 0);
                GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});
                phase = 4;
            }
        }
    }
}

void Module_Loop(void)
{
}
