#include "power.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include "legacy/debug.h"

bool Power_RunningOnBattery() {
    return CurrentWatch == 6;
}

void cb(uint8_t cb_status, const uint8_t *param) {
    printk("PM2: %i\n", cb_status);
}

void InitPower(void) {
    // TODO: enable this in next zephyr version, or find a different way
    //struct usbd_context *uds_ctx;
    // uds_ctx = usbd_get_context();
    // usbd_msg_register_cb(uds_ctx, usb_event_callback);

}
