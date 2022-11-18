#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>


#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <assert.h>
#include <zephyr/spinlock.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/bluetooth/services/bas.h>
#include <bluetooth/services/hids.h>
#include <zephyr/bluetooth/services/dis.h>
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
