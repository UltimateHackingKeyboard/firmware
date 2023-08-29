#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
// #include <zephyr/drivers/uart.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>
#include <assert.h>

#include <dk_buttons_and_leds.h>

#include "usb.h"
#include "bluetooth.h"

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

void setLedsCs(bool state)
{
    gpio_pin_set_dt(&ledsCsDt, state);
}

void writeSpi(uint8_t data)
{
    buf[0] = data;
    spi_write(spi0_dev, &spiConf, &spiBufSet);
}

// static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

// void serial_cb(const struct device *dev, void *user_data)
// {
//     if (!uart_irq_update(uart_dev)) {
//         return;
//     }

//     uint8_t c;
//     while (uart_irq_rx_ready(uart_dev)) {
//         uart_fifo_read(uart_dev, &c, 1);
//         printk("uart1 receive: %c\n", c);
//     }
// }

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
};

void main(void) {
printk("----------\nUHK 80 left half started\n");
    gpio_pin_configure_dt(&ledsCsDt, GPIO_OUTPUT);

    gpio_pin_configure_dt(&ledsSdbDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&ledsSdbDt, true);

    for (uint8_t rowId=0; rowId<6; rowId++) {
        gpio_pin_configure_dt(&rows[rowId], GPIO_OUTPUT);
    }
    for (uint8_t colId=0; colId<7; colId++) {
        gpio_pin_configure_dt(&cols[colId], GPIO_INPUT);
    }

    // if (!device_is_ready(uart_dev)) {
    //     printk("UART device not found!");
    //     return;
    // }

    // dk_buttons_init(button_changed);
    // dk_leds_init();

    // usb_init();
    // bluetooth_init();

    // uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
    // uart_irq_rx_enable(uart_dev);
//  int blink_status = 0;
    for (;;) {
        c = 0;
        for (uint8_t rowId=0; rowId<6; rowId++) {
            gpio_pin_set_dt(&rows[rowId], 1);
            for (uint8_t colId=0; colId<7; colId++) {
                if (gpio_pin_get_dt(&cols[colId])) {
                    c = HID_KEY_A;
                    printk("SW%c%c\n", rowId+'1', colId+'1');
                }
            }
            gpio_pin_set_dt(&rows[rowId], 0);
        }

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
            writeSpi(c?0xff:0);
        }
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 1);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(c?0xff:0);
        }
        setLedsCs(true);

//      bluetooth_set_adv_led(&blink_status);
//      k_sleep(K_MSEC(ADV_LED_BLINK_INTERVAL));
        // Battery level simulation
        bas_notify();
    }
}
