#include "keyboard_app.hpp"
extern "C" {
#include "hid/transport.h"
#include "utils.h"
#if __has_include(<zephyr/sys/printk.h>)
    #include <zephyr/sys/printk.h>
#endif
}

keyboard_app &keyboard_app::usb_handle()
{
    static keyboard_app app{nkro_report_protocol()};
    return app;
}
#if DEVICE_IS_UHK80_RIGHT
keyboard_app &keyboard_app::ble_handle()
{
    static keyboard_app ble_app{hid::report_protocol::from_descriptor<report_desc()>()};
    return ble_app;
}
#endif

void keyboard_app::set_rollover(rollover_t mode)
{
    if (rollover_ == mode) {
        return;
    }

    rollover_ = mode;
    if (this == &usb_handle()) {
        // swap the HID report descriptor, which needs USB re-enumeration
        report_info_ =
            mode == rollover_t::ROLLOVER_N_KEY ? nkro_report_protocol() : default_report_protocol();
    }
    if (prot_ == hid::protocol::BOOT) {
        return;
    }
    reset_keys();
}

void keyboard_app::reset_keys()
{
    keys_.reset();
    // TODO: make sure that no keys are pressed when this happens
    // or send an empty report on the virtual keyboard that is deactivated by this switch?
    if ((rollover_ == rollover_t::ROLLOVER_N_KEY) &&
        (rollover_override_ == rollover_t::ROLLOVER_N_KEY)) {
        keys_[0].nkro = {};
        keys_[1].nkro = {};
    } else {
        keys_[0].sixkro = {};
        keys_[1].sixkro = {};
    }
}

void keyboard_app::start(hid::protocol prot)
{
    prot_ = prot;
    rollover_override_ = {};

    // start receiving reports
    receive_report(&leds_buffer_);

    reset_keys();

    Hid_TransportStateChanged(
        (this == &usb_handle()) ? HID_TRANSPORT_USB : HID_TRANSPORT_BLE, true);
}

void keyboard_app::stop()
{
    Hid_TransportStateChanged(
        (this == &usb_handle()) ? HID_TRANSPORT_USB : HID_TRANSPORT_BLE, false);
}

bool keyboard_app::using_nkro() const
{
    return (prot_ == hid::protocol::REPORT) && (rollover_ == rollover_t::ROLLOVER_N_KEY) &&
           (rollover_override_ == rollover_t::ROLLOVER_N_KEY);
}

int keyboard_app::send_report(const hid_keyboard_report_t &report)
{
    if (!has_transport()) {
        return hid::result(hid::result::not_connected).to_int();
    }
    auto buf = move_to_buffer(report);
    auto result = application::send_report(buf, hid::report::type::INPUT);

#if DEVICE_IS_UHK80_RIGHT
    if (result == hid::result::not_enough_memory) {
        // this only happens on Android with NKRO mode when the transport MTU is too small
        assert(using_nkro() and (this == &ble_handle()));
        printk("keyboard NKRO mode fails, falling back to 6KRO\n");

        rollover_override_ = rollover_t::ROLLOVER_6_KEY;
        buf = move_to_buffer(report);
        result = application::send_report(buf, hid::report::type::INPUT);
    }
#endif
    if (result == hid::result::ok) {
        keys_.swap_sides();
    }
    return result.to_int();
}

