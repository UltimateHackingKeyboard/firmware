#include "keyboard_app.hpp"
#include "zephyr/sys/printk.h"

keyboard_app& keyboard_app::handle()
{
    static keyboard_app app{};
    return app;
}

void keyboard_app::set_rollover(rollover mode)
{
    if (rollover_ == mode)
    {
        return;
    }

    rollover_ = mode;
    if (prot_ == hid::protocol::BOOT)
    {
        return;
    }

    // TODO: make sure that no keys are pressed when this happens
    // or send an empty report on the virtual keyboard that is deactivated by this switch?
    if ((rollover_ == rollover::N_KEY) && (rollover_override_ == rollover::N_KEY))
    {
        keys_nkro_.reset();
        keys_6kro_.reset();
    }
    else
    {
        keys_nkro_.reset();
        keys_6kro_.reset();
    }
}

void keyboard_app::start(hid::protocol prot)
{
    prot_ = prot;
    rollover_override_ = {};

    // start receiving reports
    receive_report(&leds_buffer_);

    // TODO start handling keyboard events
    keys_nkro_.reset();
    keys_6kro_.reset();
}

void keyboard_app::stop()
{
    // TODO stop handling keyboard events
}

void keyboard_app::set_report_state(const keys_nkro_report_base<>& data)
{
    // TODO: report data accessing mutex?

    if ((prot_ == hid::protocol::BOOT) || (rollover_ != rollover::N_KEY) ||
        (rollover_override_ != rollover::N_KEY))
    {
        auto buf_idx = keys_6kro_.active_side();
        auto& keys_6kro = keys_6kro_[buf_idx];
        // fill up the report
        keys_6kro.modifiers = data.modifiers;
        keys_6kro.scancodes.reset();
        for (auto code = LOWEST_SCANCODE; code <= HIGHEST_SCANCODE;
             code = static_cast<decltype(code)>(static_cast<uint8_t>(code) + 1))
        {
            keys_6kro.scancodes.set(code, data.test(code));
        }

        send_6kro_buffer(buf_idx);
    }
    else
    {
        auto buf_idx = keys_nkro_.active_side();
        auto& keys_nkro = keys_nkro_[buf_idx];
        // fill up the report
        keys_nkro.modifiers = data.modifiers;
        keys_nkro.scancodes = data.scancodes;

        send_nkro_buffer(buf_idx);
    }
}

void keyboard_app::send_6kro_buffer(uint8_t buf_idx)
{
    if (!keys_6kro_.differs() || !has_transport())
    {
        return;
    }
    auto result = hid::result::INVALID;
    auto& report = keys_6kro_[buf_idx];
    if (prot_ == hid::protocol::BOOT)
    {
        // sending 6KRO without report ID
        result = send_report(std::span<const uint8_t>(report.data() + sizeof(report.id),
                                                      sizeof(report) - sizeof(report.id)));
    }
    else
    {
        // sending 6KRO with report ID
        result = send_report(&report);
    }

    // swap sides only if the callback hasn't done yet
    if (result == hid::result::OK)
    {
        keys_6kro_.compare_swap_copy(buf_idx);
    }
}

void keyboard_app::send_nkro_buffer(uint8_t buf_idx)
{
    if (!keys_nkro_.differs() || !has_transport())
    {
        return;
    }
    auto result = send_report(&keys_nkro_[buf_idx]);
    if (result == hid::result::NO_MEMORY)
    {
        printk("keyboard NKRO mode fails, falling back to 6KRO\n");
        keys_6kro_.reset();
        rollover_override_ = rollover::SIX_KEY;
        keys_nkro_report_base<> data{.modifiers = keys_nkro_[buf_idx].modifiers,
                                     .scancodes = keys_nkro_[buf_idx].scancodes};
        set_report_state(data);
        keys_nkro_.reset();
    }
    else if (result == hid::result::OK)
    {
        keys_nkro_.compare_swap_copy(buf_idx);
    }
}

void keyboard_app::set_report(hid::report::type type, const std::span<const uint8_t>& data)
{
    // only one report is receivable, the LEDs
    // offset it if report ID is not present due to BOOT protocol
    auto& leds = *reinterpret_cast<const uint8_t*>(data.data() + static_cast<size_t>(prot_));

    // TODO use LEDs bitfields
    printk("keyboard LED status: %x\n", leds);

    // always keep receiving new reports
    // if the report data is processed immediately, the same buffer can be used
    receive_report(&leds_buffer_);
}

void keyboard_app::in_report_sent(const std::span<const uint8_t>& data)
{
    if ((prot_ == hid::protocol::BOOT) || (rollover_ != rollover::N_KEY) ||
        (rollover_override_ != rollover::N_KEY))
    {
        auto buf_idx = keys_6kro_.indexof(data.data());
        if (!keys_6kro_.compare_swap_copy(buf_idx))
        {
            send_6kro_buffer(1 - buf_idx);
        }
    }
    else
    {
        auto buf_idx = keys_nkro_.indexof(data.data());
        if (!keys_nkro_.compare_swap_copy(buf_idx))
        {
            send_nkro_buffer(1 - buf_idx);
        }
    }
}

void keyboard_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // TODO: report data accessing mutex?

    // copy to buffer to avoid overwriting data in transit
    switch (select.id())
    {
    case KEYS_6KRO_REPORT_ID:
    {
        auto& report = keys_6kro_[keys_6kro_.inactive_side()];
        assert(buffer.size() >= sizeof(report));
        memcpy(buffer.data(), report.data(), sizeof(report));
        send_report(buffer.subspan(0, sizeof(report)));
        break;
    }
    case KEYS_NKRO_REPORT_ID:
    {
        auto& report = keys_nkro_[keys_nkro_.inactive_side()];
        assert(buffer.size() >= sizeof(report));
        memcpy(buffer.data(), report.data(), sizeof(report));
        send_report(buffer.subspan(0, sizeof(report)));
        break;
    }
    default:
    {
        auto& report = keys_6kro_[keys_6kro_.inactive_side()];
        assert(buffer.size() >= (sizeof(report) - sizeof(report.id)));
        memcpy(buffer.data(), report.data() + sizeof(report.id),
               sizeof(report) - sizeof(report.id));
        send_report(buffer.subspan(0, sizeof(report) - sizeof(report.id)));
        break;
    }
    }
}
