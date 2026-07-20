#include "hid/ble_app.hpp"
#include <bluetooth/hid_over_gatt.hpp>
extern "C" {
#include "hid/transport.h"
#include "utils.h"
#if __has_include(<zephyr/sys/printk.h>)
    #include <zephyr/sys/printk.h>
#endif
}

void ble_session::set_report(hid::report::type type, const std::span<const uint8_t> &data)
{
    if (protocol() == hid::protocol::BOOT) {
        if ((type == hid::report::type::OUTPUT) and (data.size() >= sizeof(leds_boot_report))) {
            leds_buffer_.leds = reinterpret_cast<const leds_boot_report *>(data.data())->leds;

            keyboard_leds_changed_callback(*this);
        }
    } else {
        // report protocol, report identified by type and report ID
        switch (hid::report::selector(type, data.front())) {
        case leds_report::selector(): {
            if (data.size() >= sizeof(leds_report)) {
                leds_buffer_.leds = reinterpret_cast<const leds_report *>(data.data())->leds;

                keyboard_leds_changed_callback(*this);
            }
            break;
        }
        case resolution_buffer_.selector(): {
            if (data.size() >= sizeof(resolution_buffer_)) {
                mouse_resolution_changed_callback(*this, resolution_report());
            }
            break;
        }
        case out_buffer_.selector():
            std::ignore = new (in_buffer_.data()) command_session::report_in{};
            UsbProtocolHandler(out_buffer_.payload.data(), in_buffer_.payload.data());

            if (auto err = send_report(&in_buffer_); err != c2usb::result::ok) {
#if __has_include(<zephyr/sys/printk.h>)
                printk("Command response failed to send (%d)\n", std::bit_cast<int>(err));
#endif
            }
            break;
        }
    }

    // reports are received by type, use the largest buffer for each type
    if (type == hid::report::type::OUTPUT) {
        receive_report(&out_buffer_);
    } else if (type == hid::report::type::FEATURE) {
        receive_report(&resolution_buffer_);
    }
}

std::span<const uint8_t> ble_session::get_report(
    hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (protocol() == hid::protocol::BOOT) {
        if (select == keys_boot_report::selector()) {
            assert(buffer.size() >= sizeof(keys_boot_report));
            std::ignore = new (buffer.data()) keys_boot_report{};
            return buffer.subspan(0, sizeof(keys_boot_report));
        }
        if (select == leds_boot_report::selector()) {
            assert(buffer.size() >= sizeof(leds_boot_report));
            auto *ptr = new (buffer.data()) leds_boot_report{};
            ptr->leds = leds_buffer_.leds;
            return buffer.subspan(0, sizeof(leds_boot_report));
        }
        {
            return {};
        }
    }
    // reuse in_buffer_ for all input reports, as the provided buffer size is insufficient (at least on Android)
    // always send empty reports for simplicity (as the session doesn't own the input report buffers)
    switch (select) {

    case keys_6kro_report::selector():
        std::ignore = new (in_buffer_.data()) keys_6kro_report{};
        return std::span<const uint8_t>(in_buffer_.data(), sizeof(keys_6kro_report));

    case keys_nkro_report::selector():
        std::ignore = new (in_buffer_.data()) keys_nkro_report{};
        return std::span<const uint8_t>(in_buffer_.data(), sizeof(keys_nkro_report));

    case leds_report::selector(): {
        assert(buffer.size() >= sizeof(leds_report));
        auto *ptr = new (buffer.data()) leds_report{};
        ptr->leds = leds_buffer_.leds;
        return buffer.subspan(0, sizeof(leds_report));
    }

    case mouse_report::selector():
        std::ignore = new (in_buffer_.data()) mouse_report{};
        return std::span<const uint8_t>(in_buffer_.data(), sizeof(mouse_report));

    case resolution_buffer_.selector():
        return std::span<const uint8_t>(resolution_buffer_.data(), sizeof(resolution_buffer_));

    case out_buffer_.selector():
        std::ignore = new (in_buffer_.data()) decltype(out_buffer_){};
        return std::span<const uint8_t>(in_buffer_.data(), sizeof(out_buffer_));

    case in_buffer_.selector():
        std::ignore = new (in_buffer_.data()) decltype(in_buffer_){};
        return std::span<const uint8_t>(in_buffer_.data(), sizeof(in_buffer_));

    case controls_report::selector():
        std::ignore = new (in_buffer_.data()) controls_report{};
        return std::span<const uint8_t>(in_buffer_.data(), sizeof(controls_report));

    default:
        return {};
    }
}

void ble_session::report_sent(const std::span<const uint8_t> &data)
{
    if (protocol() == hid::protocol::BOOT) {
        keyboard_report_sent_callback(*this);
    } else {
        switch (data.front()) {
        case keys_6kro_report::selector().id():
        case keys_nkro_report::selector().id():
            keyboard_report_sent_callback(*this);
            break;
        case mouse_report::selector().id():
            mouse_report_sent_callback(*this);
            break;
        case controls_report::selector().id():
            controls_report_sent_callback(*this);
            break;
        case command_session::report_in::selector().id():
            break;
        default:
            break;
        }
    }
}
