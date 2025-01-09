#ifndef __MOUSE_APP_HEADER__
#define __MOUSE_APP_HEADER__

#include "app_base.hpp"
#include "double_buffer.hpp"
#include "hid/app/mouse.hpp"
#include "hid/page/consumer.hpp"
#include "report_ids.h"

enum class mouse_button {
    LEFT = 0,
    RIGHT,
    MIDDLE,
    _4,
    _5,
    _6,
    _7,
    _8
};

class mouse_app : public app_base {
    static constexpr auto LAST_BUTTON = hid::page::button(20);
    static constexpr int16_t AXIS_LIMIT = 4096;
    static constexpr int8_t WHEEL_LIMIT = 127;

  public:
    static constexpr auto report_desc()
    {
        using namespace hid::page;
        using namespace hid::rdf;

        // clang-format off
        return descriptor(
            usage_page<generic_desktop>(),
            usage(generic_desktop::MOUSE),
            collection::application(
                conditional_report_id<report_ids::IN_MOUSE>(),
                usage(generic_desktop::POINTER),
                collection::physical(
                    // buttons
                    usage_extended_limits(button(1), LAST_BUTTON),
                    logical_limits<1, 1>(0, 1),
                    report_count(static_cast<uint8_t>(LAST_BUTTON)),
                    report_size(1),
                    input::absolute_variable(),
                    input::byte_padding<static_cast<uint8_t>(LAST_BUTTON)>(),

                    // relative X,Y directions
                    usage(generic_desktop::X),
                    usage(generic_desktop::Y),
                    logical_limits<2, 2>(-AXIS_LIMIT, AXIS_LIMIT),
                    report_count(2),
                    report_size(16),
                    input::relative_variable(),

                    // vertical wheel
                    collection::logical(
                        usage(generic_desktop::WHEEL),
                        logical_limits<1, 1>(-WHEEL_LIMIT, WHEEL_LIMIT),
                        report_count(1),
                        report_size(8),
                        input::relative_variable()
                    ),
                    // horizontal wheel
                    collection::logical(
                        usage_extended(consumer::AC_PAN),
                        logical_limits<1, 1>(-WHEEL_LIMIT, WHEEL_LIMIT),
                        report_count(1),
                        report_size(8),
                        input::relative_variable()
                    )
                )
            )
        );
        // clang-format on
    }

    template <uint8_t REPORT_ID = 0>
    struct mouse_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID> {
        hid::report_bitset<hid::page::button, hid::page::button(1), mouse_app::LAST_BUTTON>
            buttons{};
        hid::le_int16_t x{};
        hid::le_int16_t y{};
        int8_t wheel_y{};
        int8_t wheel_x{};

        constexpr mouse_report_base() = default;

        bool operator==(const mouse_report_base &other) const = default;
        bool operator!=(const mouse_report_base &other) const = default;
    };

    static mouse_app &usb_handle();
#if DEVICE_IS_UHK80_RIGHT
    static mouse_app &ble_handle();
#endif

    void set_report_state(const mouse_report_base<> &data);

  private:
    mouse_app() : app_base(this, report_buffer_) {}

    void start(hid::protocol prot) override;

    using mouse_report = mouse_report_base<report_ids::IN_MOUSE>;
    C2USB_USB_TRANSFER_ALIGN(mouse_report, report_buffer_) {};
};

using mouse_buffer = mouse_app::mouse_report_base<>;

#endif // __KEYBOARD_APP_HEADER__
