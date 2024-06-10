#include "mouse_app.hpp"

extern "C" {
#include <zephyr/kernel.h>
#include "legacy/debug.h"
}

static K_SEM_DEFINE(reportSending, 1, 1);

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
    k_sem_take(&reportSending, K_MSEC(100));
    auto buf_idx = report_buffer_.active_side();
    auto& report = report_buffer_[buf_idx];
    report = data;
    send_buffer(buf_idx);
}

bool mouse_app::swap_buffers(uint8_t buf_idx) {
    auto& report = report_buffer_[buf_idx];
    auto& otherReport = report_buffer_[1-buf_idx];

    if (report_buffer_.compare_swap_copy(buf_idx)) {
        // report report is now inactive and waiting to be sent
        // otherReport is now active and ready to be written
        otherReport.x = 0;
        otherReport.y = 0;
        otherReport.wheel_x = 0;
        otherReport.wheel_y = 0;
        return true;
    }
    return false;
}

void mouse_app::send_buffer(uint8_t buf_idx)
{
    auto& report = report_buffer_[buf_idx];
    if (!report_buffer_.differs() && report.x == 0 && report.y == 0 && report.wheel_x == 0 && report.wheel_y == 0)
    {
        k_sem_give(&reportSending);
        return;
    }
    // an asynchronous api to send the buffer
    // OK means that the report is scheduled to be sent
    // BUSY: means that another report is already being sent. 
    //       In that case, in_report_sent will trigger another sent attempt
    if (send_report(&report_buffer_[buf_idx]) == hid::result::OK)
    {
        // report has been submitted for sending, so swap buffers
        swap_buffers(buf_idx);
        // now inactive buffer is waiting to be sent
        // and active buffer is prepared for further writing
        k_sem_give(&reportSending);
    }
}

void mouse_app::in_report_sent(const std::span<const uint8_t>& data)
{
    // inactive buffer has been sent
    auto buf_idx = report_buffer_.indexof(data.data()); // this should now point to inactive buffer
    // so swapping should fail, which means that everything is ok and we can
    // continue and try to send the active buffer
    if (!swap_buffers(buf_idx))
    {
        // If someone tried to set_report_state inbetween, they have failed to 
        // send the buffer, so we need to take care of that.
        send_buffer(1 - buf_idx);
    }
    // else we have swapped buffers before the send_buffer (above) managed to
    // do so. In that case, *we* have swapped buffers, and the send_buffer's
    // swap will fail
}

void mouse_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    auto& report = report_buffer_[report_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), report.data(), sizeof(report));
    send_report(buffer.subspan(0, sizeof(report)));
}
