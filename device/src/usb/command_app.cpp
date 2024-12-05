#include "command_app.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"
#include "zephyr/sys/printk.h"

extern "C" bool CommandProtocolTx(const uint8_t *data, size_t size)
{
    return command_app::handle().send(std::span<const uint8_t>(data, size));
}

extern "C" void CommandProtocolRxHandler(const uint8_t *data, size_t size);

void __attribute__((weak)) CommandProtocolRxHandler(const uint8_t *data, size_t size)
{
    printk("CommandProtocolRxHandler: data[0]:%u size:%d\n", data[0], size);
    CommandProtocolTx(data, size);
}

command_app &command_app::handle()
{
    static command_app app{};
    return app;
}

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
    auto &out = *reinterpret_cast<const report_out *>(data.data());
    CommandProtocolRxHandler(out.payload.data(), data.size() - (report_out::has_id() ? 1 : 0));

    // always keep receiving new reports
    out_buffer_.swap_sides();
    receive_report(&out_buffer_[out_buffer_.active_side()]);
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

bool command_app::send(std::span<const uint8_t> buffer)
{
    auto buf_idx = in_buffer_.active_side();
    auto &report = in_buffer_[buf_idx];
    if (buffer.size() > report.payload.max_size()) {
        printk("Usb payload exceeded maximum size!\n");
        return false;
    }
    std::copy(buffer.begin(), buffer.end(), report.payload.begin());
    c2usb::result err = send_report(&in_buffer_[buf_idx]);
    if (err == hid::result::OK) {
        in_buffer_.compare_swap(buf_idx);
        return true;
    }
    printk("Command app failed to send report with: %i\n", (int)err);
    return false;
}

void command_app::in_report_sent(const std::span<const uint8_t> &data)
{
    if (data.front() != report_ids::IN_COMMAND) {
        return;
    }
    auto buf_idx = in_buffer_.indexof(data.data());
    in_buffer_.compare_swap(buf_idx);
}
