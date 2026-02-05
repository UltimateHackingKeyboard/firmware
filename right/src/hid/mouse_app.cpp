#include "mouse_app.hpp"

mouse_app &mouse_app::usb_handle()
{
    static mouse_app app{};
    return app;
}
#if DEVICE_IS_UHK80_RIGHT
mouse_app &mouse_app::ble_handle()
{
    static mouse_app ble_app{};
    return ble_app;
}
#endif

void mouse_app::start(hid::protocol prot)
{
    report_buffer_.reset();
    resolution_buffer_ = {};
    receive_report(&resolution_buffer_);
}

int mouse_app::send_report(const hid_mouse_report_t &report)
{
    auto &buf = report_buffer_[report_buffer_.active_side()];
    buf.data = report;
    auto result = hid::application::send_report(
        std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(&buf), sizeof(buf)));
    if (result == hid::result::ok) {
        report_buffer_.swap_sides();
    }
    return result.to_int();
}

void mouse_app::in_report_sent(const std::span<const uint8_t> &data)
{
    Hid_MouseReportSentCallback((this == &usb_handle()) ? HID_TRANSPORT_USB : HID_TRANSPORT_BLE);
}

void mouse_app::set_report(hid::report::type type, const std::span<const uint8_t> &data)
{
    if (hid::report::selector(type, data.front()) != resolution_buffer_.selector()) {
        return;
    }
    resolution_buffer_ = *reinterpret_cast<const decltype(resolution_buffer_) *>(data.data());
    receive_report(&resolution_buffer_);

    Hid_MouseScrollResolutionsChanged(
        (this == &usb_handle()) ? HID_TRANSPORT_USB : HID_TRANSPORT_BLE,
        resolution_buffer_.vertical_scroll_multiplier(),
        resolution_buffer_.horizontal_scroll_multiplier());
}

void mouse_app::get_report(hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (select == resolution_buffer_.selector()) {
        hid::application::send_report(&resolution_buffer_);
        return;
    }

    if (select != mouse_report::selector()) {
        return;
    }

    // copy to buffer to avoid overwriting data in transit
    auto &report = report_buffer_[report_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), &report, sizeof(report));
    hid::application::send_report(buffer.subspan(0, sizeof(report)));
}
