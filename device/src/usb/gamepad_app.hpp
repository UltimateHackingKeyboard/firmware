#ifndef __GAMEPAD_APP_HEADER__
#define __GAMEPAD_APP_HEADER__

#include "double_buffer.hpp"
#include "hid/application.hpp"
#include "hid/page/button.hpp"
#include "hid/page/generic_desktop.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"

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
    gamepad_app()
        : application(report_protocol())
    {}

  public:
    static constexpr auto report_desc()
    {
        using namespace hid::page;
        using namespace hid::rdf;

        // clang-format off
        // translate XBOX360 report mapping to HID as best as possible
        return descriptor(
            usage_page<generic_desktop>(),
            usage(generic_desktop::GAMEPAD),
            collection::application(
                // Report ID (0, which isn't valid, mark it as reserved)
                // Report size
                input::padding(16),

                // Buttons p1
                usage_page<button>(),
                logical_limits<1, 1>(0, 1),
                report_size(1),
                usage(button(13)),
                usage(button(14)),
                usage(button(15)),
                usage(button(16)),
                usage(button(10)),
                usage(button(9)),
                usage(button(11)),
                usage(button(12)),
                usage(button(5)),
                usage(button(6)),
                usage(button(17)),
                report_count(11),
                input::absolute_variable(),

                // 1 bit gap
                input::padding(1),

                // Buttons p2
                usage_limits(button(1), button(4)),
                report_count(4),
                input::absolute_variable(),

                // Triggers
                usage_page<generic_desktop>(),
                logical_limits<1, 2>(0, 255),
                report_size(8),
                report_count(1),

                // * Left analog trigger
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
                usage_extended(button(7)),
                collection::physical(
#endif
                    usage(generic_desktop::Z),
                    input::absolute_variable()
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
                )
#endif
                ,

                // * Right analog trigger
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
                usage_extended(button(8)),
                collection::physical(
#endif
                    usage(generic_desktop::RZ),
                    input::absolute_variable()
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
                )
#endif
                ,

                // Sticks
                logical_limits<2, 2>(-32767, 32767),
                report_size(16),
                report_count(2),

                // * Left stick
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
                usage_extended(button(11)),
#endif
                collection::physical(
                    usage(generic_desktop::X),
                    usage(generic_desktop::Y),
                    input::absolute_variable()
                ),

                // * Right stick
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
                usage_extended(button(12)),
#endif
                collection::physical(
                    usage(generic_desktop::RX),
                    usage(generic_desktop::RY),
                    input::absolute_variable()
                )
            )
        );
        // clang-format on
    }

    struct joystick
    {
        hid::le_int16_t X{};
        hid::le_int16_t Y{};
        constexpr joystick() = default;
        bool operator==(const joystick& other) const = default;
    };
    struct gamepad_report : public hid::report::base<hid::report::type::INPUT, 0>
    {
      private:
        uint8_t report_id{0};
        uint8_t report_size{sizeof(gamepad_report)};

      public:
        hid::le_uint16_t buttons{};
        uint8_t left_trigger{};
        uint8_t right_trigger{};
        joystick left;
        joystick right;

        constexpr gamepad_report() = default;

        bool operator==(const gamepad_report& other) const = default;
        bool operator!=(const gamepad_report& other) const = default;

        void set_button(gamepad_button b, bool value = true)
        {
            if (value)
            {
                buttons = buttons | (1 << static_cast<uint16_t>(b));
            }
            else
            {
                buttons = buttons & ~(1 << static_cast<uint16_t>(b));
            }
        }
        bool test_button(gamepad_button b) const
        {
            return buttons & (1 << static_cast<uint16_t>(b));
        }
    };

    static gamepad_app& handle();

    void set_report_state(const gamepad_report& data);

  private:
    static const hid::report_protocol& report_protocol()
    {
        static constexpr const auto rd{report_desc()};
        static constexpr const hid::report_protocol rp{rd};
        return rp;
    }

    void start(hid::protocol prot) override;
    void stop() override;
    void set_report(hid::report::type type, const std::span<const uint8_t>& data) override
    {
        // no FEATURE or OUTPUT reports
    }
    void in_report_sent(const std::span<const uint8_t>& data) override;
    void get_report(hid::report::selector select, const std::span<uint8_t>& buffer) override;
    void send_buffer(uint8_t buf_idx);

    double_buffer<gamepad_report> report_buffer_{};
};

using gamepad_buffer = gamepad_app::gamepad_report;

#endif // __GAMEPAD_APP_HEADER__
