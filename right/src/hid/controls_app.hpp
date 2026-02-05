#ifndef __CONTROLS_APP_HEADER__
#define __CONTROLS_APP_HEADER__

extern "C" {
#include "hid/controls_report.h"
#include "hid/report_ids.h"
#include "hid/transport.h"
}
#include "double_buffer.hpp"
#include <hid/application.hpp>
#include <hid/page/consumer.hpp>
#include <hid/page/generic_desktop.hpp>
#include <hid/rdf/descriptor.hpp>
#include <hid/report_array.hpp>

using system_code = hid::page::generic_desktop;
using consumer_code = hid::page::consumer;

class controls_app : public hid::application {
    static constexpr size_t CONSUMER_CODE_COUNT = HID_CONTROLS_MAX_CONSUMER_KEYS;
    static constexpr size_t SYSTEM_CODE_COUNT = HID_CONTROLS_MAX_SYSTEM_KEYS;

  public:
    template <hid::report::id::type REPORT_ID>
    struct controls_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID> {

        bool operator==(const controls_report_base &other) const = default;
        bool operator!=(const controls_report_base &other) const = default;

#if 1
        hid_controls_report_t data{};
#else
        hid::report_array<consumer_code, CONSUMER_CODE_COUNT, hid::le_uint16_t> consumer_codes{};
        hid::report_array<system_code, SYSTEM_CODE_COUNT, uint8_t> system_codes{};

        bool set_code(system_code c, bool value = true) { return system_codes.set(c, value); }
        bool test(system_code c) const { return system_codes.test(c); }
        bool set_code(consumer_code c, bool value = true) { return consumer_codes.set(c, value); }
        bool test(consumer_code c) const { return consumer_codes.test(c); }
#endif
    };

    static constexpr auto report_desc()
    {
        using namespace hid::page;
        using namespace hid::rdf;

        // clang-format off
        return descriptor(
            usage_page<consumer>(),
            usage(consumer::CONSUMER_CONTROL),
            collection::application(
                conditional_report_id<report_ids::IN_CONTROLS>(),

                report_size(sizeof(hid::le_uint16_t) * 8),
                report_count(CONSUMER_CODE_COUNT),
                logical_limits<1, 2>(0, get_info<consumer>().max_usage_id),
                usage_extended_limits(nullusage, static_cast<consumer>(get_info<consumer>().max_usage_id)),
                input::array(),

                report_size(sizeof(uint8_t) * 8),
                report_count(SYSTEM_CODE_COUNT),
                logical_limits<1, 1>(0, get_info<generic_desktop>().max_usage_id),
                usage_extended_limits(nullusage, static_cast<generic_desktop>(get_info<generic_desktop>().max_usage_id)),
                input::array()
            )
        );
        // clang-format on
    }

    static controls_app &usb_handle();
#if DEVICE_IS_UHK80_RIGHT
    static controls_app &ble_handle();
#endif

    int send_report(const hid_controls_report_t &report);

  private:
    controls_app() : hid::application(hid::report_protocol::from_descriptor<report_desc()>()) {}

    void start(hid::protocol prot) override;
    void get_report(hid::report::selector select, const std::span<uint8_t> &buffer) override;
    void set_report(hid::report::type type, const std::span<const uint8_t> &data) override
    {
        // No OUTPUT or FEATURE reports are used
    }
    void in_report_sent(const std::span<const uint8_t> &data) override;

    using controls_report = controls_report_base<report_ids::IN_CONTROLS>;
    double_buffer<controls_report> report_buffer_{};
};

using controls_buffer = controls_app::controls_report_base<0>;

#endif // __CONTROLS_APP_HEADER__
