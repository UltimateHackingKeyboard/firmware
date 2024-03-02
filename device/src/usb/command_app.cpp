#include "command_app.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"
#include "zephyr/sys/printk.h"

extern "C" bool CommandProtocolTx(const uint8_t* data, size_t size)
{
    return command_app::handle().send(std::span<const uint8_t>(data, size));
}

extern "C" void CommandProtocolRxHandler(const uint8_t* data, size_t size);

void __attribute__((weak)) CommandProtocolRxHandler(const uint8_t* data, size_t size)
{
    printk("CommandProtocolRxHandler: data[0]:%u size:%d\n", data[0], size);
    CommandProtocolTx(data, size);
}

command_app& command_app::handle()
{
    static command_app app{};
    return app;
}

void command_app::start(hid::protocol prot)
{
    // start receiving reports
    receive_report(&out_buffer_[out_buffer_.active_side()]);
}

void command_app::set_report(hid::report::type type, const std::span<const uint8_t>& data)
{
    // only one report is receivable
    CommandProtocolRxHandler(data.data(), data.size());

    // always keep receiving new reports
    out_buffer_.swap_sides();
    receive_report(&out_buffer_[out_buffer_.active_side()]);
}

void command_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    auto& report = in_buffer_[in_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), report.data(), sizeof(report));
    send_report(buffer.subspan(0, sizeof(report)));
}

bool command_app::send(std::span<const uint8_t> buffer)
{
    auto buf_idx = in_buffer_.active_side();
    auto& report = in_buffer_[buf_idx];
    std::copy(buffer.begin(), buffer.end(), report.payload.begin());
    if (send_report(&in_buffer_[buf_idx]) == hid::result::OK)
    {
        in_buffer_.compare_swap(buf_idx);
        return true;
    }
    return false;
}

void command_app::in_report_sent(const std::span<const uint8_t>& data)
{
    auto buf_idx = in_buffer_.indexof(data.data());
    in_buffer_.compare_swap(buf_idx);
}
