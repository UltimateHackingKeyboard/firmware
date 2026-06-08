#include "command_app.hpp"
#include <hid/rdf/descriptor.hpp>
#include <hid/report_protocol.hpp>
#if __has_include(<zephyr/sys/printk.h>)
    #include <zephyr/sys/printk.h>
#endif

command_app &command_app::usb_handle()
{
    static command_app app{};
    return app;
}

#if DEVICE_IS_UHK80_RIGHT
command_app &command_app::ble_handle()
{
    static command_app ble_app{};
    return ble_app;
}
#endif

void command_app::start(hid::protocol prot)
{
    // start receiving reports
    receive_report(&out_buffer_[out_buffer_.active_side()]);
}

void command_app::set_report(hid::report::type type, const std::span<const uint8_t> &data)
{
    if (type != hid::report::type::OUTPUT) {
        return;
    }
    if ((report_ids::OUT_COMMAND != 0) and (data.front() != report_ids::OUT_COMMAND)) {
        return;
    }
    // always keep receiving new reports
    out_buffer_.swap_sides();
    receive_report(&out_buffer_[out_buffer_.active_side()]);

    auto &out = *reinterpret_cast<const report_out *>(data.data());
    auto buf_idx = in_buffer_.active_side();
    auto &in = in_buffer_[buf_idx];
    UsbProtocolHandler(const_cast<uint8_t *>(out.payload.data()), in.payload.data());
    auto err = send_report(&in);
    if (err == hid::result::ok) {
        in_buffer_.swap_sides();
    } else {
#if __has_include(<zephyr/sys/printk.h>)
        printk("Command app failed to send report with: %d\n", std::bit_cast<int>(err));
#endif
    }
}

void command_app::get_report(hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (select != report_in::selector()) {
        return;
    }
    // copy to buffer to avoid overwriting data in transit
    auto &report = in_buffer_[in_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), report.data(), sizeof(report));
    send_report(buffer.subspan(0, sizeof(report)));
}

void command_app::in_report_sent(const std::span<const uint8_t> &data)
{
    if (data.front() != report_ids::IN_COMMAND) {
        return;
    }
    auto buf_idx = in_buffer_.indexof(data.data());
    in_buffer_.compare_swap(buf_idx);
}
