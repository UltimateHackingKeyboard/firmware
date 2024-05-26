#include "mouse_app.hpp"

mouse_app &mouse_app::handle()
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

void mouse_app::set_report_state(const mouse_report_base<> &data)
{
    auto buf_idx = report_buffer_.active_side();
    auto &report = report_buffer_[buf_idx];
    report.buttons = data.buttons;
    report.x = data.x;
    report.y = data.y;
    report.wheel_x = data.wheel_x;
    report.wheel_y = data.wheel_y;
    send_buffer(buf_idx);
}

void mouse_app::send_buffer(uint8_t buf_idx)
{
    if (!report_buffer_.differs()) {
        return;
    }
    if (send_report(&report_buffer_[buf_idx]) == hid::result::OK) {
        report_buffer_.compare_swap_copy(buf_idx);
    }
}

void mouse_app::in_report_sent(const std::span<const uint8_t> &data)
{
    if (data.front() != report_ids::IN_MOUSE) {
        return;
    }
    auto buf_idx = report_buffer_.indexof(data.data());
    if (!report_buffer_.compare_swap_copy(buf_idx)) {
        send_buffer(1 - buf_idx);
    }
}

void mouse_app::get_report(hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (select != mouse_report::selector()) {
        return;
    }
    // copy to buffer to avoid overwriting data in transit
    auto &report = report_buffer_[report_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), report.data(), sizeof(report));
    send_report(buffer.subspan(0, sizeof(report)));
}
