extern "C" {
#include "usb.h"
#include "device.h"
#include "key_states.h"
#include "keyboard/charger.h"
#include "keyboard/key_scanner.h"
#include "power_mode.h"
#include "timer.h"
#include "user_logic.h"
#include "logger.h"
#include <device.h>
#include <zephyr/kernel.h>
#include "logger.h"
#include "power_mode.h"
#include "connections.h"
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

extern "C" {
#include "device_state.h"
#include "usb_report_updater.h"
}
#if DEVICE_HAS_BATTERY
    #include "hid_battery_app.hpp"
#endif
#if DEVICE_IS_UHK80_RIGHT
    #include "port/zephyr/bluetooth/hid.hpp"

using namespace magic_enum::bitwise_operators;
#endif

uint8_t UsbSerialNumber[5];

constexpr usb::product_info product_info{CONFIG_USB_DEVICE_VID, CONFIG_USB_DEVICE_MANUFACTURER,
    CONFIG_USB_DEVICE_PID, CONFIG_USB_DEVICE_PRODUCT,
    usb::version(CONFIG_USB_DEVICE_PRODUCT_VERSION >> 8, CONFIG_USB_DEVICE_PRODUCT_VERSION),
    UsbSerialNumber};

template <typename... Args>
class multi_hid : public hid::multi_application {
  public:
    static constexpr auto report_desc() { return hid::rdf::descriptor((Args::report_desc(), ...)); }

    static multi_hid &handle()
    {
        static multi_hid s;
        return s;
    }

  private:
    std::array<hid::application *, sizeof...(Args) + 1> app_array_{(&Args::handle())..., nullptr};
    multi_hid() : multi_application({{}, 0, 0, 0}, app_array_)
    {
        static constexpr const auto desc = report_desc();
        constexpr hid::report_protocol rp{hid::rdf::ce_descriptor_view(desc)};
        report_info_ = rp;
    }
};

struct usb_manager {
    static usb::zephyr::udc_mac &mac() { return instance().mac_; }
    static usb::df::device &device() { return instance().device_; }
    static bool active() { return device().is_open(); }

    static usb_manager &instance()
    {
        static usb_manager um;
        return um;
    }

    void select_config(hid_config_t conf)
    {
        if (conf == next_config_.load()) {
            return;
        }
        auto config = next_config_.exchange(conf | launch_flag | change_flag);
        if (config & launch_flag) {
            return;
        }
        [[maybe_unused]] auto result = mac().queue_task([]() { instance().change_config(); });
        assert(result == usb::result::OK);
    }

  private:
    void change_config()
    {
        next_config_.fetch_and(~launch_flag);
        // pretend that the device is disconnected
        if (device().is_open()) {
            device().close();
            k_msleep(80);
        }
        // attempt to avoid unnecessary races
        int new_conf = next_config_.fetch_and(~(launch_flag | change_flag));
        for (; new_conf & (launch_flag | change_flag);
             new_conf = next_config_.fetch_and(~(launch_flag | change_flag))) {
            k_msleep(50); // TODO: this is guesswork so far
        }

        open_config((hid_config_t)new_conf);
    }

    void open_config(hid_config_t conf)
    {
        static constexpr auto speed = usb::speed::FULL;
        static usb::df::hid::function usb_kb{
            keyboard_app::handle(), usb::hid::boot_protocol_mode::KEYBOARD};
        static usb::df::hid::function usb_mouse{mouse_app::handle()};
        static usb::df::hid::function usb_command{command_app::usb_handle()};
        static usb::df::hid::function usb_controls{controls_app::handle()};
        static usb::df::hid::function usb_gamepad{gamepad_app::handle()};
        static usb::df::microsoft::xfunction usb_xpad{gamepad_app::handle()};
#if DEVICE_HAS_BATTERY
        static usb::df::hid::function usb_battery{hid_battery_app::handle()};
#endif

        constexpr auto config_header =
            usb::df::config::header(usb::df::config::power::bus(500, true));
        const auto shared_config_elems = usb::df::config::join_elements(
            usb::df::hid::config(usb_kb, speed, usb::endpoint::address(0x81), 1),
            usb::df::hid::config(usb_mouse, speed, usb::endpoint::address(0x82), 1),
            usb::df::hid::config(usb_command, speed, usb::endpoint::address(0x83), 10),
            usb::df::hid::config(usb_controls, speed, usb::endpoint::address(0x84), 1)
#if DEVICE_HAS_BATTERY
                ,
            usb::df::hid::config(usb_battery, speed, usb::endpoint::address(0x86), 1)
            // not very useful at the moment
#endif
        );
        static const auto inactive_config = usb::df::config::make_config(config_header,
            usb::df::hid::config(usb_command, speed, usb::endpoint::address(0x83), 10)
#if DEVICE_HAS_BATTERY
                ,
            usb::df::hid::config(usb_battery, speed, usb::endpoint::address(0x86), 1)
#endif
            );

        static const auto base_config =
            usb::df::config::make_config(config_header, shared_config_elems);

        static const auto gamepad_config =
            usb::df::config::make_config(config_header, shared_config_elems,
                usb::df::hid::config(usb_gamepad, speed, usb::endpoint::address(0x85), 1));

        static const auto xpad_config =
            usb::df::config::make_config(config_header, shared_config_elems,
                usb::df::microsoft::xconfig(
                    usb_xpad, usb::endpoint::address(0x85), 1, usb::endpoint::address(0x05), 255));

        printk("USB config changing to %s\n", magic_enum::enum_name(conf).data());
        switch (conf) {
        case Hid_Empty:
            ms_enum_.set_config({});
            device_.set_config(inactive_config);
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

    usb_manager()
    {
        device_.set_power_event_delegate([](usb::df::device &dev, usb::df::device::event ev) {
            using event = enum usb::df::device::event;
            switch (ev) {
            case event::CONFIGURATION_CHANGE:
                // printk("USB configured: %u, granted current: %uuA\n", dev.configured(), dev.granted_bus_current_uA());
                break;
            case event::POWER_STATE_CHANGE:
                // printk("USB power state: L%u, granted current: %uuA\n", 3 - static_cast<uint8_t>(dev.power_state()), dev.granted_bus_current_uA());
                switch (dev.power_state()) {
                case usb::power::state::L2_SUSPEND:
                    // TODO: handle suspend, maybe only when the HID target is USB?
                    // TODO: stop battery charging, maybe only if dev.configured(),
                    // to distinguish between USB host and charger

                    /* Log("L2 Suspend\n"); */
                    // uncomment when the waking stuff works.
                    PowerMode_SetUsbAwake(false);
                    break;
                case usb::power::state::L0_ON:
                    //TODO: handle wakeup, only if in suspend

                    /* Log("L0 ON\n"); */
                    PowerMode_SetUsbAwake(true);
                    break;
                default:
                    /* Log("L state %i\n", (int)ev); */
                    break;
                }
                break;
            }
        });
    }

    usb::zephyr::udc_mac mac_{DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0))};
    usb::df::microsoft::alternate_enumeration<usb::speeds(usb::speed::FULL)> ms_enum_{};
    usb::df::device_instance<usb::speeds(usb::speed::FULL)> device_{mac_, product_info, ms_enum_};
    static constexpr int change_flag = 0x100;
    static constexpr int launch_flag = 0x200;
    std::atomic<int> next_config_{0xff};
};

extern "C" void USB_EnableHid()
{
    printk(" === Usb Enable Hid\n");
    usb_manager::instance().select_config(HID_GetGamepadActive() ? Hid_Full : Hid_NoGamepad);
}

extern "C" void USB_DisableHid()
{
    printk(" === Usb Disable Hid\n");
    usb_manager::instance().select_config(Hid_Empty);
}

extern "C" void USB_RemoteWakeup()
{
    printk("USB: requesting remote wakeup\n");
    usb_manager::instance().mac().queue_task([]() {
        if (usb_manager::instance().device().remote_wakeup()) {
            printk("USB: sending remote wakeup\n");
        } else {
            printk("USB: remote wakeup disabled\n");
        }
    });
}

#if DEVICE_IS_UHK80_RIGHT

using multi_hid_full = multi_hid<keyboard_app, mouse_app, command_app, controls_app, gamepad_app>;
using multi_hid_nopad = multi_hid<keyboard_app, mouse_app, command_app, controls_app>;

struct hogp_manager {
    static hogp_manager &instance()
    {
        static hogp_manager hm;
        return hm;
    }

    static bool active() { return instance().hogp_nopad_.active(); }

    void select_config(hid_config_t conf)
    {
        switch (conf) {
        case Hid_Empty:
            hogp_nopad_.stop();
            break;

        case Hid_NoGamepad:
            hogp_nopad_.start();
            break;

        default:
            hogp_nopad_.start();
            break;
        }
    }

    const bluetooth::zephyr::hid::service &main_service() { return hogp_nopad_; }

  private:
    hogp_manager() {}

    static const auto security = bluetooth::zephyr::hid::security::ENCRYPT;
    static const auto features = bluetooth::zephyr::hid::flags::NORMALLY_CONNECTABLE |
                                 bluetooth::zephyr::hid::flags::REMOTE_WAKE;

    bluetooth::zephyr::hid::service_instance<hid::report_protocol_properties(
                                                 multi_hid_nopad::report_desc()),
        bluetooth::zephyr::hid::boot_protocol_mode::KEYBOARD>
        hogp_nopad_{multi_hid_nopad::handle(), security, features};
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

bool app_base::active() const
{
    if (!get_transport()) {
        return false;
    }
#if DEVICE_IS_UHK80_RIGHT
    if (get_transport() == &hogp_manager::instance().main_service()) {
        return true;
    }
#endif
    return usb_manager::instance().device().power_state() == usb::power::state::L0_ON;
}

void hidmgr_set_transport(const hid::transport *tp)
{
    connection_id_t usbHidConnId = DEVICE_IS_UHK80_LEFT ? ConnectionId_UsbHidLeft : ConnectionId_UsbHidRight;

    // tp is the transport of the keyboard app
    if (tp == nullptr) {
        printk(" === set transport null\n");
        Connections_SetState(ConnectionId_BtHid, ConnectionState_Disconnected);
        Connections_SetState(usbHidConnId, ConnectionState_Disconnected);
    }
#if DEVICE_IS_UHK80_RIGHT
    else if (tp == &hogp_manager::instance().main_service()) {
        printk(" === set transport hogp\n");
        Connections_SetState(ConnectionId_BtHid, ConnectionState_Ready);
        Connections_SetState(usbHidConnId, ConnectionState_Disconnected);
    }
#endif
    else {
        printk(" === set transport usb\n");
        Connections_SetState(ConnectionId_BtHid, ConnectionState_Disconnected);
        Connections_SetState(usbHidConnId, ConnectionState_Ready);
    }
}

static bool gamepadActive = true;

extern "C" bool HID_GetGamepadActive()
{
    return gamepadActive;
}

extern "C" void HID_SetGamepadActive(bool active)
{
    gamepadActive = active;
    if (usb_manager::active()) {
        USB_EnableHid();
    }
#if DEVICE_IS_UHK80_RIGHT
    if (hogp_manager::active()) {
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

extern "C" void USB_SetSerialNumber(uint32_t serialNumber)
{
    // Ensure UsbSerialNumber has enough space
    if (sizeof(UsbSerialNumber) < 5) {
        return;
    }

    // Convert each pair of decimal digits into a single byte
    for (uint8_t i = 4; i < 255; --i) {
        uint8_t byte = 0;
        for (uint8_t j = 0; j < 2; ++j) {
            uint8_t digit = serialNumber % 10;
            serialNumber /= 10;

            byte |= digit << (j * 4);
        }

        UsbSerialNumber[i] = byte;
    }
}
