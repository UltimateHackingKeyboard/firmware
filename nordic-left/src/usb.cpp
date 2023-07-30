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
#include "usb/df/device.hpp"
#include "usb/df/port/zephyr/udc_mac.hpp"


keyboard_app::keyboard_app(hid::page::keyboard_keypad key, hid::page::leds led)
        : hid::application(report_prot()),
            key_(static_cast<uint8_t>(key)),
            led_mask_(1 << (static_cast<uint8_t>(led) - 1))
{
}

void keyboard_app::start(hid::protocol prot)
{
    prot_ = prot;

    // start receiving reports
    receive_report(&leds_buffer_);
}

void keyboard_app::stop()
{
}

void keyboard_app::set_report(hid::report::type type, const std::span<const uint8_t>& data)
{
    // only one report is receivable
    auto *out_report = reinterpret_cast<const kb_leds_report*>(data.data());

    // DEMO if the selected LED is on, send the key to turn it off
    if ((out_report->leds & led_mask_) != 0) {
        keys_buffer_.scancodes[0] = key_;
        send_report(&keys_buffer_);
    }

    // always keep receiving new reports
    // if the report data is processed immediately, the same buffer can be used
    receive_report(&leds_buffer_);
}

void keyboard_app::in_report_sent(const std::span<const uint8_t>& data)
{
    // DEMO release the key once it has been pressed
    if (keys_buffer_.scancodes[0] == key_) {
        keys_buffer_.scancodes[0] = 0;
        send_report(&keys_buffer_);
    }
}

void keyboard_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // fetch the report data
    send_report(&keys_buffer_);
}

keyboard_app kb {hid::page::keyboard_keypad::CAPSLOCK,
    hid::page::leds::CAPS_LOCK };

void usb_init(const device* dev) {
    static usb::df::zephyr::udc_mac mac {dev};

    static usb::df::hid::function usb_kb { kb, usb::hid::boot_protocol_mode::KEYBOARD };

    static const auto single_config = usb::df::config::make_config(usb::df::config::header(usb::df::config::power::bus()),
            usb::df::hid::config(usb_kb, usb::speed::FULL, usb::endpoint::address(0x81), 5)
    );

    // TODO fill valid product info
    static constexpr usb::product_info prinfo { 
        0xfff0, "UGL", 
        0xffff, "UHK", usb::version("1.0")
    };
    static usb::df::device_instance<usb::speeds(usb::speed::FULL)> device {mac, prinfo
        , std::span<uint8_t>()
    };
    device.set_config(single_config);
    device.open();
}
