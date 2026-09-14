#pragma once

extern "C" {
#include "hid/keyboard_report.h"
#include "hid/report_ids.h"
#include "hid/transport.h"
}
#include "double_buffer.hpp"
#include <hid/app/keyboard.hpp>
#include <hid/app/lamparray.hpp>
#include <hid/application.hpp>
#include <hid/page/consumer.hpp>
#include <usb/df/class/hid.hpp>

using scancode = hid::page::keyboard_keypad;

template <std::uint8_t REPORT_ID = 0>
struct keyboard_attributes_report
    : public hid::report::base<hid::report::type::FEATURE, REPORT_ID> {
    hid::app::keyboard::form_factor form_factor{};
    hid::app::keyboard::key_type key_type{};
    hid::app::keyboard::layout layout{};
    usb::istring ietf_lang_tag_index{};

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;

        // clang-format off
        return hid::rdf::descriptor(
            usage_page<consumer>(),
            collection::logical(
                conditional_report_id<REPORT_ID>(),
                report_size(8),
                report_count(4),
                usage(consumer::KEYBOARD_FORM_FACTOR),
                usage(consumer::KEYBOARD_KEY_TYPE),
                usage(consumer::KEYBOARD_PHYSICAL_LAYOUT),
                usage(consumer::KEYBOARD_IETF_LANGUAGE_TAG_INDEX),
                logical_limits<1, 2>(0, std::numeric_limits<std::uint8_t>::max()),
                feature::absolute_constant()
            )
        );
        // clang-format on
    }
};

class keyboard_base_session : public hid::session {
  public:
    using hid::session::session;
    using leds_boot_report = hid::app::keyboard::output_report<0>;
    using leds_report = hid::app::keyboard::output_report<report_ids::OUT_KEYBOARD_LEDS>;
    using attributes_report = keyboard_attributes_report<report_ids::FEATURE_KEYBOARD_ATTRIBUTES>;

    virtual leds_boot_report get_leds_report() const = 0;

#if DEVICE_IS_UHK60
    static constexpr uint8_t MULTI_UPDATE_LIMIT = 10;

    static constexpr uint8_t LEFT_MAX_LAMP_ID = 33;
    static constexpr uint8_t LEFT_LAMP_COUNT = LEFT_MAX_LAMP_ID + 1;

    static constexpr uint8_t LEFT_HALF_LAMP_COUNT = LEFT_LAMP_COUNT - 3;

    static constexpr uint8_t RIGHT_MAX_LAMP_ID = 32;
    static constexpr uint8_t RIGHT_LAMP_COUNT = RIGHT_MAX_LAMP_ID + 1;

    using left_lamp_attrs_report =
        hid::app::lamparray::lamp_array_attributes_report<report_ids::FEATURE_LEFT_LAMP_ATTRS>;
    using left_lamp_attrs_req_report = hid::app::lamparray::lamp_attributes_request_report<
        report_ids::FEATURE_LEFT_LAMP_ATTRS_REQ>;
    using left_lamp_attrs_rsp_report = hid::app::lamparray::lamp_attributes_response_report<
        report_ids::FEATURE_LEFT_LAMP_ATTRS_RSP>;
    using left_lamp_multi_update_report =
        hid::app::lamparray::lamp_multi_update_report<report_ids::FEATURE_LEFT_LAMP_MULTI_UPDATE,
            MULTI_UPDATE_LIMIT>;
    using left_lamp_range_update_report =
        hid::app::lamparray::lamp_range_update_report<report_ids::FEATURE_LEFT_LAMP_RANGE_UPDATE>;
    using left_lamp_control_report =
        hid::app::lamparray::control_report<report_ids::FEATURE_LEFT_LAMP_CONTROL>;

    static constexpr auto left_lamp_descriptor()
    {
        using namespace hid::rdf;
        return descriptor(
            // clang-format off
            usage_page<hid::page::lighting_and_illumination>(),
            usage(hid::page::lighting_and_illumination::LAMP_ARRAY),
            collection::application(
                left_lamp_attrs_report::descriptor(),
                left_lamp_attrs_req_report::descriptor(),
                left_lamp_attrs_rsp_report::descriptor(),
                left_lamp_multi_update_report::descriptor(),
                left_lamp_range_update_report::descriptor(),
                left_lamp_control_report::descriptor()
            )
            // clang-format on
        );
    }
    using right_lamp_attrs_report =
        hid::app::lamparray::lamp_array_attributes_report<report_ids::FEATURE_RIGHT_LAMP_ATTRS>;
    using right_lamp_attrs_req_report = hid::app::lamparray::lamp_attributes_request_report<
        report_ids::FEATURE_RIGHT_LAMP_ATTRS_REQ>;
    using right_lamp_attrs_rsp_report = hid::app::lamparray::lamp_attributes_response_report<
        report_ids::FEATURE_RIGHT_LAMP_ATTRS_RSP>;
    using right_lamp_multi_update_report =
        hid::app::lamparray::lamp_multi_update_report<report_ids::FEATURE_RIGHT_LAMP_MULTI_UPDATE,
            MULTI_UPDATE_LIMIT>;
    using right_lamp_range_update_report =
        hid::app::lamparray::lamp_range_update_report<report_ids::FEATURE_RIGHT_LAMP_RANGE_UPDATE>;
    using right_lamp_control_report =
        hid::app::lamparray::control_report<report_ids::FEATURE_RIGHT_LAMP_CONTROL>;

