extern "C" {
#include "device.h"
#include "key_states.h"
#include "logger.h"
#include "power_mode.h"
#include "timer.h"
#include "usb_report_updater.h"
#include "user_logic.h"
#ifdef __ZEPHYR__
    #include "device_state.h"
    #include <nrfx_power.h>
    #include <zephyr/kernel.h>
#else
    #include "buspal/bus_pal_hardware.h"
    #include "trace.h"
#endif
}
#ifdef __ZEPHYR__
    #include "port/zephyr/udc_mac.hpp"
#else
    #include "port/nxp/mcux_mac.hpp"
#endif
#include "command_app.hpp"
#include "controls_app.hpp"
// #include "gamepad_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#include "usb/df/class/hid.hpp"
#include "usb/df/device.hpp"
#include "usb/df/vendor/microsoft_os_extension.hpp"
#include "usb/df/vendor/microsoft_xinput.hpp"
#include <magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

static uint8_t usb_serial_number[4]{};

constexpr usb::product_info product_info{CONFIG_USB_DEVICE_VID, CONFIG_USB_DEVICE_MANUFACTURER,
    CONFIG_USB_DEVICE_PID, CONFIG_USB_DEVICE_PRODUCT,
    usb::version(CONFIG_USB_DEVICE_PRODUCT_VERSION >> 8, CONFIG_USB_DEVICE_PRODUCT_VERSION),
    usb_serial_number};

struct usb_manager {
    static auto &mac() { return instance().mac_; }
    static usb::df::device &device() { return instance().device_; }
    static bool active() { return device().is_open(); }
    static bool ms_host() { return instance().ms_enum_.msos2_support(); }

    static usb_manager &instance()
    {
        static usb_manager um;
        return um;
    }

    void select_config([[maybe_unused]] bool gamepad_active)
    {
        // pretend that the device is disconnected
        if (device().is_open()) {
            device().close();
#ifdef __ZEPHYR__
            k_msleep(100);
#else
            // TODO: use non-blocking delay
            SDK_DelayAtLeastUs(100000, SystemCoreClock);
#endif
        }

        static constexpr auto speed = usb::speed::FULL;
        static usb::df::hid::function usb_kb{
            keyboard_app::usb_handle(), usb::hid::boot_protocol_mode::KEYBOARD};
        static usb::df::hid::function usb_mouse{mouse_app::usb_handle()};
        static usb::df::hid::function usb_command{command_app::usb_handle()};
        static usb::df::hid::function usb_controls{controls_app::usb_handle()};

        constexpr auto config_header =
            usb::df::config::header(usb::df::config::power::bus(500, true));
        const auto shared_config_elems = usb::df::config::join_elements(
            usb::df::hid::config(usb_kb, speed, usb::endpoint::address(0x81), 1),
            usb::df::hid::config(usb_mouse, speed, usb::endpoint::address(0x82), 1),
            usb::df::hid::config(usb_command, speed, usb::endpoint::address(0x83), 10),
            usb::df::hid::config(usb_controls, speed, usb::endpoint::address(0x84), 1));

        static const auto base_config =
            usb::df::config::make_config(config_header, shared_config_elems);
#if 0 // gamepad support disabled
        static const auto gamepad_config =
            usb::df::config::make_config(config_header, shared_config_elems,
                usb::df::hid::config(usb_gamepad, speed, usb::endpoint::address(0x85), 1));

        static const auto xpad_config =
            usb::df::config::make_config(config_header, shared_config_elems,
                usb::df::microsoft::xconfig(
                    usb_xpad, usb::endpoint::address(0x85), 1, usb::endpoint::address(0x05), 255));

        if (conf != Hid_NoGamepad) {
            ms_enum_.set_config(xpad_config);
            device_.set_config(gamepad_config);
        } else
#endif
        {
            ms_enum_.set_config({});
            device_.set_config(base_config);
        }
        device_.open();
    }

    usb_manager()
    {
        device_.set_power_event_delegate([](usb::df::device &dev, usb::df::device::event ev) {
            using event = enum usb::df::device::event;
            if ((ev & event::POWER_STATE_CHANGE) != event::NONE) {
                switch (dev.power_state()) {
                case usb::power::state::L2_SUSPEND:
                    // TODO: use a common API instead of device specific
#if DEVICE_IS_UHK60
                    if (dev.configured()) {
                        PowerMode_ActivateMode(
                            PowerMode_Uhk60Sleep, false, false, "received device suspend event");
                    }
#else
                    PowerMode_SetUsbAwake(false);
#endif
                    break;
                case usb::power::state::L0_ON:
#if DEVICE_IS_UHK60
                    PowerMode_ActivateMode(
                        PowerMode_Awake, false, false, "received device resume event");
#else
                    PowerMode_SetUsbAwake(true);
#endif
                    break;
                default:
                    break;
                }
            }
            if ((ev & event::CONFIGURATION_CHANGE) != event::NONE) {
                // reset the semaphore on USB configuration or reset
                UsbReportUpdateSemaphore = 0;
            }
        });
    }

#ifdef __ZEPHYR__
    usb::zephyr::udc_mac mac_{DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
        (nrfx_power_usbstatus_get() == NRFX_POWER_USB_STATE_CONNECTED)
            ? usb::power::state::L2_SUSPEND
            : usb::power::state::L3_OFF};
#else
    std::array<uint8_t, 128> ctrl_buffer;
    usb::df::nxp::mcux_mac mac_{usb::df::nxp::mcux_mac::khci(ctrl_buffer)};
#endif
    usb::df::microsoft::alternate_enumeration<usb::speeds(usb::speed::FULL)> ms_enum_{};
    usb::df::device_instance<usb::speeds(usb::speed::FULL)> device_{mac_, product_info, ms_enum_};
};

#ifndef __ZEPHYR__
extern "C" void USB0_IRQHandler(void)
{
    Trace_Printc("<i6");
    if (usb::df::nxp::mcux_mac::notification_routing) {
        usb_manager::mac().handle_irq();
    } else {
        USB_DeviceKhciIsrFunction(BuspalCompositeUsbDevice.device_handle);
    }
    Trace_Printc(">");
    SDK_ISR_EXIT_BARRIER;
}
#endif

extern "C" void USB_Enable()
{
    assert(!usb_manager::active());
    usb_manager::instance().select_config(HID_GetGamepadActive());
}

extern "C" void USB_Reconfigure()
{
    assert(usb_manager::active());
    usb_manager::instance().select_config(HID_GetGamepadActive());
}

extern "C" bool USB_RemoteWakeup()
{
    auto err = usb_manager::instance().device().remote_wakeup();
#ifdef __ZEPHYR__
    if (err != usb::result::ok) {
        LogUO("USB: remote wakeup request failed: %d\n", std::bit_cast<int>(err));
    }
#endif
    return err == usb::result::ok;
}

extern "C" bool USB_IsMsHost(void)
{
    return usb_manager::ms_host();
}

extern "C" void USB_SetSerialNumber(uint32_t serialNumber)
{
    static_assert(sizeof(usb_serial_number) >= 4, "usb_serial_number size is too small");

    // Convert each pair of decimal digits into a single byte
    for (uint8_t i = 4; i < 255; --i) {
        uint8_t byte = 0;
        for (uint8_t j = 0; j < 2; ++j) {
            uint8_t digit = serialNumber % 10;
            serialNumber /= 10;

            byte |= digit << (j * 4);
        }

        usb_serial_number[i] = byte;
    }
}
