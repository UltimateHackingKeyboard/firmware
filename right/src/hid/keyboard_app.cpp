#include "keyboard_app.hpp"
extern "C" {
#include "hid/transport.h"
#include "usb_state.h"
#include "utils.h"
#if __has_include(<zephyr/sys/printk.h>)
    #include <zephyr/sys/printk.h>
#endif
}

void keyboard_app::set_rollover(rollover_t mode)
{
    // swap the HID report descriptor, which needs USB re-enumeration
    report_info_ =
        mode == rollover_t::ROLLOVER_N_KEY ? nkro_report_protocol() : default_report_protocol();
}

hid::session &keyboard_app::start(const hid::session::params &params)
{
    assert(!session_.has_value());
    UsbState_SetUsbTransportUp(true);
    return session_.emplace(params);
}

void keyboard_app::stop(hid::session &sess)
{
    assert(&sess == &session_.value());
    UsbState_SetUsbTransportUp(false);
    return session_.reset();
}

void key_report_buffer::reset_to(hid::protocol prot, rollover_t rollover)
{
    reset();
    // make sure that no keys are pressed when this happens
    // or send an empty report on the virtual keyboard that is deactivated by this switch?
    if (prot == hid::protocol::BOOT) {
        (*this)[0].boot = {};
        (*this)[1].boot = {};
    } else if (rollover == rollover_t::ROLLOVER_N_KEY) {
        (*this)[0].nkro = {};
        (*this)[1].nkro = {};
    } else {
        (*this)[0].sixkro = {};
        (*this)[1].sixkro = {};
    }
}

std::span<const uint8_t> key_report_buffer::insert(const hid_keyboard_report_t &report)
{
    auto buf_idx = active_side();
    if (mode == MODE_NKRO) {
        auto &keys_nkro = (*this)[buf_idx].nkro;

        ::memcpy(&keys_nkro.modifiers, &report.modifiers, sizeof(report.modifiers));
        ::memcpy(
            static_cast<void *>(&keys_nkro.scancodes), &report.bitfield, sizeof(report.bitfield));

        return std::span<const uint8_t>(
            reinterpret_cast<const uint8_t *>(&keys_nkro), sizeof(keys_nkro));
    }

    if (mode == MODE_BOOT) {
        auto &keys_6kro = (*this)[buf_idx].boot;

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
        auto &keys_6kro = (*this)[buf_idx].sixkro;

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

keyboard_session::leds_boot_report keyboard_session::get_leds_report() const
{
    auto *ptr = reinterpret_cast<const uint8_t *>(&leds_buffer_);
    if ((protocol() != hid::protocol::BOOT) and (sizeof(leds_report) > sizeof(leds_boot_report))) {
        ptr += sizeof(leds_report) - sizeof(leds_boot_report);
    }
    return reinterpret_cast<const leds_boot_report &>(*ptr);
}

void keyboard_session::set_report(hid::report::type type, const std::span<const uint8_t> &data)
{
    // only one report is receivable, the LEDs
    if (type != hid::report::type::OUTPUT) {
        return;
    }

    keyboard_leds_changed_callback(*this);

    // always keep receiving new reports
    // if the report data is processed immediately, the same buffer can be used
    receive_report(&leds_buffer_);
}

void keyboard_session::report_sent(const std::span<const uint8_t> &data)
{
    keyboard_report_sent_callback(*this);
}

std::span<const uint8_t> keyboard_session::get_report(
    hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (protocol() == hid::protocol::BOOT) {
        if (select == keyboard_app::keys_boot_report::selector()) {
            assert(buffer.size() >= sizeof(keyboard_app::keys_boot_report));
            std::ignore = new (buffer.data()) keyboard_app::keys_boot_report{};
            return buffer.subspan(0, sizeof(keyboard_app::keys_boot_report));
        }
        if (select == leds_boot_report::selector()) {
            assert(buffer.size() >= sizeof(leds_boot_report));
            auto *ptr = new (buffer.data()) leds_boot_report{};
            ptr->leds = leds_buffer_.leds;
            return buffer.subspan(0, sizeof(leds_boot_report));
        }
        return {};
    }

    if (select == leds_report::selector()) {
        assert(buffer.size() >= sizeof(leds_report));
        auto *ptr = new (buffer.data()) leds_report{};
        ptr->leds = leds_buffer_.leds;
        return buffer.subspan(0, sizeof(leds_report));
    }
    if constexpr (report_ids::IN_KEYBOARD_NKRO == 0) {
        // no report ID, use the rollover mode to determine which report to send
        if (select.type() == hid::report::type::INPUT) {
            if (HID_GetKeyboardRollover() == rollover_t::ROLLOVER_N_KEY) {
                assert(buffer.size() >= sizeof(keyboard_app::keys_nkro_report));
                std::ignore = new (buffer.data()) keyboard_app::keys_nkro_report{};
                return buffer.subspan(0, sizeof(keyboard_app::keys_nkro_report));
            } else {
                assert(buffer.size() >= sizeof(keyboard_app::keys_6kro_report));
                std::ignore = new (buffer.data()) keyboard_app::keys_6kro_report{};
                return buffer.subspan(0, sizeof(keyboard_app::keys_6kro_report));
            }
        }
    } else {
        if (select == keyboard_app::keys_6kro_report::selector()) {
            assert(buffer.size() >= sizeof(keyboard_app::keys_6kro_report));
            std::ignore = new (buffer.data()) keyboard_app::keys_6kro_report{};
            return buffer.subspan(0, sizeof(keyboard_app::keys_6kro_report));
        }
        if (select == keyboard_app::keys_nkro_report::selector()) {
            assert(buffer.size() >= sizeof(keyboard_app::keys_nkro_report));
            std::ignore = new (buffer.data()) keyboard_app::keys_nkro_report{};
            return buffer.subspan(0, sizeof(keyboard_app::keys_nkro_report));
        }
    }
    return {};
}
