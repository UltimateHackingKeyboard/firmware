#include "mouse_app.hpp"

mouse_app& mouse_app::handle()
{
    static mouse_app app{};
    return app;
}

void mouse_app::start(hid::protocol prot)
{
    // TODO start handling mouse events
    report_buffer_.reset();
}

void mouse_app::stop()
{
    // TODO stop handling mouse events
}

void mouse_app::set_report_state(const mouse_report_base<>& data)
{
    auto buf_idx = report_buffer_.active_side();
    auto& report = report_buffer_[buf_idx];
    report = data;
    send_buffer(buf_idx);
}

bool mouse_app::swap_buffers(uint8_t buf_idx) {
    auto& report = report_buffer_[buf_idx];
    report.x = 0;
    report.y = 0;
    report.wheel_x = 0;
    report.wheel_y = 0; 

    return report_buffer_.compare_swap_copy(buf_idx);
}

void mouse_app::send_buffer(uint8_t buf_idx)
{
    if (!report_buffer_.differs())
    {
        return;
    }
    if (send_report(&report_buffer_[buf_idx]) == hid::result::OK)
    {
        swap_buffers(buf_idx);
    }
}

void mouse_app::in_report_sent(const std::span<const uint8_t>& data)
{
    auto buf_idx = report_buffer_.indexof(data.data());
    if (!swap_buffers(buf_idx))
    {
        send_buffer(1 - buf_idx);
    }
}

void mouse_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    auto& report = report_buffer_[report_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), report.data(), sizeof(report));
    send_report(buffer.subspan(0, sizeof(report)));
}
