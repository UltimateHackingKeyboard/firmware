#ifndef __GAMEPAD_APP_HEADER__
#define __GAMEPAD_APP_HEADER__

#include "hid/application.hpp"

enum class gamepad_button
{
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3,
    START = 4,
    BACK = 5,
    LS = 6,
    RS = 7,
    LB = 8,
    RB = 9,
    HOME = 10,
    A = 12,
    B = 13,
    X = 14,
    Y = 15
};

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
        constexpr joystick() = default;
    };
    struct gamepad_report : public hid::report::base<hid::report::type::INPUT, 0>
    {
    private:
        uint8_t report_id {0};
        uint8_t report_size { sizeof(gamepad_report) };
    public:
        hid::le_uint16_t buttons {};
        uint8_t left_trigger {};
        uint8_t right_trigger {};
        joystick left;
        joystick right;

        constexpr gamepad_report() = default;

        void set_button(gamepad_button b, bool value = true)
        {
            if (value) {
                buttons = buttons | (1 << static_cast<uint16_t>(b));
            } else {
                buttons = buttons & ~(1 << static_cast<uint16_t>(b));
            }
        }
        bool test_button(gamepad_button b) const
        {
            return buttons & (1 << static_cast<uint16_t>(b));
        }
    };

    static gamepad_app& handle();

    hid::result send(const gamepad_report& data);

private:
    void start(hid::protocol prot) override;
    void stop() override;
    void set_report(hid::report::type type, const std::span<const uint8_t>& data) override
    {
        // no FEATURE or OUTPUT reports
    }
    void in_report_sent(const std::span<const uint8_t>& data) override;
    void get_report(hid::report::selector select, const std::span<uint8_t>& buffer) override;

    C2USB_USB_TRANSFER_ALIGN(gamepad_report, report_data_) {};
    bool tx_busy_ {};
};

using gamepad_buffer = gamepad_app::gamepad_report;

#endif // __GAMEPAD_APP_HEADER__
