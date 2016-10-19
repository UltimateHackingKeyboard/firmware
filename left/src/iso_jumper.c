#include "iso_jumper.h"
#include "fsl_port.h"

void InitIsoJumper() {
    CLOCK_EnableClock(ISO_JUMPER_INPUT_CLOCK);
    PORT_SetPinConfig(ISO_JUMPER_INPUT_PORT, ISO_JUMPER_INPUT_PIN,
                      &(port_pin_config_t){.pullSelect=kPORT_PullDown, .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(ISO_JUMPER_INPUT_GPIO, ISO_JUMPER_INPUT_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});

    CLOCK_EnableClock(ISO_JUMPER_OUTPUT_CLOCK);
    PORT_SetPinConfig(ISO_JUMPER_OUTPUT_PORT, ISO_JUMPER_OUTPUT_PIN,
                      &(port_pin_config_t){.pullSelect=kPORT_PullDisable, .mux=kPORT_MuxAsGpio});
        GPIO_PinInit(ISO_JUMPER_OUTPUT_GPIO, ISO_JUMPER_OUTPUT_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});
}
