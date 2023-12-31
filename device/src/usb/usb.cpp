extern "C"
{
#include "keyboard/key_scanner.h"
#include <device.h>
#include <zephyr/kernel.h>
}
#include "command_app.hpp"
#include "controls_app.hpp"
#include "device.h"
#include "gamepad_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#include "port/zephyr/udc_mac.hpp"
#include "usb.hpp"
#include "usb/df/class/hid.hpp"
#include "usb/df/device.hpp"
#include "usb/df/vendor/microsoft_os_extension.hpp"
#include "usb/df/vendor/microsoft_xinput.hpp"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

// make sure that the USB IDs are used in BT
static_assert(CONFIG_BT_DIS_PNP_VID_SRC == 2);

constexpr usb::product_info prinfo{CONFIG_BT_DIS_PNP_VID, CONFIG_BT_DIS_MANUF,
                                   CONFIG_BT_DIS_PNP_PID, CONFIG_BT_DIS_MODEL,
                                   usb::version(CONFIG_BT_DIS_PNP_VER >> 8, CONFIG_BT_DIS_PNP_VER)};
scancode_buffer keys;
mouse_buffer mouseState;
controls_buffer controls;
gamepad_buffer gamepad;

void sendUsbReports(void*, void*, void*)
{
    while (true)
    {
#if CONFIG_DEVICE_ID != DEVICE_ID_UHK_DONGLE
        keys.set_code(scancode::A, KeyPressed);
#endif
        keyboard_app::handle().set_report_state(keys);

#if CONFIG_DEVICE_ID != DEVICE_ID_UHK_DONGLE
        mouseState.set_button(mouse_button::RIGHT, KeyStates[0][1]);
#endif
        mouseState.x = -50;
        // mouseState.y = -50;
        // mouseState.wheel_y = -50;
        // mouseState.wheel_x = -50;
        mouse_app::handle().set_report_state(mouseState);

#if CONFIG_DEVICE_ID != DEVICE_ID_UHK_DONGLE
        controls.set_code(consumer_code::VOLUME_INCREMENT, KeyStates[0][2]);
#endif
        controls_app::handle().set_report_state(controls);

#if CONFIG_DEVICE_ID != DEVICE_ID_UHK_DONGLE
        gamepad.set_button(gamepad_button::X, KeyStates[0][3]);
#endif
        // gamepad.left.X = 50;
        // gamepad.right.Y = 50;
        // gamepad.right_trigger = 50;
        gamepad_app::handle().set_report_state(gamepad);

        k_msleep(1);
    }
}

extern "C"
{

void usb_init(bool gamepad_enable)
{
    static constexpr auto speed = usb::speed::FULL;
    static usb::df::zephyr::udc_mac mac{DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0))};

    static usb::df::hid::function usb_kb{keyboard_app::handle(),
                                         usb::hid::boot_protocol_mode::KEYBOARD};
    static usb::df::hid::function usb_mouse{mouse_app::handle()};
    static usb::df::hid::function usb_command{command_app::handle()};
    static usb::df::hid::function usb_controls{controls_app::handle()};
    static usb::df::hid::function usb_gamepad{gamepad_app::handle()};
    static usb::df::microsoft::xfunction usb_xpad{gamepad_app::handle()};

    constexpr auto config_header = usb::df::config::header(usb::df::config::power::bus(500, true));
    const auto shared_config_elems = usb::df::config::join_elements(
        usb::df::hid::config(usb_kb, speed, usb::endpoint::address(0x81), 1),
        usb::df::hid::config(usb_mouse, speed, usb::endpoint::address(0x82), 1),
        usb::df::hid::config(usb_command, speed, usb::endpoint::address(0x83), 20,
                             usb::endpoint::address(0x03), 20),
        usb::df::hid::config(usb_controls, speed, usb::endpoint::address(0x84), 1));

    static const auto base_config =
        usb::df::config::make_config(config_header, shared_config_elems);

    static const auto gamepad_config = usb::df::config::make_config(
        config_header, shared_config_elems,
        usb::df::hid::config(usb_gamepad, speed, usb::endpoint::address(0x85), 1));

    static const auto xpad_config = usb::df::config::make_config(
        config_header, shared_config_elems,
        usb::df::microsoft::xconfig(usb_xpad, usb::endpoint::address(0x85), 1,
                                    usb::endpoint::address(0x05), 255));

    static usb::df::microsoft::alternate_enumeration<usb::speeds(usb::speed::FULL)> ms_enum{};
    static usb::df::device_instance<usb::speeds(usb::speed::FULL)> device{mac, prinfo, ms_enum};

    if (device.is_open())
    {
        device.close();
        // TODO: add enough sleep to be effective
        k_sleep(K_MSEC(100));
    }
    if (gamepad_enable)
    {
        ms_enum.set_config(xpad_config);
        device.set_config(gamepad_config);
    }
    else
    {
        ms_enum.set_config({});
        device.set_config(base_config);
    }
    device.open();

    k_thread_create(&thread_data, stack_area, K_THREAD_STACK_SIZEOF(stack_area), sendUsbReports,
                    NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&thread_data, "send_usb_reports");
}

uint8_t USB_GetKeyboardRollover(void)
{
    return (uint8_t)keyboard_app::handle().get_rollover();
}

void USB_SetKeyboardRollover(uint8_t mode)
{
    keyboard_app::handle().set_rollover((keyboard_app::rollover)mode);
}

} // extern "C"