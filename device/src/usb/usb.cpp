extern "C"
{
#include "usb.h"
#include "device.h"
#include "keyboard/key_scanner.h"
#include <device.h>
#include <zephyr/kernel.h>
#include "key_states.h"
}
#include "command_app.hpp"
#include "controls_app.hpp"
#include "gamepad_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#include "port/zephyr/udc_mac.hpp"
#include "usb/df/class/hid.hpp"
#include "usb/df/device.hpp"
#include "usb/df/vendor/microsoft_os_extension.hpp"
#include "usb/df/vendor/microsoft_xinput.hpp"
#include <magic_enum.hpp>

#if DEVICE_IS_UHK80_RIGHT
#include "port/zephyr/bluetooth/hid.hpp"

using namespace magic_enum::bitwise_operators;
#endif

// make sure that the USB IDs are used in BT
static_assert(CONFIG_BT_DIS_PNP_VID_SRC == 2);

constexpr usb::product_info product_info{
    CONFIG_BT_DIS_PNP_VID, CONFIG_BT_DIS_MANUF, CONFIG_BT_DIS_PNP_PID, CONFIG_BT_DIS_MODEL,
    usb::version(CONFIG_BT_DIS_PNP_VER >> 8, CONFIG_BT_DIS_PNP_VER)};

struct usb_manager
{
    static usb::df::zephyr::udc_mac& mac() { return instance().mac_; }
    static usb::df::device& device() { return instance().device_; }
    static bool active() { return device().is_open(); }

    void select_config(hid_config_t conf)
    {
        static constexpr auto speed = usb::speed::FULL;
        static usb::df::hid::function usb_kb{keyboard_app::handle(),
                                             usb::hid::boot_protocol_mode::KEYBOARD};
        static usb::df::hid::function usb_mouse{mouse_app::handle()};
        static usb::df::hid::function usb_command{command_app::handle()};
        static usb::df::hid::function usb_controls{controls_app::handle()};
        static usb::df::hid::function usb_gamepad{gamepad_app::handle()};
        static usb::df::microsoft::xfunction usb_xpad{gamepad_app::handle()};

        constexpr auto config_header =
            usb::df::config::header(usb::df::config::power::bus(500, true));
        const auto shared_config_elems = usb::df::config::join_elements(
            usb::df::hid::config(usb_kb, speed, usb::endpoint::address(0x81), 1),
            usb::df::hid::config(usb_mouse, speed, usb::endpoint::address(0x82), 1),
            usb::df::hid::config(usb_command, speed, usb::endpoint::address(0x83), 20),
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

        if (device_.is_open())
        {
            device_.close();
            if (conf == Hid_Empty)
            {
                return;
            }
            k_msleep(100);
        }
        switch (conf)
        {
        case Hid_Empty:
            assert(false); // returned already
            break;
        case Hid_NoGamepad:
            ms_enum_.set_config({});
            device_.set_config(base_config);
            break;
        default:
            ms_enum_.set_config(xpad_config);
            device_.set_config(gamepad_config);
            break;
        }
        device_.open();
    }

    static usb_manager& instance()
    {
        static usb_manager um;
        return um;
    }

  private:
    usb_manager() {}

    usb::df::zephyr::udc_mac mac_{DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0))};
    usb::df::microsoft::alternate_enumeration<usb::speeds(usb::speed::FULL)> ms_enum_{};
    usb::df::device_instance<usb::speeds(usb::speed::FULL)> device_{mac_, product_info, ms_enum_};
};

extern "C" void USB_EnableHid()
{
    usb_manager::instance().select_config(HID_GetGamepadActive() ? Hid_Full : Hid_NoGamepad);
}

extern "C" void USB_DisableHid()
{
    usb_manager::instance().select_config(Hid_Empty);
}

#if DEVICE_IS_UHK80_RIGHT
struct hogp_manager
{
    static hogp_manager& instance()
    {
        static hogp_manager hm;
        return hm;
    }

    static bool active() { return instance().hogp_kb_.active(); }

