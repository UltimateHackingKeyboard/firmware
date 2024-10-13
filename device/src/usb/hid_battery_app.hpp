#ifndef __HID_BATTERY_APP_HEADER__
#define __HID_BATTERY_APP_HEADER__

#include "double_buffer.hpp"
#include "hid/application.hpp"
#include "hid/page/battery_system.hpp"
#include "hid/page/generic_desktop.hpp"
#include "hid/page/generic_device.hpp"
#include "hid/page/power.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"

class hid_battery_app : public hid::application {
    hid_battery_app() : application(report_protocol()) {}

  public:
    static constexpr auto report_desc()
    {
        using namespace hid::page;
        using namespace hid::rdf;

        // clang-format off
        return descriptor(
            usage_page<power>(),
            usage(power::BATTERY_SYSTEM),
            collection::application(
                usage(power::BATTERY),
                collection::physical(
                    usage_page<battery_system>(),
                    usage(battery_system::ABSOLUTE_STATE_OF_CHARGE),
                    logical_limits<1, 1>(0, 100),
                    report_size(7),
                    report_count(1),
                    input::absolute_variable(),
                    usage(battery_system::CHARGING),
                    logical_limits<1, 1>(0, 1),
                    report_size(1),
                    input::absolute_variable()
                )
            )
        );
        // clang-format off
    }

    struct report : public hid::report::base<hid::report::type::INPUT, 0>
    {
        uint8_t remaining_capacity : 7;
        bool charging : 1;
    };

    static hid_battery_app& handle();

    void send(uint8_t remaining_capacity, bool charging);

    void set_report(hid::report::type type, const std::span<const uint8_t> &data) override
    {
        // no FEATURE or OUTPUT reports
    }
    void get_report(hid::report::selector select, const std::span<uint8_t>& buffer) override;
    void in_report_sent(const std::span<const uint8_t>& data) override;

  private:
    void send_buffer(uint8_t buf_idx);
    static hid::report_protocol report_protocol()
    {
        static constexpr const auto rd{report_desc()};
        constexpr hid::report_protocol rp{rd};
        return rp;
    }

    double_buffer<report> report_buffer_{};
};

#endif // __HID_BATTERY_APP_HEADER__
