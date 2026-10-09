extern "C" {
#include "debug.h"
#ifdef CONFIG_UHK_USB_LEFT_RELAY
    #include "connections.h"
    #include "usb_left_relay_uhk.h"
#include "event_scheduler.h"
#endif
#include "device.h"
#include "key_states.h"
#include "logger.h"
#include "power_mode.h"
#include "timer.h"
#include "usb_report_updater.h"
#include "usb_semaphore.h"
#include "usb_state.h"
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
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#include "usb/df/class/hid.hpp"
#include "usb/df/device.hpp"
#include "usb/df/vendor/microsoft/os_extension.hpp"
#include "usb/df/vendor/microsoft/xinput.hpp"
#include <magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

#ifdef CONFIG_UHK_USB_LEFT_RELAY
class relay_hid_function : public usb::df::hid::function {
  public:
    using usb::df::hid::function::function;
    void cancel()
    {
        if (ep_in_handle().valid()) {
            (void)cancel_ep(ep_in_handle());
        }
    }
};
static relay_hid_function *relayFunctions[RelayKind_Count + 1];
/* udc_ep_dequeue waits on the nRF driver. Run it on c2usb's worker,
 * keeping main-loop scanning and route selection nonblocking. */
static uint32_t cancellationGeneration[RelayKind_Count + 1];
static atomic_t lifecycleQueued, lifecycleFinished;
static uint32_t fenceGeneration;
template <uint8_t Kind>
static void cancelOnUsbThread()
{
    uint32_t generation;
    if (Hid_LocalUsbCancellationPending(Kind, &generation) &&
        generation == cancellationGeneration[Kind] && relayFunctions[Kind]) {
        relayFunctions[Kind]->cancel();
    }
}
static void lifecycleBarrier()
{
    Hid_LocalUsbFinishFence(fenceGeneration);
    atomic_set(&lifecycleFinished, fenceGeneration);
    atomic_set(&lifecycleQueued, 0);
}
static void lifecycleCancel();
extern "C" void Hid_LocalUsbRequestFence(void)
{
    EventVector_WakeMain();
}

#endif
static uint8_t usb_serial_number[5]{};

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

    void select_config()
    {
        // pretend that the device is disconnected
        if (device().is_open()) {
            const unsigned bus_reset_delay_ms = 100;
            device().close();
#ifdef __ZEPHYR__
            k_msleep(bus_reset_delay_ms);
#else
            // TODO: use non-blocking delay
            SDK_DelayAtLeastUs(bus_reset_delay_ms * 1000, SystemCoreClock);
#endif
        }

        using namespace usb::df;

        static constexpr auto speed = usb::speed::FULL;
        static
#ifdef CONFIG_UHK_USB_LEFT_RELAY
            relay_hid_function
#else
            usb::df::hid::function
#endif
                usb_kb{keyboard_app::usb_handle(), usb::hid::boot_protocol_mode::KEYBOARD};
        static
#ifdef CONFIG_UHK_USB_LEFT_RELAY
            relay_hid_function
#else
            usb::df::hid::function
#endif
                usb_mouse{mouse_app::usb_handle()};
        static usb::df::hid::function usb_command{command_app::usb_handle()};
        static
#ifdef CONFIG_UHK_USB_LEFT_RELAY
            relay_hid_function
#else
            usb::df::hid::function
#endif
                usb_controls{controls_app::usb_handle()};

#ifdef CONFIG_UHK_USB_LEFT_RELAY
        relayFunctions[RelayKind_Keyboard] = &usb_kb;
        relayFunctions[RelayKind_Mouse] = &usb_mouse;
        relayFunctions[RelayKind_Controls] = &usb_controls;
#endif
        constexpr auto config_header =
            config::header(config::power::bus(500, config::remote_wakeup));

        static const auto base_config = config::make_config(config_header,
            usb_kb.config_entry(speed, usb::endpoint::address(0x81), 1),
            usb_mouse.config_entry(speed, usb::endpoint::address(0x82), 1),
            usb_command.config_entry(speed, usb::endpoint::address(0x83), 8),
            usb_controls.config_entry(speed, usb::endpoint::address(0x84), 1));

        device_.set_config(base_config);
        device_.open();
    }

    usb_manager()
    {

        device_.set_power_event_delegate([](usb::df::device &dev, usb::df::device::event ev) {
            using event = enum usb::df::device::event;
            if ((ev & event::POWER_STATE_CHANGE) != event::NONE) {
                switch (dev.power_state()) {
                case usb::power::state::L2_SUSPEND:
#if DEVICE_IS_UHK60
                    if (dev.configured()) {
                        UsbState_SetHostSuspended(true);
                    }
#else
                    UsbState_SetHostSuspended(true);
#endif
                    break;
                case usb::power::state::L0_ON:
                    UsbState_SetHostSuspended(false);
                    break;
                case usb::power::state::L3_OFF:
                    // Detached - there is no host to be suspended. Leaving the flag set
                    // would make it stale until the next enumeration, which only stays
                    // harmless as long as every reader also checks UsbState_TransportUp.
                    UsbState_SetHostSuspended(false);
                    break;
                default:
                    break;
                }
            }
            if ((ev & event::CONFIGURATION_CHANGE) != event::NONE) {
                // reset the semaphore on USB configuration or reset
#ifdef CONFIG_UHK_USB_LEFT_RELAY
                Hid_LocalUsbConfigurationChanged();
                // The main loop retires only the local route whose generation changed.
#else
                UsbSemaphore_Clear();
#endif

                if (dev.configured()) {
                    // A host that just selected a configuration is awake and listening.
                    // Reconfiguration also clears the host's remote wakeup arming, so a
                    // stale suspended state here would make USB_RemoteWakeup() fail with
                    // EPERM indefinitely.
                    UsbState_SetHostSuspended(false);
                }
            }
        });
    }

#ifdef __ZEPHYR__
    usb::zephyr::udc_mac mac_{DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 256,
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

#ifdef CONFIG_UHK_USB_LEFT_RELAY
static void lifecycleCancel()
{
    fenceGeneration = Hid_LocalUsbGeneration();
    uint8_t mask = Hid_LocalUsbRetireTickets();
    for (uint8_t kind = 1; kind <= RelayKind_Count; ++kind) {
        if ((mask & (1u << kind)) && relayFunctions[kind]) {
            relayFunctions[kind]->cancel();
        }
    }
    /* nRF posts dequeued completions before waking the cancellation caller.
     * FIFO ordering places this barrier after those completions. */
    auto result = usb_manager::mac().queue_task(etl::delegate<void()>::create<lifecycleBarrier>());
    if (result != usb::result::ok) {
        atomic_set(&lifecycleQueued, 0);
    }
}
extern "C" void Hid_LocalUsbService(void)
{
    /* Re-requesting a fence while idle is cheap; it is only needed when a
     * generation has not yet passed its ordered USB-worker barrier. */
    uint32_t generation = Hid_LocalUsbGeneration();
    if (uint32_t(atomic_get(&lifecycleFinished)) == generation) {
        return;
    }
    if (atomic_cas(&lifecycleQueued, 0, 1)) {
        auto result =
            usb_manager::mac().queue_task(etl::delegate<void()>::create<lifecycleCancel>());
        if (result != usb::result::ok) {
            atomic_set(&lifecycleQueued, 0);
        }
    }
}
template <uint8_t Kind>
static void sendOnUsbThread()
{
    Hid_LocalUsbSendQueued(Kind);
}
extern "C" bool Hid_LocalUsbQueueSend(uint8_t kind)
{
    etl::delegate<void()> task;
    switch (kind) {
    case RelayKind_Keyboard:
        task = etl::delegate<void()>::create<sendOnUsbThread<RelayKind_Keyboard>>();
        break;
    case RelayKind_Mouse:
        task = etl::delegate<void()>::create<sendOnUsbThread<RelayKind_Mouse>>();
        break;
    case RelayKind_Controls:
        task = etl::delegate<void()>::create<sendOnUsbThread<RelayKind_Controls>>();
        break;
    default:
        return false;
    }
    return usb_manager::mac().queue_task(task) == usb::result::ok;
}
extern "C" bool Hid_LocalUsbCancel(uint8_t kind)
{
    uint32_t generation;
    if (!Hid_LocalUsbCancellationPending(kind, &generation)) {
        return true;
    }
    cancellationGeneration[kind] = generation;
    etl::delegate<void()> task;
    switch (kind) {
    case RelayKind_Keyboard:
        task = etl::delegate<void()>::create<cancelOnUsbThread<RelayKind_Keyboard>>();
        break;
    case RelayKind_Mouse:
        task = etl::delegate<void()>::create<cancelOnUsbThread<RelayKind_Mouse>>();
        break;
    case RelayKind_Controls:
        task = etl::delegate<void()>::create<cancelOnUsbThread<RelayKind_Controls>>();
        break;
    default:
        return false;
    }
    return usb_manager::mac().queue_task(task) == usb::result::ok;
}
#endif

#ifndef __ZEPHYR__
extern "C" void USB0_IRQHandler(void)
{
    ISR_LIFE_START(usb);
    Trace_Printc("<i6");
    if (usb::df::nxp::mcux_mac::notification_routing) {
        usb_manager::mac().handle_irq();
    } else {
        USB_DeviceKhciIsrFunction(BuspalCompositeUsbDevice.device_handle);
    }
    Trace_Printc(">");
    ISR_LIFE_END(usb);
    SDK_ISR_EXIT_BARRIER;
}
#endif

extern "C" void USB_Enable()
{
    assert(!usb_manager::active());
    usb_manager::instance().select_config();
}

#ifdef CONFIG_UHK_USB_LEFT_RELAY
static void reconfigureOnUsbThread()
{
    if (usb_manager::active()) {
        usb_manager::instance().select_config();
    }
}
#endif
extern "C" void USB_Reconfigure()
{
#ifdef CONFIG_UHK_USB_LEFT_RELAY
    (void)usb_manager::mac().queue_task(etl::delegate<void()>::create<reconfigureOnUsbThread>());
    return;
#endif
    if (usb_manager::active()) {
        usb_manager::instance().select_config();
    }
}

extern "C" bool USB_RemoteWakeup()
{
    auto err = usb_manager::instance().device().remote_wakeup();
    if (err.to_int() == -1 /* not permitted; treat it as awake */) {
        UsbState_SetHostSuspended(false);
        LogInf("USB: remote wakeup not permitted, usb suspended: %d\n", UsbState_HostIsSuspended);
    } else if (err != usb::result::ok) {
        LogErr("USB: remote wakeup request failed: %d\n", err.to_int());
    } else {
        LogInf("USB: remote wakeup request succeeded\n");
    }
    return err == usb::result::ok;
}

extern "C" bool USB_IsMsHost(void)
{
    return usb_manager::ms_host();
}

extern "C" void USB_SetSerialNumber(uint32_t serialNumber)
{
    const uint8_t serialNumberByteCount = 5;

    static_assert(
        sizeof(usb_serial_number) >= serialNumberByteCount, "usb_serial_number size is too small");

    // Convert each pair of decimal digits into a single byte
    for (uint8_t i = serialNumberByteCount - 1; i < 255; --i) {
        uint8_t byte = 0;
        for (uint8_t j = 0; j < 2; ++j) {
            uint8_t digit = serialNumber % 10;
            serialNumber /= 10;

            byte |= digit << (j * 4);
        }

        usb_serial_number[i] = byte;
    }
}

#ifdef CONFIG_UHK_USB_LEFT_RELAY
extern "C" bool Hid_LocalUsbWakeAllowed()
{
    return usb_manager::device().allows_remote_wakeup();
}
#endif
