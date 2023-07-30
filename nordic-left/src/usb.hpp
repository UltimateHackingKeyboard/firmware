#ifndef __USB_HEADER__
#define __USB_HEADER__

#include <zephyr/device.h>
#include "hid/report_protocol.hpp"
#include "hid/app/mouse.hpp"
#include "hid/app/keyboard.hpp"
#include "usb/df/class/hid.hpp"

void usb_init(const device* dev);

class keyboard_app : public hid::application
{
    static const auto& report_prot()
    {
        static constexpr auto rd { hid::app::keyboard::app_report_descriptor<0>() };
        static constexpr hid::report_protocol rp {rd};
        return rp;
    }

public:
    using keys_report    = hid::app::keyboard::keys_input_report<0>;
    using kb_leds_report = hid::app::keyboard::output_report<0>;
    using hid::application::send_report;

    keyboard_app(hid::page::keyboard_keypad key, hid::page::leds led);
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
    C2USB_USB_TRANSFER_ALIGN(keys_report, keys_buffer_) {};
    const uint8_t key_ {};
    const uint8_t led_mask_ {};
    C2USB_USB_TRANSFER_ALIGN(kb_leds_report, leds_buffer_) {};
    hid::protocol prot_ {};
};

extern keyboard_app kb;

#endif // __USB_HEADER__