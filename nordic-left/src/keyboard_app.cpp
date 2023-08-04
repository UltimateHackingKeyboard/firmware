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

void keyboard_app::start(hid::protocol prot)
{
    prot_ = prot;

    // start receiving reports
    receive_report(&leds_buffer_);

    // TODO start handling keyboard events
}

void keyboard_app::stop()
{
    // TODO stop handling keyboard events
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
    // the next IN report can be sent if any are queued
}

void keyboard_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // TODO fetch the keys report data in the protocol specific format
}
