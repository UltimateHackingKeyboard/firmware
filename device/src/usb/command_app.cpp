#include "command_app.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"
#include "zephyr/sys/printk.h"

extern "C" void UsbProtocolHandler(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

command_app &command_app::usb_handle()
{
    static command_app app{};
    return app;
}

#if DEVICE_IS_UHK80_RIGHT
command_app &command_app::handle()
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
    if (((type != hid::report::type::OUTPUT) || (data.front() != report_ids::OUT_COMMAND))) {
        return;
    }
    // always keep receiving new reports
    out_buffer_.swap_sides();
    receive_report(&out_buffer_[out_buffer_.active_side()]);

    auto &out = *reinterpret_cast<const report_out *>(data.data());
    auto buf_idx = in_buffer_.active_side();
    auto &in = in_buffer_[buf_idx];
    UsbProtocolHandler(out.payload.data(), in.payload.data());
    auto err = send_report(&in);
    if (err == hid::result::OK) {
        in_buffer_.compare_swap(buf_idx);
    } else {
        printk("Command app failed to send report with: %d\n", (int)err);
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