std::span<const uint8_t> keyboard_app::move_to_buffer(const hid_keyboard_report_t &report)
{
    auto buf_idx = keys_.active_side();
    if (using_nkro()) {
        auto &keys_nkro = keys_[buf_idx].nkro;

        ::memcpy(&keys_nkro.modifiers, &report.modifiers, sizeof(report.modifiers));
        ::memcpy(&keys_nkro.scancodes, &report.bitfield, sizeof(report.bitfield));

        return std::span<const uint8_t>(
            reinterpret_cast<const uint8_t *>(&keys_nkro), sizeof(keys_nkro));
    }

    if (prot_ == hid::protocol::BOOT) {
        auto &keys_6kro = keys_[buf_idx].boot;

        ::memcpy(&keys_6kro.modifiers, &report.modifiers, sizeof(report.modifiers));
        keys_6kro.scancodes.reset();
        for (auto code = uint8_t(LOWEST_SCANCODE); code <= uint8_t(HIGHEST_SCANCODE); ++code) {
            keys_6kro.scancodes.set(
                scancode(code), test_bit(code - uint8_t(LOWEST_SCANCODE), report.bitfield));
        }

        return std::span<const uint8_t>(
            reinterpret_cast<const uint8_t *>(&keys_6kro), sizeof(keys_6kro));
    }

    {
        auto &keys_6kro = keys_[buf_idx].sixkro;

        ::memcpy(&keys_6kro.modifiers, &report.modifiers, sizeof(report.modifiers));
        keys_6kro.scancodes.reset();
        for (auto code = uint8_t(LOWEST_SCANCODE); code <= uint8_t(HIGHEST_SCANCODE); ++code) {
            keys_6kro.scancodes.set(
                scancode(code), test_bit(code - uint8_t(LOWEST_SCANCODE), report.bitfield));
        }

        return std::span<const uint8_t>(
            reinterpret_cast<const uint8_t *>(&keys_6kro), sizeof(keys_6kro));
    }
}

keyboard_app::leds_boot_report keyboard_app::get_leds() const
{
    auto *ptr = reinterpret_cast<const uint8_t *>(&leds_buffer_);
    if ((prot_ != hid::protocol::BOOT) and (sizeof(leds_report) > sizeof(leds_boot_report))) {
        ptr += sizeof(leds_report) - sizeof(leds_boot_report);
    }
    return reinterpret_cast<const leds_boot_report &>(*ptr);
}

void keyboard_app::set_report(hid::report::type type, const std::span<const uint8_t> &data)
{
    if ((prot_ == hid::protocol::REPORT) &&
        ((type != hid::report::type::OUTPUT) || (data.front() != LEDS_REPORT_ID))) {
        return;
    }
    // only one report is receivable, the LEDs
    // offset it if report ID is not present due to BOOT protocol
    [[maybe_unused]] auto &leds =
        *reinterpret_cast<const uint8_t *>(data.data() + static_cast<size_t>(prot_));

    Hid_KeyboardLedsStateChanged((this == &usb_handle()) ? HID_TRANSPORT_USB : HID_TRANSPORT_BLE);

    // always keep receiving new reports
    // if the report data is processed immediately, the same buffer can be used
    receive_report(&leds_buffer_);
}

void keyboard_app::in_report_sent(const std::span<const uint8_t> &data)
{
#if DEVICE_IS_UHK80_RIGHT
    if ((prot_ == hid::protocol::REPORT) && (data.front() != KEYS_NKRO_REPORT_ID) &&
        (data.front() != KEYS_6KRO_REPORT_ID)) {
        return;
    }
#endif
    Hid_KeyboardReportSentCallback((this == &usb_handle()) ? HID_TRANSPORT_USB : HID_TRANSPORT_BLE);
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
    const auto &keys = keys_[keys_.inactive_side()];
#if DEVICE_IS_UHK60
    if (using_nkro()) {
        auto *report = reinterpret_cast<keys_nkro_report *>(buffer.data());
        *report = keys.nkro;
        application::send_report(report);
    } else {
        static_assert(KEYS_6KRO_REPORT_ID == 0);
        auto *report = reinterpret_cast<keys_6kro_report *>(buffer.data());
        *report = keys.sixkro;
        application::send_report(report);
    }
#else
    switch (select.id()) {
    case KEYS_6KRO_REPORT_ID: {
        auto *report = reinterpret_cast<keys_6kro_report *>(buffer.data());
        if (using_nkro()) {
            *report = {};
        } else {
            *report = keys.sixkro;
        }
        application::send_report(report);
        break;
    }
    case KEYS_NKRO_REPORT_ID: {
        auto *report = reinterpret_cast<keys_nkro_report *>(buffer.data());
        if (using_nkro()) {
            *report = keys.nkro;
        } else {
            *report = {};
        }
        application::send_report(report);
        break;
    }
    default: {
        auto *report = reinterpret_cast<keys_boot_report *>(buffer.data());
        *report = keys.boot;
        application::send_report(report);
        break;
    }
    }
#endif
}
