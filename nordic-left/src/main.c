#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

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

void main(void) {
	dk_buttons_init(button_changed);
	dk_leds_init();

	usb_init();
	bluetooth_init();

	int blink_status = 0;
	for (;;) {
		bluetooth_set_adv_led(&blink_status);
		k_sleep(K_MSEC(ADV_LED_BLINK_INTERVAL));
		// Battery level simulation
		bas_notify();
	}
}
