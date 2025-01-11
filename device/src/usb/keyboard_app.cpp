#include "keyboard_app.hpp"
extern "C" {
#include "connections.h"
#include "usb_compatibility.h"
#include "zephyr/sys/printk.h"
}

keyboard_app &keyboard_app::usb_handle()
{
    static keyboard_app app{};
    return app;
}
#if DEVICE_IS_UHK80_RIGHT
keyboard_app &keyboard_app::ble_handle()
{
    static keyboard_app ble_app{};
    return ble_app;
}
#endif

void keyboard_app::set_rollover(rollover mode)
{
    if (rollover_ == mode) {
        return;
    }

    rollover_ = mode;
    if (prot_ == hid::protocol::BOOT) {
        return;
    }
    reset_keys();
}

void keyboard_app::reset_keys()
{
    keys_ = {};
    // TODO: make sure that no keys are pressed when this happens
    // or send an empty report on the virtual keyboard that is deactivated by this switch?
    if ((rollover_ == rollover::N_KEY) && (rollover_override_ == rollover::N_KEY)) {
        keys_.nkro = {};
        keys_.nkro = {};
    } else {
        keys_.sixkro = {};
        keys_.sixkro = {};
    }
}

static inline connection_id_t usbHidConnId()
{
    return DEVICE_IS_UHK80_LEFT ? ConnectionId_UsbHidLeft : ConnectionId_UsbHidRight;
}

void keyboard_app::start(hid::protocol prot)
{
    prot_ = prot;
    rollover_override_ = {};

    // start receiving reports
    receive_report(&leds_buffer_);

    // TODO start handling keyboard events
    reset_keys();

    Connections_SetState(
        (this == &usb_handle()) ? usbHidConnId() : ConnectionId_BtHid, ConnectionState_Ready);
}

void keyboard_app::stop()
{
    sending_sem_.release();
    // TODO stop handling keyboard events
    Connections_SetState((this == &usb_handle()) ? usbHidConnId() : ConnectionId_BtHid,
        ConnectionState_Disconnected);
}

bool keyboard_app::using_nkro() const
{
    return (prot_ == hid::protocol::REPORT) && (rollover_ == rollover::N_KEY) &&
           (rollover_override_ == rollover::N_KEY);
}

void keyboard_app::set_report_state(const keys_nkro_report_base<> &data)
{
    if (!active()) {
        return;
    }
    if (!sending_sem_.try_acquire_for(SEMAPHORE_RESET_TIMEOUT)) {
        //return;
    }
    auto result = hid::result::INVALID;
    if (!using_nkro()) {
        if (prot_ == hid::protocol::BOOT) {
            auto &keys_6kro = keys_.boot;
            keys_6kro.modifiers = data.modifiers;
            keys_6kro.scancodes.reset();
            for (auto code = LOWEST_SCANCODE; code <= HIGHEST_SCANCODE;
                code = static_cast<decltype(code)>(static_cast<uint8_t>(code) + 1)) {
                keys_6kro.scancodes.set(code, data.test(code));
            }
        } else {
            auto &keys_6kro = keys_.sixkro;
            keys_6kro.modifiers = data.modifiers;
            keys_6kro.scancodes.reset();
            for (auto code = LOWEST_SCANCODE; code <= HIGHEST_SCANCODE;
                code = static_cast<decltype(code)>(static_cast<uint8_t>(code) + 1)) {
                keys_6kro.scancodes.set(code, data.test(code));
            }
        }

        if (prot_ == hid::protocol::BOOT) {
            result = send_report(&keys_.boot);
        } else {
            result = send_report(&keys_.sixkro);
        }
    } else {
        auto &keys_nkro = keys_.nkro;
        // fill up the report
        keys_nkro.modifiers = data.modifiers;
        keys_nkro.scancodes = data.scancodes;

        result = send_report(&keys_.nkro);
        if (result == hid::result::NO_MEMORY) {
            printk("keyboard NKRO mode fails, falling back to 6KRO\n");

            // save key state
            keys_nkro_report_base<> data{
                .modifiers = keys_.nkro.modifiers, .scancodes = keys_.nkro.scancodes};

            // switch report layout
            rollover_override_ = rollover::SIX_KEY;
            keys_ = {};
            keys_.sixkro = {};

            keys_.sixkro.modifiers = data.modifiers;
            keys_.sixkro.scancodes.reset();
            for (auto code = LOWEST_SCANCODE; code <= HIGHEST_SCANCODE;
                code = static_cast<decltype(code)>(static_cast<uint8_t>(code) + 1)) {
                keys_.sixkro.scancodes.set(code, data.test(code));
            }

            result = send_report(&keys_.sixkro);
        }
    }
    if (result != hid::result::OK) {
        sending_sem_.release();
    }
}

void keyboard_app::set_report(hid::report::type type, const std::span<const uint8_t> &data)
{
    if ((prot_ == hid::protocol::REPORT) &&
        ((type != hid::report::type::OUTPUT) || (data.front() != LEDS_REPORT_ID))) {
        return;
    }
    // only one report is receivable, the LEDs
    // offset it if report ID is not present due to BOOT protocol
    auto &leds = *reinterpret_cast<const uint8_t *>(data.data() + static_cast<size_t>(prot_));

    const uint8_t ScrollLockMask = 4;
    const uint8_t CapsLockMask = 2;
    const uint8_t NumLockMask = 1;

    UsbCompatibility_SetKeyboardLedsState(
        (this == &usb_handle()) ? usbHidConnId() : ConnectionId_BtHid, leds & CapsLockMask,
        leds & NumLockMask, leds & ScrollLockMask);

    // always keep receiving new reports
    // if the report data is processed immediately, the same buffer can be used
    receive_report(&leds_buffer_);
}

void keyboard_app::in_report_sent(const std::span<const uint8_t> &data)
{
    if ((prot_ == hid::protocol::REPORT) && (data.front() != KEYS_NKRO_REPORT_ID) &&
        (data.front() != KEYS_6KRO_REPORT_ID)) {
        return;
    }
    sending_sem_.release();
}

void keyboard_app::get_report(hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (prot_ == hid::protocol::REPORT) {
        if ((select != keys_6kro_report::selector()) && (select != keys_nkro_report::selector())) {
            return;
        }
    } else {
        if (select != keys_boot_report::selector()) {
            return;
        }
    }

    // copy to buffer to avoid overwriting data in transit
    switch (select.id()) {
    case KEYS_6KRO_REPORT_ID: {
        auto *report = reinterpret_cast<keys_6kro_report *>(buffer.data());
        if (using_nkro()) {
            *report = {};
        } else {
            *report = keys_.sixkro;
        }
        send_report(report);
        break;
    }
    case KEYS_NKRO_REPORT_ID: {
        auto *report = reinterpret_cast<keys_nkro_report *>(buffer.data());
        if (using_nkro()) {
            *report = keys_.nkro;
        } else {
            *report = {};
        }
        send_report(report);
        break;
    }
    default: {
        auto *report = reinterpret_cast<keys_boot_report *>(buffer.data());
        *report = keys_.boot;
        send_report(report);
        break;
    }
    }
}
