#include <zephyr/shell/shell.h>
extern "C"
{
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>
#include <assert.h>

#include <dk_buttons_and_leds.h>

#include "bluetooth.h"
}
#include "usb.hpp"
#include <zephyr/drivers/adc.h>

#define DEVICE_ID_UHK80_LEFT 3
#define DEVICE_ID_UHK80_RIGHT 4

#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_LEFT
    #define DEVICE_NAME "UHK 80 left half"
#elif CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT
    #define DEVICE_NAME "UHK 80 right half"
#endif

#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_LEFT
    #define HAS_MERGE_SENSE
#endif

#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT
    #define HAS_OLED
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};

#define LedPagePrefix 0b01010000

static struct spi_config spiConf = {
    .frequency = 400000U,
    .operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
};

uint8_t buf[] = {1};
const struct spi_buf spiBuf[] = {
    {
        .buf = &buf,
        .len = 1,
    }
};

const struct spi_buf_set spiBufSet = {
    .buffers = spiBuf,
    .count = 1,
};

const struct device *spi0_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
static const struct gpio_dt_spec ledsCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(leds_cs), gpios);
static const struct gpio_dt_spec ledsSdbDt = GPIO_DT_SPEC_GET(DT_ALIAS(leds_sdb), gpios);
static const struct gpio_dt_spec chargerEnDt = GPIO_DT_SPEC_GET(DT_ALIAS(charger_en), gpios);
static const struct gpio_dt_spec chargerStatDt = GPIO_DT_SPEC_GET(DT_ALIAS(charger_stat), gpios);

#ifdef HAS_OLED
static const struct gpio_dt_spec oledEn = GPIO_DT_SPEC_GET(DT_ALIAS(oled_en), gpios);
static const struct gpio_dt_spec oledResetDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_reset), gpios);
static const struct gpio_dt_spec oledCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_cs), gpios);
static const struct gpio_dt_spec oledA0Dt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_a0), gpios);

void setOledCs(bool state)
{
    gpio_pin_set_dt(&oledCsDt, state);
}

void setA0(bool state)
{
    gpio_pin_set_dt(&oledA0Dt, state);
}

#endif

void setLedsCs(bool state)
{
    gpio_pin_set_dt(&ledsCsDt, state);
}

void writeSpi(uint8_t data)
{
    buf[0] = data;
    spi_write(spi0_dev, &spiConf, &spiBufSet);
}

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

void serial_cb(const struct device *dev, void *user_data)
{
    if (!uart_irq_update(uart_dev)) {
        return;
    }

    uint8_t c;
    while (uart_irq_rx_ready(uart_dev)) {
        uart_fifo_read(uart_dev, &c, 1);
        printk("uart1 receive: %c\n", c);
    }
}

static struct gpio_dt_spec rows[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(row1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row3), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row4), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row5), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row6), gpios),
};

static struct gpio_dt_spec cols[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(col1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col3), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col4), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col5), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col6), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col7), gpios),
#if DEVICE_ID == DEVICE_ID_UHK80_RIGHT
    GPIO_DT_SPEC_GET(DT_ALIAS(col8), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col9), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col10), gpios),
#endif
};

#define COLS_COUNT (sizeof(cols) / sizeof(cols[0]))

// Shell functions

uint8_t keyLog = 1;
static int cmd_uhk_keylog(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", keyLog);
    } else {
        keyLog = argv[1][0] == '1';
    }
    return 0;
}

uint8_t statLog = 1;
static int cmd_uhk_statlog(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", statLog);
    } else {
        statLog = argv[1][0] == '1';
    }
    return 0;
}

uint8_t ledsAlwaysOn = 0;
static int cmd_uhk_leds(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", ledsAlwaysOn);
    } else {
        ledsAlwaysOn = argv[1][0] == '1';
    }
    return 0;
}

uint8_t sdbState = 1;
static int cmd_uhk_sdb(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", sdbState ? 1 : 0);
    } else {
        sdbState = argv[1][0] == '1';
        gpio_pin_set_dt(&ledsSdbDt, sdbState);
    }
    return 0;
}

