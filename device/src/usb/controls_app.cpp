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
    auto& report = report_buffer_.left();
    report = data;
    send_buffer(report);
}

void controls_app::send_buffer(const controls_report& report)
{
    if (!report_buffer_.differs())
    {
        return;
    }
    auto result = send_report(&report);
    // swap sides only if the callback hasn't done yet
    if ((result == hid::result::OK) && (&report == &report_buffer_.left()))
    {
        report_buffer_.right() = report;
        report_buffer_.swap_sides();
    }
}

void controls_app::in_report_sent(const std::span<const uint8_t>& data)
{
    auto dataptr = reinterpret_cast<std::uintptr_t>(data.data());
    auto& report = report_buffer_.left();
    auto leftptr = reinterpret_cast<std::uintptr_t>(&report);
    // if the sent was still on the left side, swap now
    if ((dataptr >= leftptr) && (dataptr <= (leftptr + sizeof(report))))
    {
        report_buffer_.right() = report;
        report_buffer_.swap_sides();
    }
    else
    {
        send_buffer(report);
    }
}

void controls_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    assert(buffer.size() >= sizeof(report_buffer_.right()));
    memcpy(buffer.data(), report_buffer_.right().data(), sizeof(report_buffer_.right()));
    send_report(buffer.subspan(0, sizeof(report_buffer_.right())));
}
