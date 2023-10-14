#include "keyboard_app.hpp"
#include "hid/report_protocol.hpp"

const hid::report_protocol& keyboard_app::report_protocol()
{
    using namespace hid::page;
    using namespace hid::rdf;
    using namespace hid::app::keyboard;

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
    static constexpr hid::report_protocol rp {rd};
    return rp;
}

keyboard_app& keyboard_app::handle()
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

    // TODO: make sure that no keys are pressed when this happens
    // or send an empty report on the virtual keyboard that is deactivated by this switch?
    if (rollover_ == rollover::N_KEY) {
        keys_nkro_ = {};
        keys_6kro_ = {};
    } else {
        keys_nkro_ = {};
        keys_6kro_ = {};
    }
}

void keyboard_app::start(hid::protocol prot)
{
    prot_ = prot;

    // start receiving reports
    receive_report(&leds_buffer_);

    // TODO start handling keyboard events
    keys_nkro_ = {};
    keys_6kro_ = {};
    tx_busy_ = false;
}

void keyboard_app::stop()
{
    // TODO stop handling keyboard events
}

hid::result keyboard_app::send(const keys_nkro_report_base<>& data)
{
    // TODO: report data accessing mutex?

    auto result = hid::result::BUSY;
    if (tx_busy_) {
        // protect data in transit
        return result;
    }
    if ((prot_ == hid::protocol::BOOT) || (rollover_ == rollover::SIX_KEY)) {
        // fill up the report
        keys_6kro_.modifiers = data.modifiers;
        keys_6kro_.scancodes.fill(0);
        size_t i = 0;
        for (size_t code = 0; code < NKRO_USAGE_COUNT; code++) {
            if (data.scancode_flags[code / 8] & (1 << (code % 8))) {
                if (i < 6) {
                    keys_6kro_.scancodes[i] = code + LOWEST_SCANCODE;
                    i++;
                } else {
                    // raise rollover error
                    for (auto &b : keys_6kro_.scancodes) {
                        b = static_cast<uint8_t>(scancode::ERRORROLLOVER);
                    }
                    break;
                }
            }
        }

        if (prot_ == hid::protocol::BOOT) {
            // sending 6KRO without report ID
            result = send_report(std::span<const uint8_t>(keys_6kro_.data() + sizeof(keys_6kro_.id),
                    sizeof(keys_6kro_) - sizeof(keys_6kro_.id)));
        } else {
            // sending 6KRO with report ID
            result = send_report(&keys_6kro_);
        }
    } else {
        // fill up the report
        keys_nkro_.modifiers = data.modifiers;
        keys_nkro_.scancode_flags = data.scancode_flags;

        // sending NKRO with report ID
        result = send_report(&keys_nkro_);
    }

    if (result == hid::result::OK) {
        tx_busy_ = true;
    }

    return result;
}

void keyboard_app::set_report(hid::report::type type, const std::span<const uint8_t>& data)
{
    // only one report is receivable, the LEDs
    // offset it if report ID is not present due to BOOT protocol
    auto &leds = *reinterpret_cast<const decltype(leds_buffer_.leds)*>(data.data() - (1 - static_cast<size_t>(prot_)));

    // TODO use LEDs bitfields

    // always keep receiving new reports
    // if the report data is processed immediately, the same buffer can be used
    receive_report(&leds_buffer_);
}

void keyboard_app::in_report_sent(const std::span<const uint8_t>& data)
{
    tx_busy_ = false;
    // the next IN report can be sent if any are queued
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
            memcpy(buffer.data(), keys_6kro_.data(), sizeof(keys_6kro_));
            send_report(buffer.subspan(0, sizeof(keys_6kro_)));
            break;
        }
        case KEYS_NKRO_REPORT_ID:
        {
            assert(buffer.size() >= sizeof(keys_nkro_));
            memcpy(buffer.data(), keys_nkro_.data(), sizeof(keys_nkro_));
            send_report(buffer.subspan(0, sizeof(keys_nkro_)));
            break;
        }
        default:
        {
            assert(buffer.size() >= sizeof(keys_6kro_));
            memcpy(buffer.data(), keys_6kro_.data() + sizeof(keys_6kro_.id),
                    sizeof(keys_6kro_) - sizeof(keys_6kro_.id));
            send_report(buffer.subspan(0, sizeof(keys_6kro_) - sizeof(keys_6kro_.id)));
            break;
        }
    }
}
