#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include "keyboard/charger.h"
#include "shell.h"

const struct gpio_dt_spec chargerEnDt = GPIO_DT_SPEC_GET(DT_ALIAS(charger_en), gpios);
const struct gpio_dt_spec chargerStatDt = GPIO_DT_SPEC_GET(DT_ALIAS(charger_stat), gpios);
struct gpio_callback callbackStruct;
const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

void chargerStatCallback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
    if (Shell.statLog) {
        printk("STAT changed to %i\n", gpio_pin_get_dt(&chargerStatDt) ? 1 : 0);
    }
}

void InitCharger(void) {
    gpio_pin_configure_dt(&chargerEnDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&chargerEnDt, true);

    gpio_pin_configure_dt(&chargerStatDt, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&chargerStatDt, GPIO_INT_EDGE_BOTH);
    gpio_init_callback(&callbackStruct, chargerStatCallback, BIT(chargerStatDt.pin));
    gpio_add_callback(chargerStatDt.port, &callbackStruct);

    adc_channel_setup_dt(&adc_channel);

    // TODO: Update battery level. See bas_notify()
}
