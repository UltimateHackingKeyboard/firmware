#ifndef __KEYBOARD_APP_HEADER__
#define __KEYBOARD_APP_HEADER__

#include "hid/app/keyboard.hpp"
#include "hid/application.hpp"
#include <bitset>

using scancode = hid::page::keyboard_keypad;

class keyboard_app : public hid::application
{
    static constexpr uint8_t KEYS_6KRO_REPORT_ID = 1;
    static constexpr uint8_t KEYS_NKRO_REPORT_ID = 2;
    static constexpr uint8_t LEDS_REPORT_ID = 1;

    static constexpr auto LOWEST_MODIFIER =
            static_cast<hid::usage_index_type>(scancode::LEFTCTRL);
    static constexpr auto HIGHEST_MODIFIER =
            static_cast<hid::usage_index_type>(scancode::RIGHTGUI);
            
    static constexpr auto NKRO_FIRST_USAGE = scancode::A; // the first 4 codes are error codes
    static constexpr auto NKRO_LAST_USAGE = scancode::KEYPAD_HEXADECIMAL; // TODO: reduce this to a sensible minimum
    static constexpr auto LOWEST_SCANCODE = static_cast<hid::usage_index_type>(NKRO_FIRST_USAGE); 
    static constexpr auto HIGHEST_SCANCODE = static_cast<hid::usage_index_type>(NKRO_LAST_USAGE); 

    static constexpr auto NKRO_USAGE_COUNT = 1 + HIGHEST_SCANCODE - LOWEST_SCANCODE;

public:
    enum class rollover
    {
        N_KEY = 0,
        SIX_KEY = 1,
    };
    static keyboard_app& handle();

    void set_rollover(rollover mode);
    rollover get_rollover() const { return rollover_; }

    template<uint8_t REPORT_ID>
    struct keys_nkro_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID>
    {
        void set_code(scancode code, bool value = true)
        {
            auto codeval = static_cast<hid::usage_index_type>(code);
            if ((codeval >= LOWEST_SCANCODE) and (codeval <= HIGHEST_SCANCODE)) {
                codeval -= LOWEST_SCANCODE;
                if (value) {
                    scancode_flags[codeval / 8] |= 1 << (codeval % 8);
                } else {
                    scancode_flags[codeval / 8] &= ~(1 << (codeval % 8));
                }
            } else if ((codeval >= LOWEST_MODIFIER) and (codeval <= HIGHEST_MODIFIER)) {
                codeval -= LOWEST_MODIFIER;
                if (value) {
                    modifiers |= 1 << codeval;
                } else {
                    modifiers &= ~(1 << codeval);
                }
            } else {
                assert(false);
            }
        }

        bool test(scancode code) const
        {
            auto codeval = static_cast<hid::usage_index_type>(code);
            if ((codeval >= LOWEST_SCANCODE) and (codeval <= HIGHEST_SCANCODE)) {
                codeval -= LOWEST_SCANCODE;
                return scancode_flags[codeval / 8] & (1 << (codeval % 8));
            } else if ((codeval >= LOWEST_MODIFIER) and (codeval <= HIGHEST_MODIFIER)) {
                codeval -= LOWEST_MODIFIER;
                return modifiers & (1 << codeval);
            } else {
                assert(false);
                return false;
            }
        }

        bool operator==(const keys_nkro_report_base& other) const = default;

        uint8_t modifiers {};
        std::array<uint8_t, (NKRO_USAGE_COUNT + 7) / 8> scancode_flags {};
    };

    hid::result send(const keys_nkro_report_base<0>& data);

private:
    static const hid::report_protocol& report_protocol();

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
    hid::protocol get_protocol() const override
    {
        return prot_;
    }

    C2USB_USB_TRANSFER_ALIGN(leds_report, leds_buffer_) {};
    hid::protocol prot_ {};
    rollover rollover_ {};
    C2USB_USB_TRANSFER_ALIGN(keys_6kro_report, keys_6kro_) {};
    bool tx_busy_ {};
    C2USB_USB_TRANSFER_ALIGN(keys_nkro_report, keys_nkro_) {};
};

using scancode_buffer = keyboard_app::keys_nkro_report_base<0>;

#endif // __KEYBOARD_APP_HEADER__
