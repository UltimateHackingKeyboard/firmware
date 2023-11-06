#include "leds.h"
#include <zephyr/drivers/gpio.h>

const struct gpio_dt_spec ledsCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(leds_cs), gpios);
const struct gpio_dt_spec ledsSdbDt = GPIO_DT_SPEC_GET(DT_ALIAS(leds_sdb), gpios);

void InitLeds(void) {
    gpio_pin_configure_dt(&ledsCsDt, GPIO_OUTPUT);

    gpio_pin_configure_dt(&ledsSdbDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&ledsSdbDt, true);
}