    void select_config(hid_config_t conf)
    {
        switch (conf)
        {
        case Hid_Empty:
            hogp_kb_.stop();
            hogp_mouse_.stop();
            hogp_controls_.stop();
            hogp_command_.stop();
            hogp_gamepad_.stop();
            break;

        case Hid_NoGamepad:
            hogp_kb_.start();
            hogp_mouse_.start();
            hogp_controls_.start();
            hogp_command_.start();
            hogp_gamepad_.stop();
            break;

        default:
            hogp_kb_.start();
            hogp_mouse_.start();
            hogp_controls_.start();
            hogp_command_.start();
            hogp_gamepad_.start();
            break;
        }
    }

  private:
    hogp_manager() {}

    static const auto security = bluetooth::zephyr::hid::security::ENCRYPT;
    static const auto features = bluetooth::zephyr::hid::flags::NORMALLY_CONNECTABLE |
                                 bluetooth::zephyr::hid::flags::REMOTE_WAKE;

    bluetooth::zephyr::hid::service_instance<hid::report_protocol_properties(
                                                 keyboard_app::report_desc()),
                                             bluetooth::zephyr::hid::boot_protocol_mode::KEYBOARD>
        hogp_kb_{keyboard_app::handle(), security, features};

    bluetooth::zephyr::hid::service_instance<hid::report_protocol_properties(
        mouse_app::report_desc())>
        hogp_mouse_{mouse_app::handle(), security, features};

    bluetooth::zephyr::hid::service_instance<hid::report_protocol_properties(
        controls_app::report_desc())>
        hogp_controls_{controls_app::handle(), security, features};

    bluetooth::zephyr::hid::service_instance<hid::report_protocol_properties(
        command_app::report_desc())>
        hogp_command_{command_app::handle(), security, features};

    bluetooth::zephyr::hid::service_instance<hid::report_protocol_properties(
        gamepad_app::report_desc())>
        hogp_gamepad_{gamepad_app::handle(), security, features};
};

extern "C" void HOGP_Enable()
{
    hogp_manager::instance().select_config(HID_GetGamepadActive() ? Hid_Full : Hid_NoGamepad);
}

extern "C" void HOGP_Disable()
{
    hogp_manager::instance().select_config(Hid_Empty);
}
#endif

static bool gamepadActive = true;

extern "C" bool HID_GetGamepadActive()
{
    return gamepadActive;
}

extern "C" void HID_SetGamepadActive(bool active)
{
    gamepadActive = active;
    if (usb_manager::active())
    {
        USB_EnableHid();
    }
#if DEVICE_IS_UHK80_RIGHT
    if (hogp_manager::active())
    {
        HOGP_Enable();
    }
#endif
}

extern "C" rollover_t HID_GetKeyboardRollover()
{
    static_assert(((uint8_t)Rollover_6Key == (uint8_t)keyboard_app::rollover::SIX_KEY) &&
                  ((uint8_t)Rollover_NKey == (uint8_t)keyboard_app::rollover::N_KEY));
    return (rollover_t)keyboard_app::handle().get_rollover();
}

extern "C" void HID_SetKeyboardRollover(rollover_t mode)
{
    keyboard_app::handle().set_rollover((keyboard_app::rollover)mode);
}

extern "C" void HID_SendReportsThread()
{
#if !DEVICE_IS_UHK_DONGLE
    scancode_buffer keys;
    mouse_buffer mouseState;
    controls_buffer controls;
    gamepad_buffer gamepad;

    while (true)
    {
        keys.set_code(scancode::KEYBOARD_A, KeyPressed);
        keyboard_app::handle().set_report_state(keys);

        mouseState.buttons.set(hid::page::button(2), KeyStates[CURRENT_SLOT_ID][1].current);
        mouseState.x = -50;
        // mouseState.y = -50;
        // mouseState.wheel_y = -50;
        // mouseState.wheel_x = -50;
        mouse_app::handle().set_report_state(mouseState);

        controls.set_code(consumer_code::VOLUME_INCREMENT, KeyStates[CURRENT_SLOT_ID][2].current);
        controls_app::handle().set_report_state(controls);

        gamepad.set_button(gamepad_button::X, KeyStates[CURRENT_SLOT_ID][3].current);
        // gamepad.left.X = 50;
        // gamepad.right.Y = 50;
        // gamepad.right_trigger = 50;
        gamepad_app::handle().set_report_state(gamepad);

        k_msleep(1);
    }
#endif
}
