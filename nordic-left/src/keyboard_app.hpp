#ifndef __KEYBOARD_APP_HEADER__
#define __KEYBOARD_APP_HEADER__

#include "hid/app/keyboard.hpp"
#include "hid/application.hpp"

class keyboard_app : public hid::application
{
    static constexpr uint8_t KEYS_6KRO_REPORT_ID = 1;
    static constexpr uint8_t KEYS_NKRO_REPORT_ID = 2;
    static constexpr uint8_t LEDS_REPORT_ID = 1;

    // the first 4 codes are error codes, so not needed in bitmask
    static constexpr auto NKRO_FIRST_USAGE = hid::page::keyboard_keypad::A;
    // this bitfield goes up all the way until the modifiers
    static constexpr auto NKRO_LAST_USAGE = hid::page::keyboard_keypad::KEYPAD_HEXADECIMAL;

    static constexpr auto NKRO_USAGE_COUNT = 1 + static_cast<hid::usage_index_type>(NKRO_LAST_USAGE)
            - static_cast<hid::usage_index_type>(NKRO_FIRST_USAGE);

    static const hid::report_protocol& report_protocol();

    keyboard_app()
        : application(report_protocol())
    {}

public:
    using keys_boot_report = hid::app::keyboard::keys_input_report<0>;
    using keys_6kro_report = hid::app::keyboard::keys_input_report<KEYS_6KRO_REPORT_ID>;
    using leds_boot_report = hid::app::keyboard::output_report<0>;
    using leds_report = hid::app::keyboard::output_report<LEDS_REPORT_ID>;

    template<uint8_t REPORT_ID>
    struct keys_nkro_report_base : public hid::report::base<hid::report::type::INPUT, REPORT_ID>
    {
        uint8_t modifiers = 0;
        uint8_t reserved = 0;
        std::array<uint8_t, (NKRO_USAGE_COUNT + 7) / 8> bitfield {};

        constexpr keys_nkro_report_base() = default;

        // TODO: add helpers to set and clear scancodes
    };
    using keys_nkro_report = keys_nkro_report_base<KEYS_NKRO_REPORT_ID>;

    static keyboard_app& handle();

    using hid::application::send_report;

    void start(hid::protocol prot) override;
    void stop() override;
    void set_report(hid::report::type type, const std::span<const uint8_t>& data) override;
    void in_report_sent(const std::span<const uint8_t>& data) override;
    void get_report(hid::report::selector select, const std::span<uint8_t>& buffer) override;
    hid::protocol get_protocol() const override
    {
        return prot_;
    }

private:
    C2USB_USB_TRANSFER_ALIGN(leds_report, leds_buffer_) {};
    hid::protocol prot_ {};
};

#endif // __KEYBOARD_APP_HEADER__
