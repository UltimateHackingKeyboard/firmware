#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <assert.h>

#include <dk_buttons_and_leds.h>

#include "usb.h"
#include "bluetooth.h"

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

void main(void) {
	printk("UHK 80 nordic-left");

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return;
	}


	dk_buttons_init(button_changed);
	dk_leds_init();

	usb_init();
	bluetooth_init();

	uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
	uart_irq_rx_enable(uart_dev);
	int blink_status = 0;
	for (;;) {
		bluetooth_set_adv_led(&blink_status);
		k_sleep(K_MSEC(ADV_LED_BLINK_INTERVAL));
		// Battery level simulation
		bas_notify();
	}
}
