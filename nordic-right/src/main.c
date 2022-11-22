#include <zephyr/kernel.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

void main(void)
{
	struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
	printk("UHK 80 nordic-right\n");

	while (true) {
		printk("uart1 send: a\n");
		uart_poll_out(uart_dev, 'a');
		k_sleep(K_MSEC(1000));
	}
}
