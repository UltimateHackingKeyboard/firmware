#include "fsl_gpio.h"
#include "key_vector.h"

void KeyVector_Init(key_vector_t *keyVector)
{
    for (key_vector_pin_t *item = keyVector->items; item < keyVector->items + keyVector->itemNum; item++) {
        CLOCK_EnableClock(item->clock);
        PORT_SetPinConfig(item->port, item->pin,
                          &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});
        GPIO_PinInit(item->gpio, item->pin, &(gpio_pin_config_t){kGPIO_DigitalInput});
    }
}

void KeyVector_Scan(key_vector_t *keyVector)
{
    uint8_t *keyState = keyVector->keyStates;
    for (key_vector_pin_t *item = keyVector->items; item < keyVector->items + keyVector->itemNum; item++) {
        *(keyState++) = !GPIO_ReadPinInput(item->gpio, item->pin);
    }
}