// Charger behavior:
// - If CHARGER_EN=0 or USB is disconnected, then TS reads 6552[0-9] raw value and sometimes drops to 0.
// - If CHARGER_EN=1, USB is connected, and the battery is disconnected, then STAT alternates between 0 and 1 per second

uint8_t chargerState = 1;
static int cmd_uhk_charger(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "CHARGER_EN: %i | ", chargerState ? 1 : 0);
        shell_fprintf(shell, SHELL_NORMAL, "STAT: %i", gpio_pin_get_dt(&chargerStatDt) ? 1 : 0);

        int err;
        uint16_t buf;
        struct adc_sequence sequence = {
            .buffer = &buf,
            .buffer_size = sizeof(buf),
        };

        for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
            int32_t val_mv;

            printk(" | ");
            printk(i ? "VBAT" : "TS");

            (void)adc_sequence_init_dt(&adc_channels[i], &sequence);

            err = adc_read(adc_channels[i].dev, &sequence);
            if (err < 0) {
                printk("Could not read (%d)\n", err);
                continue;
            }

            // If using differential mode, the 16 bit value
            // in the ADC sample buffer should be a signed 2's
            // complement value.
            if (adc_channels[i].channel_cfg.differential) {
                val_mv = (int32_t)((int16_t)buf);
            } else {
                val_mv = (int32_t)buf;
            }
            printk(": %d", val_mv);
            err = adc_raw_to_millivolts_dt(&adc_channels[i], &val_mv);
            // conversion to mV may not be supported, skip if not
            if (err < 0) {
                printk(" (value in mV not available)");
            } else {
                printk(" = %d mV", val_mv);
            }
        }
        printk("\n");
    } else {
        chargerState = argv[1][0] == '1';
        gpio_pin_set_dt(&chargerEnDt, chargerState);
    }
    return 0;
}

#ifdef HAS_OLED
uint8_t oledState = 1;
static int cmd_uhk_oled(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", oledState ? 1 : 0);
    } else {
        oledState = argv[1][0] == '1';
        gpio_pin_set_dt(&oledEn, oledState);
    }
    return 0;
}
#endif

#ifdef HAS_MERGE_SENSE
static const struct gpio_dt_spec mergeSenseDt = GPIO_DT_SPEC_GET(DT_ALIAS(merge_sense), gpios);

static int cmd_uhk_merge(const struct shell *shell, size_t argc, char *argv[])
{
    shell_fprintf(shell, SHELL_NORMAL, "%i\n", gpio_pin_get_dt(&mergeSenseDt) ? 1 : 0);
    return 0;
}
#endif

void chargerStatCallback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
    if (statLog) {
        printk("STAT changed to %i\n", gpio_pin_get_dt(&chargerStatDt) ? 1 : 0);
    }
}

struct gpio_callback callbackStruct;

volatile char keyPressed;

