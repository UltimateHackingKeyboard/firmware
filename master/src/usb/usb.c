#include <zephyr/kernel.h>
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
#include <soc.h>
#include <assert.h>
#include <zephyr/spinlock.h>

#include <zephyr/settings/settings.h>

#include "bluetooth.h"

char volatile c = 0;
static const uint8_t hid_keyboard_report_desc[] = HID_KEYBOARD_REPORT_DESC();
static const uint8_t hid_mouse_report_desc[] = HID_MOUSE_REPORT_DESC(2);

static enum usb_dc_status_code usb_status;

#define MOUSE_BTN_REPORT_POS    0
#define MOUSE_X_REPORT_POS  1
#define MOUSE_Y_REPORT_POS  2

#define MOUSE_BTN_LEFT      BIT(0)
#define MOUSE_BTN_RIGHT     BIT(1)
#define MOUSE_BTN_MIDDLE    BIT(2)

uint8_t mouse_report[4] = { 0x00 };
uint8_t keyboard_report[8] = { 0x00 };
const struct device *hid_keyboard_dev;
const struct device *hid_mouse_dev;

static void status_cb(enum usb_dc_status_code status, const uint8_t *param) {
    printk("USB status code change: %i\n", status);
    usb_status = status;
    if (status == USB_DC_CONFIGURED) {
        hid_int_ep_write(hid_mouse_dev, mouse_report, sizeof(mouse_report), NULL);
        hid_int_ep_write(hid_keyboard_dev, keyboard_report, sizeof(keyboard_report), NULL);
    }
}

/*
    if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
        if (usb_status == USB_DC_SUSPEND) {
            usb_wakeup_request();
            return;
        }
    }
*/

void int_in_ready_mouse(const struct device *dev) {
    // mouse_report[MOUSE_X_REPORT_POS] = buttons & DK_BTN1_MSK ? 5 : 0;
    // mouse_report[MOUSE_Y_REPORT_POS] = buttons & DK_BTN2_MSK ? 5 : 0;
    // mouse_report[MOUSE_BTN_REPORT_POS] = buttons & DK_BTN3_MSK ? MOUSE_BTN_LEFT : 0;
    hid_int_ep_write(hid_mouse_dev, mouse_report, sizeof(mouse_report), NULL);
}

// bool c=false;
void int_in_ready_keyboard(const struct device *dev) {
    // c=!c;
    keyboard_report[2] = c ? HID_KEY_A : 0;
    hid_int_ep_write(hid_keyboard_dev, keyboard_report, sizeof(keyboard_report), NULL);
}

struct hid_ops hidops_mouse = {
    .int_in_ready = int_in_ready_mouse,
};

struct hid_ops hidops_keyboard = {
    .int_in_ready = int_in_ready_keyboard,
};

void usb_init(void) {
    hid_keyboard_dev = device_get_binding("HID_0");
    hid_mouse_dev = device_get_binding("HID_1");

    usb_hid_register_device(hid_keyboard_dev, hid_keyboard_report_desc, sizeof(hid_keyboard_report_desc), &hidops_keyboard);
    usb_hid_register_device(hid_mouse_dev, hid_mouse_report_desc, sizeof(hid_mouse_report_desc), &hidops_mouse);

    usb_hid_init(hid_keyboard_dev);
    usb_hid_init(hid_mouse_dev);

    usb_enable(status_cb);
}
