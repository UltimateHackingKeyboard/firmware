#include "keyboard_app.hpp"
#include "hid/report_protocol.hpp"
#include <zephyr/sys/printk.h>

const hid::report_protocol& keyboard_app::report_protocol()
{
    using namespace hid::page;
    using namespace hid::rdf;
    using namespace hid::app::keyboard;

    // clang-format off
    static constexpr auto rd = descriptor(
        // 6KRO keyboard with report ID
        usage_extended(generic_desktop::KEYBOARD),
        collection::application(
            // input keys report
            keys_input_report_descriptor<KEYS_6KRO_REPORT_ID>(),

            // LED report
            leds_output_report_descriptor<LEDS_REPORT_ID>()
        ),
        // NKRO keyboard with report ID, no LEDs
        usage_extended(generic_desktop::KEYBOARD),
        collection::application(

            conditional_report_id<KEYS_NKRO_REPORT_ID>(),
            // modifier byte can stay in position
            report_size(1),
            report_count(8),
            logical_limits<1, 1>(0, 1),
            usage_page<keyboard_keypad>(),
            usage_limits(keyboard_keypad::LEFTCTRL, keyboard_keypad::RIGHTGUI),
            input::absolute_variable(),

            // scancode bitfield
            usage_limits(NKRO_FIRST_USAGE, NKRO_LAST_USAGE),
            // report_size(1),
            // logical_limits<1, 1>(0, 1),
            report_count(NKRO_USAGE_COUNT),
            input::absolute_variable(),
            input::byte_padding<NKRO_USAGE_COUNT>()
        )
    );
    // clang-format on
    static constexpr hid::report_protocol rp{rd};
    return rp;
}

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
    if (rollover_ == rollover::N_KEY)
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

    if ((prot_ == hid::protocol::BOOT) || (rollover_ == rollover::SIX_KEY))
    {
        auto buf_idx = keys_6kro_.active_side();
        auto& keys_6kro = keys_6kro_[buf_idx];
        // fill up the report
        keys_6kro.modifiers = data.modifiers;
        keys_6kro.scancodes.fill(0);
        size_t i = 0;
        for (size_t code = 0; code < NKRO_USAGE_COUNT; code++)
        {
            if (data.scancode_flags[code / 8] & (1 << (code % 8)))
            {
                if (i < 6)
                {
                    keys_6kro.scancodes[i] = code + LOWEST_SCANCODE;
                    i++;
                }
                else
                {
                    // raise rollover error
                    for (auto& b : keys_6kro.scancodes)
                    {
                        b = static_cast<uint8_t>(scancode::ERRORROLLOVER);
                    }
                    break;
                }
            }
        }

        send_6kro_buffer(buf_idx);
    }
    else
    {
        auto buf_idx = keys_nkro_.active_side();
        auto& keys_nkro = keys_nkro_[buf_idx];
        // fill up the report
        keys_nkro.modifiers = data.modifiers;
        keys_nkro.scancode_flags = data.scancode_flags;

        send_nkro_buffer(buf_idx);
    }
}

void keyboard_app::send_6kro_buffer(uint8_t buf_idx)
{
    if (!keys_6kro_.differs())
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
    if (!keys_nkro_.differs())
    {
        return;
    }
    if (send_report(&keys_nkro_[buf_idx]) == hid::result::OK)
    {
        keys_nkro_.compare_swap_copy(buf_idx);
    }
}

void keyboard_app::set_report(hid::report::type type, const std::span<const uint8_t>& data)
{
    // only one report is receivable, the LEDs
    // offset it if report ID is not present due to BOOT protocol
    auto& leds = *reinterpret_cast<const decltype(leds_buffer_.leds)*>(
        data.data() + static_cast<size_t>(prot_));

    // TODO use LEDs bitfields
    printk("keyboard LED status: %x\n", leds);

    // always keep receiving new reports
    // if the report data is processed immediately, the same buffer can be used
    receive_report(&leds_buffer_);
}

void keyboard_app::in_report_sent(const std::span<const uint8_t>& data)
{
    if ((prot_ == hid::protocol::BOOT) || (rollover_ == rollover::SIX_KEY))
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