int main(void) {
    printk("----------\n" DEVICE_NAME " started\n");

    // Configure GPIOs

    gpio_pin_configure_dt(&ledsCsDt, GPIO_OUTPUT);

    gpio_pin_configure_dt(&ledsSdbDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&ledsSdbDt, true);

    gpio_pin_configure_dt(&chargerEnDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&chargerEnDt, true);

    gpio_pin_configure_dt(&chargerStatDt, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&chargerStatDt, GPIO_INT_EDGE_BOTH);
    gpio_init_callback(&callbackStruct, chargerStatCallback, BIT(chargerStatDt.pin));
    gpio_add_callback(chargerStatDt.port, &callbackStruct);

#ifdef HAS_OLED
    gpio_pin_configure_dt(&oledEn, GPIO_OUTPUT);
    gpio_pin_set_dt(&oledEn, true);

    gpio_pin_configure_dt(&oledResetDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&oledResetDt, false);
    k_msleep(1);
    gpio_pin_set_dt(&oledResetDt, true);

    gpio_pin_configure_dt(&oledCsDt, GPIO_OUTPUT);
    gpio_pin_configure_dt(&oledA0Dt, GPIO_OUTPUT);
#endif

    struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

    for (uint8_t rowId=0; rowId<6; rowId++) {
        gpio_pin_configure_dt(&rows[rowId], GPIO_OUTPUT);
    }
    for (uint8_t colId=0; colId<COLS_COUNT; colId++) {
        gpio_pin_configure_dt(&cols[colId], GPIO_INPUT);
    }

#ifdef HAS_MERGE_SENSE
    gpio_pin_configure_dt(&mergeSenseDt, GPIO_INPUT);
#endif

    // Create shell commands

    SHELL_STATIC_SUBCMD_SET_CREATE(
        uhk_cmds,
        SHELL_CMD_ARG(keylog, NULL,
            "get/set key logging",
            cmd_uhk_keylog, 1, 1),
        SHELL_CMD_ARG(statlog, NULL,
            "get/set stat logging",
            cmd_uhk_statlog, 1, 1),
        SHELL_CMD_ARG(leds, NULL,
            "get/set LEDs always on state",
            cmd_uhk_leds, 1, 1),
        SHELL_CMD_ARG(sdb, NULL,
            "get/set LED driver SDB pin",
            cmd_uhk_sdb, 1, 1),
        SHELL_CMD_ARG(charger, NULL,
            "get/set CHARGER_EN pin",
            cmd_uhk_charger, 1, 1),
#ifdef HAS_OLED
        SHELL_CMD_ARG(oled, NULL,
            "get/set OLED_EN pin",
            cmd_uhk_oled, 1, 1),
#endif
#ifdef HAS_MERGE_SENSE
        SHELL_CMD_ARG(merge, NULL,
            "get the merged state of UHK halves",
            cmd_uhk_merge, 1, 0),
#endif
        SHELL_SUBCMD_SET_END
    );

    SHELL_CMD_REGISTER(uhk, &uhk_cmds, "UHK commands", NULL);

    // Init ADC channels
    for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
        if (!device_is_ready(adc_channels[i].dev)) {
            printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
            return 0;
        }
        int err;
        err = adc_channel_setup_dt(&adc_channels[i]);
        if (err < 0) {
            printk("Could not setup channel #%d (%d)\n", i, err);
            return 0;
        }
    }

    if (!device_is_ready(uart_dev)) {
        printk("UART device not found!");
        return;
    }

    // dk_buttons_init(button_changed);
    // dk_leds_init();

    usb_init(true);
    bluetooth_init();

    uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
    uart_irq_rx_enable(uart_dev);
//  int blink_status = 0;
    uint32_t counter = 0;
    bool pixel = 1;

    for (;;) {
        keyPressed = false;
        for (uint8_t rowId=0; rowId<6; rowId++) {
            gpio_pin_set_dt(&rows[rowId], 1);
            for (uint8_t colId=0; colId<COLS_COUNT; colId++) {
                if (gpio_pin_get_dt(&cols[colId])) {
                    keyPressed = true;
                    if (keyLog) {
                        printk("SW%c%c\n", rowId+'1', colId+'1');
                        uart_poll_out(uart_dev, 'a');
                    }
                }
            }
            gpio_pin_set_dt(&rows[rowId], 0);
        }

        #ifdef HAS_OLED
        setA0(false);
        setOledCs(false);
        writeSpi(0xaf);
        setOledCs(true);

        setA0(false);
        setOledCs(false);
        writeSpi(0x81);
        writeSpi(0xff);
        setOledCs(true);

        setA0(true);
        setOledCs(false);
        writeSpi(pixel ? 0xff : 0x00);
        setOledCs(true);
        #endif

        setLedsCs(false);
        writeSpi(LedPagePrefix | 2);
        writeSpi(0x00);
        writeSpi(0b00001001);
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 2);
        writeSpi(0x01);
        writeSpi(0xff);
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 0);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(keyPressed || ledsAlwaysOn ? 0xff : 0);
        }
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 1);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(keyPressed || ledsAlwaysOn ? 0xff : 0);
        }
        setLedsCs(true);

//      bluetooth_set_adv_led(&blink_status);
//      k_sleep(K_MSEC(ADV_LED_BLINK_INTERVAL));
        // Battery level simulation
        bas_notify();

        if (counter++ > 19) {
            pixel = !pixel;
            counter = 0;
        }
    }
}
