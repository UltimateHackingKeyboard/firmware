extern "C"
{
#include <zephyr/kernel.h>
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
#include "device.h"
#include "usb.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#include "command_app.hpp"
#include "controls_app.hpp"
#include "gamepad_app.hpp"
#include "usb/df/device.hpp"
#include "usb/df/port/zephyr/udc_mac.hpp"
#include "usb/df/class/hid.hpp"
#include "usb/df/vendor/microsoft_os_extension.hpp"
#include "usb/df/vendor/microsoft_xinput.hpp"

constexpr usb::product_info prinfo {
    0x1D50, "Ultimage Gadget Laboratories",
    USB_DEVICE_PRODUCT_ID, DEVICE_NAME, usb::version("1.0")
};

void usb_init(bool gamepad_enable) {
    static constexpr auto speed = usb::speed::FULL;
    static usb::df::zephyr::udc_mac mac {DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0))};

    static usb::df::hid::function usb_kb { keyboard_app::handle(), usb::hid::boot_protocol_mode::KEYBOARD };
    static usb::df::hid::function usb_mouse { mouse_app::handle() };
    static usb::df::hid::function usb_command { command_app::handle() };
    static usb::df::hid::function usb_controls { controls_app::handle() };
    static usb::df::hid::function usb_gamepad { gamepad_app::handle() };
    static usb::df::microsoft::xfunction usb_xpad { gamepad_app::handle() };

    constexpr auto config_header = usb::df::config::header(usb::df::config::power::bus(500, true));
    const auto shared_config_elems = usb::df::config::join_elements(
            usb::df::hid::config(usb_kb, speed, usb::endpoint::address(0x81), 1),
            usb::df::hid::config(usb_mouse, speed, usb::endpoint::address(0x82), 1),
            usb::df::hid::config(usb_command, speed, usb::endpoint::address(0x83), 20,
                    usb::endpoint::address(0x03), 20),
            usb::df::hid::config(usb_controls, speed, usb::endpoint::address(0x84), 1)
    );

    static const auto base_config = usb::df::config::make_config(config_header, shared_config_elems);

    static const auto gamepad_config = usb::df::config::make_config(config_header, shared_config_elems,
            usb::df::hid::config(usb_gamepad, speed, usb::endpoint::address(0x85), 1)
    );

    static const auto xpad_config = usb::df::config::make_config(config_header, shared_config_elems,
            usb::df::microsoft::xconfig(usb_xpad, usb::endpoint::address(0x85), 1,
                    usb::endpoint::address(0x05), 255)
    );

    static usb::df::microsoft::alternate_enumeration<usb::speeds(usb::speed::FULL)> ms_enum {};
    static usb::df::device_instance<usb::speeds(usb::speed::FULL)> device {mac, prinfo, ms_enum};

    if (device.is_open()) {
        device.close();
        // TODO: add enough sleep to be effective
        k_sleep(K_MSEC(100));
    }
    if (gamepad_enable) {
        ms_enum.set_config(xpad_config);
        device.set_config(gamepad_config);
    } else {
        ms_enum.set_config({});
        device.set_config(base_config);
    }
    device.open();
}
