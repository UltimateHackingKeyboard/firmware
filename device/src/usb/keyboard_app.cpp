#include "keyboard_app.hpp"
#include "hid/report_protocol.hpp"

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
        auto& keys_6kro = keys_6kro_.left();
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

        send_6kro_report(keys_6kro);
    }
    else
    {
        auto& keys_nkro = keys_nkro_.left();
        // fill up the report
        keys_nkro.modifiers = data.modifiers;
        keys_nkro.scancode_flags = data.scancode_flags;

        // sending NKRO with report ID
        send_nkro_report(keys_nkro);
    }
}

void keyboard_app::send_6kro_report(const keys_6kro_report& report)
{
    if (!keys_6kro_.differs())
    {
        return;
    }
    auto result = hid::result::INVALID;
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
    if ((result == hid::result::OK) && (&report == &keys_6kro_.left()))
    {
        keys_6kro_.right() = report;
        keys_6kro_.swap_sides();
    }
}

void keyboard_app::send_nkro_report(const keys_nkro_report& report)
{
    if (!keys_nkro_.differs())
    {
        return;
    }
    auto result = send_report(&report);
    // swap sides only if the callback hasn't done yet
    if ((result == hid::result::OK) && (&report == &keys_nkro_.left()))
    {
        keys_nkro_.right() = report;
        keys_nkro_.swap_sides();
    }
}

void keyboard_app::set_report(hid::report::type type, const std::span<const uint8_t>& data)
{
    // only one report is receivable, the LEDs
    // offset it if report ID is not present due to BOOT protocol
    auto& leds = *reinterpret_cast<const decltype(leds_buffer_.leds)*>(
        data.data() - (1 - static_cast<size_t>(prot_)));

    // TODO use LEDs bitfields

    // always keep receiving new reports
    // if the report data is processed immediately, the same buffer can be used
    receive_report(&leds_buffer_);
}

void keyboard_app::in_report_sent(const std::span<const uint8_t>& data)
{
    auto dataptr = reinterpret_cast<std::uintptr_t>(data.data());
    if ((prot_ == hid::protocol::BOOT) || (rollover_ == rollover::SIX_KEY))
    {
        auto& keys_6kro = keys_6kro_.left();
        auto leftptr = reinterpret_cast<std::uintptr_t>(&keys_6kro);
        // if the sent was still on the left side, swap now
        if ((dataptr >= leftptr) && (dataptr <= (leftptr + sizeof(keys_6kro))))
        {
            keys_6kro_.right() = keys_6kro;
            keys_6kro_.swap_sides();
        }
        else
        {
            send_6kro_report(keys_6kro);
        }
    }
    else
    {
        auto& keys_nkro = keys_nkro_.left();
        auto leftptr = reinterpret_cast<std::uintptr_t>(&keys_nkro);
        // if the sent was still on the left side, swap now
        if ((dataptr >= leftptr) && (dataptr <= (leftptr + sizeof(keys_nkro))))
        {
            keys_nkro_.right() = keys_nkro;
            keys_nkro_.swap_sides();
        }
        else
        {
            send_nkro_report(keys_nkro);
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
        assert(buffer.size() >= sizeof(keys_6kro_));
        memcpy(buffer.data(), keys_6kro_.right().data(), sizeof(keys_6kro_));
        send_report(buffer.subspan(0, sizeof(keys_6kro_.right())));
        break;
    }
    case KEYS_NKRO_REPORT_ID:
    {
        assert(buffer.size() >= sizeof(keys_nkro_));
        memcpy(buffer.data(), keys_nkro_.right().data(), sizeof(keys_nkro_));
        send_report(buffer.subspan(0, sizeof(keys_nkro_.right())));
        break;
    }
    default:
    {
        assert(buffer.size() >= sizeof(keys_6kro_));
        memcpy(buffer.data(), keys_6kro_.right().data() + sizeof(keys_6kro_.right().id),
               sizeof(keys_6kro_.right()) - sizeof(keys_6kro_.right().id));
        send_report(buffer.subspan(0, sizeof(keys_6kro_.right()) - sizeof(keys_6kro_.right().id)));
        break;
    }
    }
}
