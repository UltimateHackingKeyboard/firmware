#include "controls_app.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"

const hid::report_protocol& controls_app::report_protocol()
{
    using namespace hid;
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    static constexpr auto rd = descriptor(
        usage_page<consumer>(),
        usage(consumer::CONSUMER_CONTROL),
        collection::application(
            report_size(sizeof(std::declval<controls_report>().consumer_codes[0]) * 8),
            report_count(CONSUMER_CODE_COUNT),
            logical_limits<1, 2>(0, info<consumer>::max_usage),
            usage_extended_limits(nullusage, static_cast<consumer>(info<consumer>::max_usage)),
            input::array(),

            report_size(sizeof(std::declval<controls_report>().system_codes[0]) * 8),
            report_count(SYSTEM_CODE_COUNT),
            logical_limits<1, 2>(0, info<generic_desktop>::max_usage),
            usage_extended_limits(nullusage, static_cast<generic_desktop>(info<generic_desktop>::max_usage)),
            input::array(),

            report_size(sizeof(std::declval<controls_report>().telephony_codes[0]) * 8),
            report_count(TELEPHONY_CODE_COUNT),
            logical_limits<1, 2>(0, info<telephony>::max_usage),
            usage_extended_limits(nullusage, static_cast<telephony>(info<consumer>::max_usage)),
            input::array()
        )
    );
    // clang-format on
    static constexpr hid::report_protocol rp{rd};
    return rp;
}

controls_app& controls_app::handle()
{
    static controls_app app{};
    return app;
}

void controls_app::start(hid::protocol prot)
{
    // TODO start handling controls events
    report_buffer_.reset();
}

void controls_app::stop()
{
    // TODO stop handling controls events
}

void controls_app::set_report_state(const controls_report& data)
{
    auto buf_idx = report_buffer_.active_side();
    auto& report = report_buffer_[buf_idx];
    report = data;
    send_buffer(buf_idx);
}

void controls_app::send_buffer(uint8_t buf_idx)
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

void controls_app::in_report_sent(const std::span<const uint8_t>& data)
{
    auto buf_idx = report_buffer_.indexof(data.data());
    if (!report_buffer_.compare_swap_copy(buf_idx))
    {
        send_buffer(1 - buf_idx);
    }
}

void controls_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    auto& report = report_buffer_[report_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), report.data(), sizeof(report));
    send_report(buffer.subspan(0, sizeof(report)));
}
