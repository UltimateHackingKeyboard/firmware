#include "gamepad_app.hpp"

gamepad_app& gamepad_app::handle()
{
    static gamepad_app app{};
    return app;
}

void gamepad_app::start(hid::protocol prot)
{
    // TODO start handling gamepad events
    report_buffer_.reset();
}

void gamepad_app::stop()
{
    // TODO stop handling gamepad events
}

void gamepad_app::set_report_state(const gamepad_report& data)
{
    auto buf_idx = report_buffer_.active_side();
    auto& report = report_buffer_[buf_idx];
    report = data;
    send_buffer(buf_idx);
}

void gamepad_app::send_buffer(uint8_t buf_idx)
{
    if (!report_buffer_.differs())
    {
        return;
    }
    if (send_report(&report_buffer_[buf_idx]) == hid::result::OK)
    {
        report_buffer_.compare_swap_copy(buf_idx);
    }
}

void gamepad_app::in_report_sent(const std::span<const uint8_t>& data)
{
    auto buf_idx = report_buffer_.indexof(data.data());
    if (!report_buffer_.compare_swap_copy(buf_idx))
    {
        send_buffer(1 - buf_idx);
    }
}

void gamepad_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    auto& report = report_buffer_[report_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), report.data(), sizeof(report));
    send_report(buffer.subspan(0, sizeof(report)));
}
