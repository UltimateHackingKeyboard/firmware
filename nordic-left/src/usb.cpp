extern "C"
{
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
#include <zephyr/spinlock.h>

#include <zephyr/settings/settings.h>

#include "bluetooth.h"
}
#include "usb.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#include "command_app.hpp"
#include "usb/df/device.hpp"
#include "usb/df/port/zephyr/udc_mac.hpp"
#include "usb/df/class/hid.hpp"

// TODO fill valid product info
constexpr usb::product_info prinfo {
    0xfff0, "UGL",
    0xffff, "UHK", usb::version("1.0")
};

void usb_init(const device* dev) {
    static constexpr auto speed = usb::speed::FULL;
    static usb::df::zephyr::udc_mac mac {dev};

    static usb::df::hid::function usb_kb { keyboard_app::handle(), usb::hid::boot_protocol_mode::KEYBOARD };
    static usb::df::hid::function usb_mouse { mouse_app::handle() };
    static usb::df::hid::function usb_command { command_app::handle() };

    static const auto single_config = usb::df::config::make_config(usb::df::config::header(usb::df::config::power::bus(500, true)),
            usb::df::hid::config(usb_kb, speed, usb::endpoint::address(0x81), 1),
            usb::df::hid::config(usb_mouse, speed, usb::endpoint::address(0x82), 1),
            usb::df::hid::config(usb_command, speed, usb::endpoint::address(0x83), 20,
            usb::endpoint::address(0x03), 20)
    );

    static usb::df::device_instance<usb::speeds(usb::speed::FULL)> device {mac, prinfo};
    device.set_config(single_config);
    device.open();
}
