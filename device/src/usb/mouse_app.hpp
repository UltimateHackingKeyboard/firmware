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

template <int LIMIT>
using limit_fitted_int =
    std::conditional_t<(LIMIT > std::numeric_limits<int8_t>::max()), hid::le_int16_t, int8_t>;

class mouse_app : public app_base {
    static constexpr auto LAST_BUTTON = hid::page::button(20);
    static constexpr int16_t AXIS_LIMIT = 4096;
    static constexpr int16_t MAX_SCROLL_RESOLUTION = 120;
    static constexpr int16_t WHEEL_LIMIT = 32767;

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
                    logical_limits<(AXIS_LIMIT > std::numeric_limits<int8_t>::max() ? 2 : 1)>(-AXIS_LIMIT, AXIS_LIMIT),
                    report_count(2),
                    report_size(AXIS_LIMIT > std::numeric_limits<int8_t>::max() ? 16 : 8),
                    input::relative_variable(),

                    hid::app::mouse::high_resolution_scrolling<WHEEL_LIMIT, MAX_SCROLL_RESOLUTION>()
                )
            )
        );
        // clang-format on
    }

    template <uint8_t REPORT_ID = 0>
    struct mouse_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID> {
        hid::report_bitset<hid::page::button, hid::page::button(1), mouse_app::LAST_BUTTON>
            buttons{};
        limit_fitted_int<AXIS_LIMIT> x{};
        limit_fitted_int<AXIS_LIMIT> y{};
        limit_fitted_int<WHEEL_LIMIT> wheel_y{};
        limit_fitted_int<WHEEL_LIMIT> wheel_x{};

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
    void set_report(hid::report::type type, const std::span<const uint8_t> &data) override;
    void get_report(hid::report::selector select, const std::span<uint8_t> &buffer) override;

    using mouse_report = mouse_report_base<report_ids::IN_MOUSE>;
    C2USB_USB_TRANSFER_ALIGN(mouse_report, report_buffer_) {};
    using scroll_resolution_report =
        hid::app::mouse::resolution_multiplier_report<MAX_SCROLL_RESOLUTION,
            report_ids::FEATURE_MOUSE>;
    C2USB_USB_TRANSFER_ALIGN(scroll_resolution_report, resolution_buffer_) {};

  public:
    const auto &resolution_report() const { return resolution_buffer_; }
};

using mouse_buffer = mouse_app::mouse_report_base<>;

#endif // __KEYBOARD_APP_HEADER__
