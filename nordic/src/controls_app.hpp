#ifndef __CONTROLS_APP_HEADER__
#define __CONTROLS_APP_HEADER__

#include "hid/application.hpp"
#include "hid/page/consumer.hpp"
#include "hid/page/generic_desktop.hpp"
#include "hid/page/telephony.hpp"

class controls_app : public hid::application
{
    static const hid::report_protocol& report_protocol();

    static constexpr size_t CONSUMER_CODE_COUNT = 2;
    static constexpr size_t SYSTEM_CODE_COUNT = 2;
    static constexpr size_t TELEPHONY_CODE_COUNT = 2;

    controls_app()
        : application(report_protocol())
    {}

public:
    struct controls_report : public hid::report::base<hid::report::type::INPUT, 0>
    {
        std::array<hid::le_uint16_t, CONSUMER_CODE_COUNT> consumer_codes {};
        std::array<hid::le_uint16_t, SYSTEM_CODE_COUNT> system_codes {};
        std::array<hid::le_uint16_t, TELEPHONY_CODE_COUNT> telephony_codes {};

        constexpr controls_report() = default;

        // TODO: add helpers
    };

    static controls_app& handle();

    using hid::application::send_report;

    void start(hid::protocol prot) override;
    void stop() override;
    void set_report(hid::report::type type, const std::span<const uint8_t>& data) override
    {
        // no FEATURE or OUTPUT reports
    }
    void in_report_sent(const std::span<const uint8_t>& data) override;
    void get_report(hid::report::selector select, const std::span<uint8_t>& buffer) override;

private:
};

#endif // __CONTROLS_APP_HEADER__
