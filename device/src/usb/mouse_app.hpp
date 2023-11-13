#ifndef __MOUSE_APP_HEADER__
#define __MOUSE_APP_HEADER__

#include "hid/app/mouse.hpp"
#include "hid/application.hpp"

enum class mouse_button
{
    LEFT = 0,
    RIGHT,
    MIDDLE,
    _4,
    _5,
    _6,
    _7,
    _8
};

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
    template<uint8_t REPORT_ID = 0>
    struct mouse_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID>
    {
        uint8_t buttons {};
        static_assert(static_cast<uint8_t>(mouse_app::LAST_BUTTON) <= 8);
        hid::le_int16_t x {};
        hid::le_int16_t y {};
        int8_t wheel_y {};
        int8_t wheel_x {};

        constexpr mouse_report_base() = default;

        bool operator==(const mouse_report_base& other) const = default;

        void set_button(mouse_button b, bool value = true)
        {
            if (value) {
                buttons |= 1 << static_cast<uint8_t>(b);
            } else {
                buttons &= ~(1 << static_cast<uint8_t>(b));
            }
        }
        bool test_button(mouse_button b) const
        {
            return buttons & (1 << static_cast<uint8_t>(b));
        }
        void set_button(uint8_t number, bool value = true)
        {
            assert((number > 0) and (number <= 8));
            if (value) {
                buttons |= 1 << (number - 1);
            } else {
                buttons &= ~(1 << (number - 1));
            }
        }
        bool test_button(uint8_t number) const
        {
            return buttons & (1 << (number - 1));
        }
    };

    static mouse_app& handle();

    hid::result send(const mouse_report_base<>& data);

private:
    void start(hid::protocol prot) override;
    void stop() override;
    void set_report(hid::report::type type, const std::span<const uint8_t>& data) override
    {
        // no FEATURE or OUTPUT reports
    }
    void in_report_sent(const std::span<const uint8_t>& data) override;
    void get_report(hid::report::selector select, const std::span<uint8_t>& buffer) override;

    C2USB_USB_TRANSFER_ALIGN(mouse_report_base<>, report_data_) {};
    bool tx_busy_ {};
};

using mouse_buffer = mouse_app::mouse_report_base<>;

#endif // __KEYBOARD_APP_HEADER__
