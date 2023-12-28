#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/merge_sensor.h"
#include "device.h"

#ifdef DEVICE_HAS_MERGE_SENSE
const struct gpio_dt_spec mergeSenseDt = GPIO_DT_SPEC_GET(DT_ALIAS(merge_sense), gpios);
#endif

void InitMergeSensor(void) {
#ifdef DEVICE_HAS_MERGE_SENSE
    gpio_pin_configure_dt(&mergeSenseDt, GPIO_INPUT);
#endif
}
