#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/leds.h"
#include "keyboard/spi.h"
#include "shell.h"
#include "keyboard/key_scanner.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

// LED GPIOs

const struct gpio_dt_spec ledsCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(leds_cs), gpios);
const struct gpio_dt_spec ledsSdbDt = GPIO_DT_SPEC_GET(DT_ALIAS(leds_sdb), gpios);

void setLedsCs(bool state)
{
    gpio_pin_set_dt(&ledsCsDt, state);
}

#define LedPagePrefix 0b01010000

void ledUpdater() {
    while (true) {
        k_mutex_lock(&SpiMutex, K_FOREVER);

        // Set software shutdown control (SSD) register to normal mode
        setLedsCs(true);
        writeSpi(LedPagePrefix | 2);
        writeSpi(0x00);
        writeSpi(0b00001001);
        setLedsCs(false);

        // Set 180 degree phase delay to reduce audible noise, although it doesn't seem to make a difference
        setLedsCs(true);
        writeSpi(LedPagePrefix | 2);
        writeSpi(0x02);
        writeSpi(0b10110011);
        setLedsCs(false);

        // Enable spread spectrum with 15% range and 1980us cycle time, which substantially reduces audible noise
        setLedsCs(true);
        writeSpi(LedPagePrefix | 2);
        writeSpi(0x25);
        writeSpi(0x14);
        setLedsCs(false);

        setLedsCs(true);
        writeSpi(LedPagePrefix | 2);
        writeSpi(0x01);
        writeSpi(0xff);
        setLedsCs(false);

        setLedsCs(true);
        writeSpi(LedPagePrefix | 0);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(KeyPressed || Shell.ledsAlwaysOn ? 0xff : 0);
        }
        setLedsCs(false);

        setLedsCs(true);
        writeSpi(LedPagePrefix | 1);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(KeyPressed || Shell.ledsAlwaysOn ? 0xff : 0);
        }
        setLedsCs(false);

        k_mutex_unlock(&SpiMutex);

        k_sleep(K_MSEC(100));
    }
}

void InitLeds(void) {
    gpio_pin_configure_dt(&ledsCsDt, GPIO_OUTPUT);

    gpio_pin_configure_dt(&ledsSdbDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&ledsSdbDt, true);

    k_thread_create(
        &thread_data, stack_area,
        K_THREAD_STACK_SIZEOF(stack_area),
        ledUpdater,
        NULL, NULL, NULL,
        THREAD_PRIORITY, 0, K_NO_WAIT
    );
    k_thread_name_set(&thread_data, "led_updater");
}
