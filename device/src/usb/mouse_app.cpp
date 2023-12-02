#include "mouse_app.hpp"
#include "hid/page/consumer.hpp"
#include "hid/report_protocol.hpp"

const hid::report_protocol& mouse_app::report_protocol()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
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
    // clang-format on
    static constexpr hid::report_protocol rp{rd};
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
    report_buffer_.reset();
}

void mouse_app::stop()
{
    // TODO stop handling mouse events
}

void mouse_app::set_report_state(const mouse_report_base<>& data)
{
    auto buf_idx = report_buffer_.active_side();
    auto& report = report_buffer_[buf_idx];
    report = data;
    send_buffer(buf_idx);
}

void mouse_app::send_buffer(uint8_t buf_idx)
{
    if (!report_buffer_.differs())
    {
        return;
    }
    if (send_report(&report_buffer_[buf_idx]) == hid::result::OK)
    {
        report_buffer_.compare_swap_copy(buf_idx);
    }
}

void mouse_app::in_report_sent(const std::span<const uint8_t>& data)
{
    auto buf_idx = report_buffer_.indexof(data.data());
    if (!report_buffer_.compare_swap_copy(buf_idx))
    {
        send_buffer(1 - buf_idx);
    }
}

void mouse_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    auto& report = report_buffer_[report_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), report.data(), sizeof(report));
    send_report(buffer.subspan(0, sizeof(report)));
}