    static constexpr auto right_lamp_descriptor()
    {
        using namespace hid::rdf;
        return descriptor(
            // clang-format off
            usage_page<hid::page::lighting_and_illumination>(),
            usage(hid::page::lighting_and_illumination::LAMP_ARRAY),
            collection::application(
                right_lamp_attrs_report::descriptor(),
                right_lamp_attrs_req_report::descriptor(),
                right_lamp_attrs_rsp_report::descriptor(),
                right_lamp_multi_update_report::descriptor(),
                right_lamp_range_update_report::descriptor(),
                right_lamp_control_report::descriptor()
            )
            // clang-format on
        );
    }
#endif
};

class keyboard_session : public keyboard_base_session {
    C2USB_USB_TRANSFER_ALIGN(leds_report, leds_buffer_) {};

#if DEVICE_IS_UHK60
    uint8_t req_led_left_{};
    uint8_t req_led_right_{};
#endif

  protected:
    void report_sent(const std::span<const uint8_t> &data) override;
    std::span<const uint8_t> get_report(
        hid::report::selector select, const std::span<uint8_t> &buffer) override;
    void set_report(hid::report::type type, const std::span<const uint8_t> &data) override;

  public:
    keyboard_session(const hid::session::params &p) : keyboard_base_session(p)
    {
        receive_report(&leds_buffer_);
    }

    leds_boot_report get_leds_report() const override;
};

class keyboard_app : public hid::application {
    static constexpr uint8_t KEYS_6KRO_REPORT_ID = report_ids::IN_KEYBOARD_6KRO;
    static constexpr uint8_t KEYS_NKRO_REPORT_ID = report_ids::IN_KEYBOARD_NKRO;
    static constexpr uint8_t LEDS_REPORT_ID = report_ids::OUT_KEYBOARD_LEDS;

    static constexpr auto NKRO_FIRST_USAGE =
        (scancode)HID_KEYBOARD_MIN_BITFIELD_SCANCODE; // scancode::KEYBOARD_A;
    static constexpr auto NKRO_LAST_USAGE = (scancode)HID_KEYBOARD_MAX_BITFIELD_SCANCODE;

  public:
    static constexpr auto LOWEST_SCANCODE = NKRO_FIRST_USAGE;
    static constexpr auto HIGHEST_SCANCODE = NKRO_LAST_USAGE;
    static constexpr auto NKRO_USAGE_COUNT =
        1 + static_cast<size_t>(HIGHEST_SCANCODE) - static_cast<size_t>(LOWEST_SCANCODE);

    using keys_boot_report = hid::app::keyboard::keys_input_report<0>;
    using keys_6kro_report = hid::app::keyboard::keys_input_report<KEYS_6KRO_REPORT_ID>;

    // this includes both 6KRO and NKRO reports, used on BLE
    static constexpr auto report_desc()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        using namespace hid::app::keyboard;

