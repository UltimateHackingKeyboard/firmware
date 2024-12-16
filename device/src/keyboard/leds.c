#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/leds.h"
#include "keyboard/spi.h"
#include "leds.h"
#include "shell.h"
#include "keyboard/key_scanner.h"
#include "ledmap.h"
#include "slave_drivers/is31fl3xxx_driver.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;
static k_tid_t ledUpdaterTid = 0;

// LED GPIOs

const struct gpio_dt_spec ledsCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(leds_cs), gpios);
const struct gpio_dt_spec ledsSdbDt = GPIO_DT_SPEC_GET(DT_ALIAS(leds_sdb), gpios);

uint8_t Uhk80LedDriverValues[UHK80_LED_DRIVER_LED_COUNT_MAX];

void setLedsCs(bool state)
{
    gpio_pin_set_dt(&ledsCsDt, state);
}

#define LedPagePrefix 0b01010000

static volatile bool ledsNeedUpdate = false;

static void setOperationMode(bool on) {
    // Set software shutdown control (SSD) register to normal mode
    setLedsCs(true);
    writeSpi(LedPagePrefix | 2);
    writeSpi(0x00);
    writeSpi(0b00001000 | (on ? 1 : 0));
    setLedsCs(false);
}

static void sleepLeds() {
    k_mutex_lock(&SpiMutex, K_FOREVER);
    setOperationMode(0);
    k_mutex_unlock(&SpiMutex);

    while (KeyBacklightBrightness == 0) {
        k_sleep(K_FOREVER);
    }
}

void ledUpdater() {
    k_sleep(K_MSEC(100));
    while (true) {
        k_mutex_lock(&SpiMutex, K_FOREVER);

        setOperationMode(1);

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
            writeSpi(Uhk80LedDriverValues[i]);
        }
        setLedsCs(false);

        setLedsCs(true);
        writeSpi(LedPagePrefix | 1);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(KeyBacklightBrightness);
        }
        setLedsCs(false);

        k_mutex_unlock(&SpiMutex);

        if (!ledsNeedUpdate) {
            k_sleep(K_FOREVER);
        }

        if (KeyBacklightBrightness == 0) {
            sleepLeds();
        }

        ledsNeedUpdate = false;
    }
}

void Uhk80_UpdateLeds() {
    ledsNeedUpdate = true;
    k_wakeup(ledUpdaterTid);
}

void InitLeds(void) {
    gpio_pin_configure_dt(&ledsCsDt, GPIO_OUTPUT);

    gpio_pin_configure_dt(&ledsSdbDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&ledsSdbDt, true);

    ledUpdaterTid = k_thread_create(
        &thread_data, stack_area,
        K_THREAD_STACK_SIZEOF(stack_area),
        ledUpdater,
        NULL, NULL, NULL,
        THREAD_PRIORITY, 0, K_NO_WAIT
    );
    k_thread_name_set(&thread_data, "led_updater");
}
