extern "C" {
#include "device.h"
}
#if DEVICE_HAS_BATTERY
    #include "hid_battery_app.hpp"

extern "C" void HID_SetBatteryStatus(uint8_t remaining_capacity, bool charging)
{
    return hid_battery_app::handle().send(remaining_capacity, charging);
}

hid_battery_app &hid_battery_app::handle()
{
    static hid_battery_app app{};
    return app;
}

void hid_battery_app::send(uint8_t remaining_capacity, bool charging)
{
    auto buf_idx = report_buffer_.active_side();
    auto &r = report_buffer_[buf_idx];
    r.remaining_capacity = remaining_capacity;
    r.charging = charging;
    send_buffer(buf_idx);
}

void hid_battery_app::send_buffer(uint8_t buf_idx)
{
    if (!report_buffer_.differs()) {
        return;
    }
    if (send_report(&report_buffer_[buf_idx]) == hid::result::OK) {
        report_buffer_.compare_swap_copy(buf_idx);
    }
}

void hid_battery_app::in_report_sent(const std::span<const uint8_t> &data)
{
    auto buf_idx = report_buffer_.indexof(data.data());
    if (!report_buffer_.compare_swap_copy(buf_idx)) {
        send_buffer(1 - buf_idx);
    }
}

void hid_battery_app::get_report(hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (select != report::selector()) {
        return;
    }
    // copy to buffer to avoid overwriting data in transit
    auto &report = report_buffer_[report_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), report.data(), sizeof(report));
    send_report(buffer.subspan(0, sizeof(report)));
}

#endif // DEVICE_HAS_BATTERY
