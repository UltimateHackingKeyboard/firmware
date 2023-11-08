#include "gamepad_app.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"
#include "hid/page/generic_desktop.hpp"
#include "hid/page/button.hpp"

const hid::report_protocol& gamepad_app::report_protocol()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // translate XBOX360 report mapping to HID as best as possible
    static constexpr auto rd = descriptor(
        usage_page<generic_desktop>(),
        usage(generic_desktop::GAME_PAD),
        collection::application(
            // Report ID (0, which isn't valid, mark it as reserved)
            // Report size
            input::padding(16),

            // Buttons p1
            usage_page<button>(),
            logical_limits<1, 1>(0, 1),
            report_size(1),
            usage(button(13)),
            usage(button(14)),
            usage(button(15)),
            usage(button(16)),
            usage(button(10)),
            usage(button(9)),
            usage(button(11)),
            usage(button(12)),
            usage(button(5)),
            usage(button(6)),
            usage(button(17)),
            report_count(11),
            input::absolute_variable(),

            // 1 bit gap
            input::padding(1),

            // Buttons p2
            usage_limits(button(1), button(4)),
            report_count(4),
            input::absolute_variable(),

            // Triggers
            usage_page<generic_desktop>(),
            logical_limits<1, 2>(0, 255),
            report_size(8),
            report_count(1),

            // * Left analog trigger
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            usage_extended(button(7)),
            collection::physical(
#endif
                usage(generic_desktop::Z),
                input::absolute_variable()
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            )
#endif
            ,

            // * Right analog trigger
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            usage_extended(button(8)),
            collection::physical(
#endif
                usage(generic_desktop::RZ),
                input::absolute_variable()
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            )
#endif
            ,

            // Sticks
            logical_limits<2, 2>(-32767, 32767),
            report_size(16),
            report_count(2),

            // * Left stick
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            usage_extended(button(11)),
#endif
            collection::physical(
                usage(generic_desktop::X),
                usage(generic_desktop::Y),
                input::absolute_variable()
            ),

            // * Right stick
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            usage_extended(button(12)),
#endif
            collection::physical(
                usage(generic_desktop::RX),
                usage(generic_desktop::RY),
                input::absolute_variable()
            )
        )
    );
    static constexpr hid::report_protocol rp {rd};
    return rp;
}

gamepad_app& gamepad_app::handle()
{
    static gamepad_app app{};
    return app;
}

void gamepad_app::start(hid::protocol prot)
{
    // TODO start handling gamepad events
    report_data_ = {};
    tx_busy_ = false;
}

void gamepad_app::stop()
{
    // TODO stop handling gamepad events
}

hid::result gamepad_app::send(const gamepad_report& data)
{
    auto result = hid::result::BUSY;
    if (tx_busy_) {
        // protect data in transit
        return result;
    }
    report_data_ = data;
    result = send_report(&report_data_);
    if (result == hid::result::OK) {
        tx_busy_ = true;
    }
    return result;
}

void gamepad_app::in_report_sent(const std::span<const uint8_t>& data)
{
    tx_busy_ = false;
    // the next IN report can be sent if any are queued
}

void gamepad_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    assert(buffer.size() >= sizeof(report_data_));
    memcpy(buffer.data(), report_data_.data(), sizeof(report_data_));
    send_report(buffer.subspan(0, sizeof(report_data_)));
}