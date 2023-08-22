#ifndef __MOUSE_APP_HEADER__
#define __MOUSE_APP_HEADER__

#include "hid/app/mouse.hpp"
#include "hid/application.hpp"

class mouse_app : public hid::application
{
    static const hid::report_protocol& report_protocol();

    static constexpr auto LAST_BUTTON = hid::page::button(8);
    static constexpr int16_t AXIS_LIMIT = 4096;
    static constexpr int8_t WHEEL_LIMIT = 127;

    mouse_app()
        : application(report_protocol())
    {}

public:
    template<uint8_t REPORT_ID>
    struct mouse_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID>
    {
        uint8_t buttons {};
        static_assert(static_cast<uint8_t>(mouse_app::LAST_BUTTON) <= 8);
        hid::le_int16_t x {};
        hid::le_int16_t y {};
        int8_t wheel_y {};
        int8_t wheel_x {};

        constexpr mouse_report_base() = default;

        // TODO: add helpers
    };
    using mouse_report = mouse_report_base<0>;

    static mouse_app& handle();

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

#endif // __KEYBOARD_APP_HEADER__
