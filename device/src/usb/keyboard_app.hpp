#ifndef __KEYBOARD_APP_HEADER__
#define __KEYBOARD_APP_HEADER__

#include "double_buffer.hpp"
#include "hid/app/keyboard.hpp"
#include "hid/application.hpp"
#include "hid/report_protocol.hpp"

using scancode = hid::page::keyboard_keypad;

class keyboard_app : public hid::application
{
    static constexpr uint8_t KEYS_6KRO_REPORT_ID = 1;
    static constexpr uint8_t KEYS_NKRO_REPORT_ID = 2;
    static constexpr uint8_t LEDS_REPORT_ID = 1;

    static constexpr auto NKRO_FIRST_USAGE =
        scancode::KEYBOARD_A; // the first 4 codes are error codes
    static constexpr auto NKRO_LAST_USAGE =
        scancode::KEYPAD_HEXADECIMAL; // TODO: reduce this to a sensible minimum
    static constexpr auto LOWEST_SCANCODE = NKRO_FIRST_USAGE;
    static constexpr auto HIGHEST_SCANCODE = NKRO_LAST_USAGE;

    static constexpr auto NKRO_USAGE_COUNT =
        1 + static_cast<size_t>(HIGHEST_SCANCODE) - static_cast<size_t>(LOWEST_SCANCODE);

  public:
    static constexpr auto report_desc()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        using namespace hid::app::keyboard;

        // clang-format off
        return descriptor(
            // 6KRO keyboard with report ID
            usage_extended(generic_desktop::KEYBOARD),
            collection::application(
                // input keys report
                keys_input_report_descriptor<KEYS_6KRO_REPORT_ID>(),

                // LED report
                leds_output_report_descriptor<LEDS_REPORT_ID>()
            ),
            // NKRO keyboard with report ID, no LEDs
            usage_extended(generic_desktop::KEYBOARD),
            collection::application(

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

    enum class rollover
    {
        N_KEY = 0,
        SIX_KEY = 1,
    };
    static keyboard_app& handle();

    void set_rollover(rollover mode);
    rollover get_rollover() const { return rollover_; }

    template <uint8_t REPORT_ID = 0>
    struct keys_nkro_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID>
    {
        hid::report_bitset<hid::page::keyboard_keypad,
                           hid::page::keyboard_keypad::KEYBOARD_LEFT_CONTROL,
                           hid::page::keyboard_keypad::KEYBOARD_RIGHT_GUI>
            modifiers;
        hid::report_bitset<hid::page::keyboard_keypad, NKRO_FIRST_USAGE, NKRO_LAST_USAGE> scancodes;
        void set_code(scancode code, bool value = true)
        {
            if (modifiers.set(code, value))
            {
                return;
            }
            if (scancodes.set(code, value))
            {
                return;
            }
            assert(false);
        }

        bool test(scancode code) const
        {
            if (modifiers.in_range(code))
            {
                return modifiers.test(code);
            }
            if (scancodes.in_range(code))
            {
                return scancodes.test(code);
            }
            assert(false);
            return false;
        }

        bool operator==(const keys_nkro_report_base& other) const = default;
        bool operator!=(const keys_nkro_report_base& other) const = default;
    };

    void set_report_state(const keys_nkro_report_base<>& data);

  private:
    static const hid::report_protocol& report_protocol()
    {
        static constexpr const auto rd{report_desc()};
        static constexpr const hid::report_protocol rp{rd};
        return rp;
    }

    keyboard_app()
        : application(report_protocol())
    {}

    using keys_boot_report = hid::app::keyboard::keys_input_report<0>;
    using keys_6kro_report = hid::app::keyboard::keys_input_report<KEYS_6KRO_REPORT_ID>;
    using leds_boot_report = hid::app::keyboard::output_report<0>;
    using leds_report = hid::app::keyboard::output_report<LEDS_REPORT_ID>;

    using keys_nkro_report = keys_nkro_report_base<KEYS_NKRO_REPORT_ID>;

    void start(hid::protocol prot) override;
    void stop() override;
    void set_report(hid::report::type type, const std::span<const uint8_t>& data) override;
    void in_report_sent(const std::span<const uint8_t>& data) override;
    void get_report(hid::report::selector select, const std::span<uint8_t>& buffer) override;
    hid::protocol get_protocol() const override { return prot_; }

    void send_6kro_buffer(uint8_t buf_idx);
    void send_nkro_buffer(uint8_t buf_idx);

    C2USB_USB_TRANSFER_ALIGN(leds_report, leds_buffer_){};
    hid::protocol prot_{};
    rollover rollover_{};
    rollover rollover_override_{};
    double_buffer<keys_6kro_report> keys_6kro_{};
    double_buffer<keys_nkro_report> keys_nkro_{};
};

using scancode_buffer = keyboard_app::keys_nkro_report_base<>;

#endif // __KEYBOARD_APP_HEADER__
