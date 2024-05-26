#include "keyboard_app.hpp"
#include "zephyr/sys/printk.h"

keyboard_app &keyboard_app::handle()
{
    static keyboard_app app{};
    return app;
}

void keyboard_app::set_rollover(rollover mode)
{
    if (rollover_ == mode) {
        return;
    }

    rollover_ = mode;
    if (prot_ == hid::protocol::BOOT) {
        return;
    }

    keys_.reset();
    // TODO: make sure that no keys are pressed when this happens
    // or send an empty report on the virtual keyboard that is deactivated by this switch?
    if ((rollover_ == rollover::N_KEY) && (rollover_override_ == rollover::N_KEY)) {
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

    // TODO start handling keyboard events
    keys_.reset();
    if (prot == hid::protocol::BOOT) {
        keys_[0].boot = {};
        keys_[1].boot = {};
    } else if ((rollover_ == rollover::N_KEY) && (rollover_override_ == rollover::N_KEY)) {
        keys_[0].nkro = {};
        keys_[1].nkro = {};
    } else {
        keys_[0].sixkro = {};
        keys_[1].sixkro = {};
    }
}

void keyboard_app::stop()
{
    // TODO stop handling keyboard events
}

bool keyboard_app::using_nkro() const
{
    return (prot_ == hid::protocol::REPORT) && (rollover_ == rollover::N_KEY) &&
           (rollover_override_ == rollover::N_KEY);
}

void keyboard_app::set_report_state(const keys_nkro_report_base<> &data)
{
    auto buf_idx = keys_.active_side();
    if (!using_nkro()) {
        if (prot_ == hid::protocol::BOOT) {
            auto &keys_6kro = keys_[buf_idx].boot;
            keys_6kro.modifiers = data.modifiers;
            keys_6kro.scancodes.reset();
            for (auto code = LOWEST_SCANCODE; code <= HIGHEST_SCANCODE;
                 code = static_cast<decltype(code)>(static_cast<uint8_t>(code) + 1)) {
                keys_6kro.scancodes.set(code, data.test(code));
            }
        } else {
            auto &keys_6kro = keys_[buf_idx].sixkro;
            keys_6kro.modifiers = data.modifiers;
            keys_6kro.scancodes.reset();
            for (auto code = LOWEST_SCANCODE; code <= HIGHEST_SCANCODE;
                 code = static_cast<decltype(code)>(static_cast<uint8_t>(code) + 1)) {
                keys_6kro.scancodes.set(code, data.test(code));
            }
        }
        send_6kro_buffer(buf_idx);
    } else {
        auto &keys_nkro = keys_[buf_idx].nkro;
        // fill up the report
        keys_nkro.modifiers = data.modifiers;
        keys_nkro.scancodes = data.scancodes;

        send_nkro_buffer(buf_idx);
    }
}

void keyboard_app::send_6kro_buffer(uint8_t buf_idx)
{
    if (!keys_.differs() || !has_transport()) {
        return;
    }
    auto result = hid::result::INVALID;
    auto &report = keys_[buf_idx];
    if (prot_ == hid::protocol::BOOT) {
        result = send_report(&report.boot);
    } else {
        result = send_report(&report.sixkro);
    }

    // swap sides only if the callback hasn't done yet
    if (result == hid::result::OK) {
        keys_.compare_swap_copy(buf_idx);
    }
}

void keyboard_app::send_nkro_buffer(uint8_t buf_idx)
{
    if (!keys_.differs() || !has_transport()) {
        return;
    }
    auto result = send_report(&keys_[buf_idx].nkro);
    if (result == hid::result::NO_MEMORY) {
        printk("keyboard NKRO mode fails, falling back to 6KRO\n");

        // save key state
        keys_nkro_report_base<> data{
            .modifiers = keys_[buf_idx].nkro.modifiers, .scancodes = keys_[buf_idx].nkro.scancodes};

        // switch report layout
        rollover_override_ = rollover::SIX_KEY;
        keys_.reset();
        keys_[0].sixkro = {};
        keys_[1].sixkro = {};

        // apply current state
        set_report_state(data);

    } else if (result == hid::result::OK) {
        keys_.compare_swap_copy(buf_idx);
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

    // TODO use LEDs bitfields
    printk("keyboard LED status: %x\n", leds);

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
    auto buf_idx = keys_.indexof(data.data());
    if (!keys_.compare_swap_copy(buf_idx)) {
        if (using_nkro()) {
            send_nkro_buffer(1 - buf_idx);
        } else {
            send_6kro_buffer(1 - buf_idx);
        }
    }
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
    switch (select.id()) {
    case KEYS_6KRO_REPORT_ID: {
        auto *report = reinterpret_cast<keys_6kro_report *>(buffer.data());
        if (using_nkro()) {
            *report = {};
        } else {
            *report = keys.sixkro;
        }
        send_report(report);
        break;
    }
    case KEYS_NKRO_REPORT_ID: {
        auto *report = reinterpret_cast<keys_nkro_report *>(buffer.data());
        if (using_nkro()) {
            *report = keys.nkro;
        } else {
            *report = {};
        }
        send_report(report);
        break;
    }
    default: {
        auto *report = reinterpret_cast<keys_boot_report *>(buffer.data());
        *report = keys.boot;
        send_report(report);
        break;
    }
    }
}
