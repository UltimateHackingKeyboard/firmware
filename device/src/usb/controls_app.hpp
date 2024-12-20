#ifndef __CONTROLS_APP_HEADER__
#define __CONTROLS_APP_HEADER__

#include "app_base.hpp"
#include "hid/page/consumer.hpp"
#include "hid/page/generic_desktop.hpp"
#include "hid/page/telephony.hpp"
#include "hid/report_array.hpp"
#include "report_ids.h"

using system_code = hid::page::generic_desktop;
using consumer_code = hid::page::consumer;
using telephony_code = hid::page::telephony;

class controls_app : public app_base {
    static constexpr size_t CONSUMER_CODE_COUNT = 2;
    static constexpr size_t SYSTEM_CODE_COUNT = 2;
    static constexpr size_t TELEPHONY_CODE_COUNT = 2;

  public:
    template <hid::report::id::type REPORT_ID>
    struct controls_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID> {
        hid::report_array<consumer_code, CONSUMER_CODE_COUNT, hid::le_uint16_t> consumer_codes{};
        hid::report_array<system_code, SYSTEM_CODE_COUNT, hid::le_uint16_t> system_codes{};
        hid::report_array<telephony_code, TELEPHONY_CODE_COUNT, hid::le_uint16_t> telephony_codes{};

        constexpr controls_report_base() = default;

        bool operator==(const controls_report_base &other) const = default;
        bool operator!=(const controls_report_base &other) const = default;

        bool set_code(system_code c, bool value = true) { return system_codes.set(c, value); }
        bool test(system_code c) const { return system_codes.test(c); }
        bool set_code(consumer_code c, bool value = true) { return consumer_codes.set(c, value); }
        bool test(consumer_code c) const { return consumer_codes.test(c); }
        bool set_code(telephony_code c, bool value = true) { return telephony_codes.set(c, value); }
        bool test(telephony_code c) const { return telephony_codes.test(c); }
    };

    static constexpr auto report_desc()
    {
        using namespace hid;
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
                logical_limits<1, 2>(0, info<consumer>::max_usage_id),
                usage_extended_limits(nullusage, static_cast<consumer>(info<consumer>::max_usage_id)),
                input::array(),

                report_size(sizeof(hid::le_uint16_t) * 8),
                report_count(SYSTEM_CODE_COUNT),
                logical_limits<1, 2>(0, info<generic_desktop>::max_usage_id),
                usage_extended_limits(nullusage, static_cast<generic_desktop>(info<generic_desktop>::max_usage_id)),
                input::array(),

                report_size(sizeof(hid::le_uint16_t) * 8),
                report_count(TELEPHONY_CODE_COUNT),
                logical_limits<1, 2>(0, info<telephony>::max_usage_id),
                usage_extended_limits(nullusage, static_cast<telephony>(info<consumer>::max_usage_id)),
                input::array()
            )
        );
        // clang-format on
    }

    static controls_app &handle();

    void set_report_state(const controls_report_base<0> &data);

  private:
    controls_app() : app_base(this, report_buffer_) {}

    void start(hid::protocol prot) override;

    using controls_report = controls_report_base<report_ids::IN_CONTROLS>;
    C2USB_USB_TRANSFER_ALIGN(controls_report, report_buffer_){};
};

using controls_buffer = controls_app::controls_report_base<0>;

#endif // __CONTROLS_APP_HEADER__
