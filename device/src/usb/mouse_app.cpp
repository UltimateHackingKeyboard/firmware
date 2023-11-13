#include "mouse_app.hpp"
#include "hid/report_protocol.hpp"
#include "hid/page/consumer.hpp"

const hid::report_protocol& mouse_app::report_protocol()
{
    using namespace hid::page;
    using namespace hid::rdf;

    static constexpr auto rd = descriptor(
        usage_page<generic_desktop>(),
        usage(generic_desktop::MOUSE),
        collection::application(
            usage(generic_desktop::POINTER),
            collection::physical(
                // buttons
                usage_extended_limits(button(1), LAST_BUTTON),
                logical_limits<1, 1>(0, 1),
                report_count(static_cast<uint8_t>(LAST_BUTTON)),
                report_size(1),
                input::absolute_variable(),
                input::byte_padding<static_cast<uint8_t>(LAST_BUTTON)>(),

                // relative X,Y directions
                usage(generic_desktop::X),
                usage(generic_desktop::Y),
                logical_limits<2, 2>(-AXIS_LIMIT, AXIS_LIMIT),
                report_count(2),
                report_size(16),
                input::relative_variable(),

                // vertical wheel
                collection::logical(
                    usage(generic_desktop::WHEEL),
                    logical_limits<1, 1>(-WHEEL_LIMIT, WHEEL_LIMIT),
                    report_count(1),
                    report_size(8),
                    input::relative_variable()
                ),
                // horizontal wheel
                collection::logical(
                    usage_extended(consumer::AC_PAN),
                    logical_limits<1, 1>(-WHEEL_LIMIT, WHEEL_LIMIT),
                    report_count(1),
                    report_size(8),
                    input::relative_variable()
                )
            )
        )
    );
    static constexpr hid::report_protocol rp {rd};
    return rp;
}

mouse_app& mouse_app::handle()
{
    static mouse_app app{};
    return app;
}

void mouse_app::start(hid::protocol prot)
{
    // TODO start handling mouse events
    report_data_ = {};
    tx_busy_ = false;
}

void mouse_app::stop()
{
    // TODO stop handling mouse events
}

hid::result mouse_app::send(const mouse_report_base<>& data)
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

void mouse_app::in_report_sent(const std::span<const uint8_t>& data)
{
    tx_busy_ = false;
    // the next IN report can be sent if any are queued
}

void mouse_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    assert(buffer.size() >= sizeof(report_data_));
    memcpy(buffer.data(), report_data_.data(), sizeof(report_data_));
    send_report(buffer.subspan(0, sizeof(report_data_)));
}
