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

    CLOCK_EnableClock(PS2_DATA_CLOCK);
    PORT_SetPinConfig(PS2_DATA_PORT, PS2_DATA_PIN,
                      &(port_pin_config_t){/*.pullSelect=kPORT_PullDown,*/ .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});
}

uint32_t clockTransitionCounter = 0;
uint8_t prevClock = 1;
bool done = false;
void Module_Loop(void)
{
    uint8_t clock = GPIO_ReadPinInput(PS2_CLOCK_GPIO, PS2_CLOCK_PIN);
    if (prevClock != clock) {
        clockTransitionCounter++;
    }

    bool isLedOn = (clockTransitionCounter / 44) % 2;
    TestLed_Set(isLedOn);

    if (clockTransitionCounter == 46 && !done) {
        for (volatile uint32_t i=0; i<150; i++);
        GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput});
        GPIO_WritePinOutput(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 0);
        for (volatile uint32_t i=0; i<150; i++);
        GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput});
        GPIO_WritePinOutput(PS2_DATA_GPIO, PS2_DATA_PIN, 0);
        for (volatile uint32_t i=0; i<150; i++);
        GPIO_WritePinOutput(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, 1);
        GPIO_PinInit(PS2_CLOCK_GPIO, PS2_CLOCK_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});
        GPIO_PinInit(PS2_DATA_GPIO, PS2_DATA_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});
        done = true;
    }

    prevClock = clock;
}
