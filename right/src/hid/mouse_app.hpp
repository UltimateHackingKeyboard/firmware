#pragma once

extern "C" {
#include "hid/mouse_report.h"
#include "hid/report_ids.h"
#include "hid/transport.h"
}
#include "double_buffer.hpp"
#include <hid/app/mouse.hpp>
#include <hid/application.hpp>
#include <hid/page/consumer.hpp>

class mouse_session : public hid::session {
  public:
    static constexpr int16_t MAX_SCROLL_RESOLUTION = 120;
    using scroll_resolution_report =
        hid::app::mouse::resolution_multiplier_report<MAX_SCROLL_RESOLUTION,
            report_ids::FEATURE_MOUSE>;

    mouse_session(const hid::session::params &p) : hid::session(p)
    {
        receive_report(&resolution_buffer_);
    }

    const auto &resolution_report() const { return resolution_buffer_; }

  protected:
    C2USB_USB_TRANSFER_ALIGN(scroll_resolution_report, resolution_buffer_) {};

    void report_sent(const std::span<const uint8_t> &data) override;

    std::span<const uint8_t> get_report(
        hid::report::selector select, const std::span<uint8_t> &buffer) override;

    void set_report(hid::report::type type, const std::span<const uint8_t> &data) override;
};

class mouse_app : public hid::application {
    static constexpr auto LAST_BUTTON = hid::page::button(HID_MOUSE_BUTTON_COUNT);
    static constexpr int16_t AXIS_LIMIT = 4096;
    static constexpr int16_t WHEEL_LIMIT = 32767;

  public:
    static constexpr int16_t MAX_SCROLL_RESOLUTION = mouse_session::MAX_SCROLL_RESOLUTION;

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
                    usage_page<button>(),
                    usage_limits(button(1), LAST_BUTTON),
                    logical_limits<1, 1>(0, 1),
                    report_count(static_cast<uint8_t>(LAST_BUTTON)),
                    report_size(1),
                    input::absolute_variable(),
                    input::byte_padding<static_cast<uint8_t>(LAST_BUTTON)>(),

                    // relative X,Y directions
                    usage_page<generic_desktop>(),
                    usage(generic_desktop::X),
                    usage(generic_desktop::Y),
                    logical_limits<byte_width(AXIS_LIMIT)>(-AXIS_LIMIT, AXIS_LIMIT),
                    report_count(2),
                    report_size(byte_width(AXIS_LIMIT) * 8),
                    input::relative_variable(),

                    hid::app::mouse::high_resolution_scrolling<WHEEL_LIMIT, MAX_SCROLL_RESOLUTION>()
                )
            )
        );
        // clang-format on
    }

    template <uint8_t REPORT_ID = 0>
    struct mouse_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID> {
        hid_mouse_report_t data{};

        bool operator==(const mouse_report_base &other) const = default;
        bool operator!=(const mouse_report_base &other) const = default;
    };

    static mouse_app &usb_handle()
    {
        static mouse_app app{};
        return app;
    }

    mouse_session *session() { return session_.has_value() ? &*session_ : nullptr; }

  private:
    mouse_app() : application(hid::report_protocol::from_descriptor<report_desc()>()) {}

    hid::session &start(const hid::session::params &params) override;
    void stop(hid::session &sess) override;

    std::optional<mouse_session> session_{};
};

void mouse_report_sent_callback(hid::session &session);
void mouse_resolution_changed_callback(
    hid::session &session, const mouse_session::scroll_resolution_report &report);
