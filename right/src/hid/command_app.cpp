#include "command_app.hpp"
#include <hid/rdf/descriptor.hpp>
#include <hid/report_protocol.hpp>
#if __has_include(<zephyr/sys/printk.h>)
    #include <zephyr/sys/printk.h>
#endif

hid::session &command_app::start(const hid::session_params &params)
{
    assert(!session_.has_value());
    return session_.emplace();
}

void command_app::stop(hid::session &sess)
{
    assert(&sess == &session_.value());
    return session_.reset();
}

void command_session::set_report(hid::report::type type, const std::span<const uint8_t> &data)
{
    if (type != hid::report::type::OUTPUT) {
        return;
    }
    if ((report_ids::OUT_COMMAND != 0) and (data.front() != report_ids::OUT_COMMAND)) {
        return;
    }
    UsbProtocolHandler(out_buffer_.payload.data(), in_buffer_.payload.data());
    auto err = send_report(&in_buffer_);
    if (err != c2usb::result::ok) {
#if __has_include(<zephyr/sys/printk.h>)
        printk("Command response failed to send (%d)\n", std::bit_cast<int>(err));
#endif
    }
    receive_report(&out_buffer_);
}

std::span<const uint8_t> command_session::get_report(
    hid::report::selector select, const std::span<uint8_t> &buffer)
{
    switch (select.type()) {
    case hid::report::type::INPUT:
        in_buffer_.payload.fill(0);
        return std::span<const uint8_t>(in_buffer_.data(), sizeof(in_buffer_));
    case hid::report::type::OUTPUT:
        out_buffer_.payload.fill(0);
        return std::span<const uint8_t>(out_buffer_.data(), sizeof(out_buffer_));
    default:
        return {};
    }
}
