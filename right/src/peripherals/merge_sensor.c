#include <stdbool.h>
#include "merge_sensor.h"

#ifdef __ZEPHYR__
#include <zephyr/drivers/gpio.h>
#else
#include "fsl_gpio.h"
#include "fsl_port.h"

#define MERGE_SENSOR_GPIO        GPIOB
#define MERGE_SENSOR_PORT        PORTB
#define MERGE_SENSOR_CLOCK       kCLOCK_PortB
#define MERGE_SENSOR_PIN         3
#define MERGE_SENSOR_IRQ         PORTB_IRQn
#define MERGE_SENSOR_IRQ_HANDLER PORTB_IRQHandler
#endif

#if !(defined(__ZEPHYR__) && !defined(DEVICE_HAS_MERGE_SENSE))

#ifdef __ZEPHYR__
static const struct gpio_dt_spec mergeSenseDt = GPIO_DT_SPEC_GET(DT_ALIAS(merge_sense), gpios);
#endif

void MergeSensor_Init(void)
{
#ifdef __ZEPHYR__
    gpio_pin_configure_dt(&mergeSenseDt, GPIO_INPUT);
#else
    CLOCK_EnableClock(MERGE_SENSOR_CLOCK);
    PORT_SetPinConfig(MERGE_SENSOR_PORT, MERGE_SENSOR_PIN,
                      &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});
#endif
}

bool MergeSensor_IsMerged(void)
{
#ifdef __ZEPHYR__
    return gpio_pin_get_dt(&mergeSenseDt);
#else
    return !GPIO_ReadPinInput(MERGE_SENSOR_GPIO, MERGE_SENSOR_PIN);
#endif
}
#endif
