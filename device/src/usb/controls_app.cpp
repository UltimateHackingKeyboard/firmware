#include "controls_app.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"

const hid::report_protocol& controls_app::report_protocol()
{
    using namespace hid;
    using namespace hid::page;
    using namespace hid::rdf;

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
    static constexpr hid::report_protocol rp {rd};
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
    report_data_ = {};
    tx_busy_ = false;
}

void controls_app::stop()
{
    // TODO stop handling controls events
}

hid::result controls_app::send(const controls_report& data)
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

void controls_app::in_report_sent(const std::span<const uint8_t>& data)
{
    tx_busy_ = false;
    // the next IN report can be sent if any are queued
}

void controls_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    assert(buffer.size() >= sizeof(report_data_));
    memcpy(buffer.data(), report_data_.data(), sizeof(report_data_));
    send_report(buffer.subspan(0, sizeof(report_data_)));
}
