#ifndef __GAMEPAD_APP_HEADER__
#define __GAMEPAD_APP_HEADER__

#include "hid/application.hpp"

class gamepad_app : public hid::application
{
    static const hid::report_protocol& report_protocol();

    gamepad_app()
        : application(report_protocol())
    {}

public:
    struct joystick
    {
        hid::le_int16_t X {};
        hid::le_int16_t Y {};
    };
    struct gamepad_report : public hid::report::base<hid::report::type::INPUT, 0>
    {
    private:
        const uint8_t report_id {};
        const uint8_t report_size { sizeof(gamepad_report) };
    public:
        union
        {
            hid::le_uint16_t buttons {};
            struct
            {
                uint16_t UP : 1;
                uint16_t DOWN : 1;
                uint16_t LEFT : 1;
                uint16_t RIGHT : 1;
                uint16_t START : 1;
                uint16_t BACK : 1;
                uint16_t LS : 1;
                uint16_t RS : 1;
                uint16_t LB : 1;
                uint16_t RB : 1;
                uint16_t HOME : 1;
                uint16_t : 1;
                uint16_t A : 1;
                uint16_t B : 1;
                uint16_t X : 1;
                uint16_t Y : 1;
            };
        };
        uint8_t left_trigger {};
        uint8_t right_trigger {};
        joystick left;
        joystick right;

        constexpr gamepad_report() = default;

        // TODO: add helpers
    };

    static gamepad_app& handle();

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

#endif // __GAMEPAD_APP_HEADER__
