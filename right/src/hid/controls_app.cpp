#include "controls_app.hpp"

controls_app &controls_app::usb_handle()
{
    static controls_app app{};
    return app;
}

#if DEVICE_IS_UHK80_RIGHT
controls_app &controls_app::ble_handle()
{
    static controls_app ble_app{};
    return ble_app;
}
#endif

void controls_app::start(hid::protocol prot)
{
    report_buffer_.reset();
}

int controls_app::send_report(const hid_controls_report_t &report)
{
    auto &buf = report_buffer_[report_buffer_.active_side()];
    buf.data = report;
    auto result = application::send_report(
        std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(&buf), sizeof(buf)));
    if (result == hid::result::ok) {
        report_buffer_.swap_sides();
    }
    return result.to_int();
}

void controls_app::in_report_sent(const std::span<const uint8_t> &data)
{
    Hid_ControlsReportSentCallback((this == &usb_handle()) ? HID_TRANSPORT_USB : HID_TRANSPORT_BLE);
}

void controls_app::get_report(hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (select != controls_report::selector()) {
        return;
    }

    // copy to buffer to avoid overwriting data in transit
    auto &report = report_buffer_[report_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), &report, sizeof(report));
    application::send_report(buffer.subspan(0, sizeof(report)));
}
