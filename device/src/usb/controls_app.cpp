#include "controls_app.hpp"

extern "C"
{
#include "usb/usb.h"
#include <zephyr/kernel.h>
}

static K_SEM_DEFINE(reportSending, 1, 1);

controls_app& controls_app::handle()
{
    static controls_app app{};
    return app;
}

void controls_app::start(hid::protocol prot)
{
    // TODO start handling controls events
    report_buffer_.reset();
}

void controls_app::stop()
{
    // TODO stop handling controls events
}

void controls_app::set_report_state(const controls_report& data)
{
    k_sem_take(&reportSending, K_MSEC(SEMAPHOR_RESET_TIMEOUT));
    auto buf_idx = report_buffer_.active_side();
    auto& report = report_buffer_[buf_idx];
    report = data;
    send_buffer(buf_idx);
}

void controls_app::send_buffer(uint8_t buf_idx)
{
    if (!report_buffer_.differs())
    {
        k_sem_give(&reportSending);
        return;
    }
    if (send_report(&report_buffer_[buf_idx]) == hid::result::OK)
    {
        report_buffer_.compare_swap_copy(buf_idx);
        k_sem_give(&reportSending);
    }
}

void controls_app::in_report_sent(const std::span<const uint8_t>& data)
{
    auto buf_idx = report_buffer_.indexof(data.data());
    if (!report_buffer_.compare_swap_copy(buf_idx))
    {
        send_buffer(1 - buf_idx);
    }
}

void controls_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // copy to buffer to avoid overwriting data in transit
    auto& report = report_buffer_[report_buffer_.inactive_side()];
    assert(buffer.size() >= sizeof(report));
    memcpy(buffer.data(), report.data(), sizeof(report));
    send_report(buffer.subspan(0, sizeof(report)));
}
