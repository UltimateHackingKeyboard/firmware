#pragma once

extern "C" {
#include "hid/keyboard_report.h"
#include "hid/report_ids.h"
#include "hid/transport.h"
}
#include "double_buffer.hpp"
#include <hid/app/keyboard.hpp>
#include <hid/application.hpp>

using scancode = hid::page::keyboard_keypad;

class keyboard_base_session : public hid::session {
  public:
    using leds_boot_report = hid::app::keyboard::output_report<0>;
    using leds_report = hid::app::keyboard::output_report<report_ids::OUT_KEYBOARD_LEDS>;
    virtual leds_boot_report get_leds_report() const = 0;
};
class keyboard_session : public keyboard_base_session {
    C2USB_USB_TRANSFER_ALIGN(leds_report, leds_buffer_) {};

  protected:
    void report_sent(const std::span<const uint8_t> &data) override;
    std::span<const uint8_t> get_report(
        hid::report::selector select, const std::span<uint8_t> &buffer) override;
    void set_report(hid::report::type type, const std::span<const uint8_t> &data) override;

  public:
    keyboard_session() { receive_report(&leds_buffer_); }

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
                keys_input_report_descriptor<KEYS_6KRO_REPORT_ID>(),

                // LED report
                leds_output_report_descriptor<LEDS_REPORT_ID>(),

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
                leds_output_report_descriptor<LEDS_REPORT_ID>(),

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
        hid::report_bitset<hid::page::keyboard_keypad,
            hid::page::keyboard_keypad::KEYBOARD_LEFT_CONTROL,
            hid::page::keyboard_keypad::KEYBOARD_RIGHT_GUI>
            modifiers;
        hid::report_bitset<hid::page::keyboard_keypad, NKRO_FIRST_USAGE, NKRO_LAST_USAGE> scancodes;
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

  private:
    std::optional<keyboard_session> session_{};

    keyboard_app(const hid::report_protocol &rp) : hid::application(rp) {}
    hid::session &start(const hid::session_params &params) override;
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