        // clang-format off
        return descriptor(
            usage_page<generic_desktop>(),
            usage(generic_desktop::KEYBOARD),
            collection::application(
                // 6KRO input keys report
                keys_input_report<KEYS_6KRO_REPORT_ID>::descriptor(),

                // LED report
                output_report<LEDS_REPORT_ID>::descriptor(),

                // NKRO keys report with report ID
                conditional_report_id<KEYS_NKRO_REPORT_ID>(),
                // modifier byte can stay in position
                report_size(1),
                report_count(8),
                logical_limits<1, 1>(0, 1),
                usage_page<keyboard_keypad>(),
                usage_limits(keyboard_keypad::KEYBOARD_LEFT_CONTROL, keyboard_keypad::KEYBOARD_RIGHT_GUI),
                input::absolute_variable(),

                // scancode bitfield
                usage_limits(NKRO_FIRST_USAGE, NKRO_LAST_USAGE),
                // report_size(1),
                // logical_limits<1, 1>(0, 1),
                report_count(NKRO_USAGE_COUNT),
                input::absolute_variable(),
                input::byte_padding<NKRO_USAGE_COUNT>()
            )
        );
        // clang-format on
    }
    static constexpr auto nkro_report_desc()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        using namespace hid::app::keyboard;

        // clang-format off
        return descriptor(
            usage_page<generic_desktop>(),
            usage(generic_desktop::KEYBOARD),
            collection::application(
                // LED report
                output_report<LEDS_REPORT_ID>::descriptor(),

                // NKRO keys report with report ID
                conditional_report_id<KEYS_NKRO_REPORT_ID>(),
                // modifier byte can stay in position
                report_size(1),
                report_count(8),
                logical_limits<1, 1>(0, 1),
                usage_page<keyboard_keypad>(),
                usage_limits(keyboard_keypad::KEYBOARD_LEFT_CONTROL, keyboard_keypad::KEYBOARD_RIGHT_GUI),
                input::absolute_variable(),

                // scancode bitfield
                usage_limits(NKRO_FIRST_USAGE, NKRO_LAST_USAGE),
                // report_size(1),
                // logical_limits<1, 1>(0, 1),
                report_count(NKRO_USAGE_COUNT),
                input::absolute_variable(),
                input::byte_padding<NKRO_USAGE_COUNT>()
            )
#if DEVICE_IS_UHK60
            ,
            keyboard_session::left_lamp_descriptor(),
            keyboard_session::right_lamp_descriptor()
#endif
        );
        // clang-format on
    }
    static hid::report_protocol nkro_report_protocol()
    {
        return hid::report_protocol::from_descriptor<nkro_report_desc()>();
    }
    static hid::report_protocol default_report_protocol()
    {
        return hid::report_protocol::from_descriptor<
            hid::app::keyboard::app_report_descriptor<KEYS_6KRO_REPORT_ID>()>();
    }

    template <uint8_t REPORT_ID = 0>
    struct keys_nkro_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID> {
        hid::report_bitset_range<hid::page::keyboard_keypad::KEYBOARD_LEFT_CONTROL,
            hid::page::keyboard_keypad::KEYBOARD_RIGHT_GUI>
            modifiers;
        hid::report_bitset_range<NKRO_FIRST_USAGE, NKRO_LAST_USAGE> scancodes;
        void set_code(scancode code, bool value = true)
        {
            if (modifiers.set(code, value)) {
                return;
            }
            if (scancodes.set(code, value)) {
                return;
            }
            assert(false);
        }

        bool test(scancode code) const
        {
            if (modifiers.in_range(code)) {
                return modifiers.test(code);
            }
            if (scancodes.in_range(code)) {
                return scancodes.test(code);
            }
            assert(false);
            return false;
        }

        bool operator==(const keys_nkro_report_base &other) const = default;
        bool operator!=(const keys_nkro_report_base &other) const = default;
    };
    using keys_nkro_report = keys_nkro_report_base<KEYS_NKRO_REPORT_ID>;

    static keyboard_app &usb_handle()
    {
        static keyboard_app app{nkro_report_protocol()};
        return app;
    }
    void set_rollover(rollover_t mode);

    keyboard_session *session() { return session_.has_value() ? &*session_ : nullptr; }

    static usb::df::hid::string_function &usb_function() { return usb_handle().usb_function_; }

  private:
    std::optional<keyboard_session> session_{};
    usb::df::hid::string_function usb_function_;

    keyboard_app(const hid::report_protocol &rp);
    hid::session &start(const hid::session::params &params) override;
    void stop(hid::session &sess) override;
};

union keys_report_variants {
    keyboard_app::keys_nkro_report nkro{};
    keyboard_app::keys_boot_report boot;
    keyboard_app::keys_6kro_report sixkro;
};

struct key_report_buffer : double_buffer<keys_report_variants> {
    static constexpr auto HIGHEST_SCANCODE = keyboard_app::HIGHEST_SCANCODE;
    static constexpr auto LOWEST_SCANCODE = keyboard_app::LOWEST_SCANCODE;
    enum mode_t {
        MODE_BOOT,
        MODE_NKRO,
        MODE_6KRO,
    } mode{MODE_NKRO};
    key_report_buffer() = default;

    void reset_to(hid::protocol prot, rollover_t override);
    std::span<const uint8_t> insert(const hid_keyboard_report_t &report);
};

void keyboard_report_sent_callback(hid::session &session);
void keyboard_leds_changed_callback(keyboard_base_session &session);
