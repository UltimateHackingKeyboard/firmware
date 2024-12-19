#include "mouse_app.hpp"
#include "app_base.hpp"
#include "zephyr/sys/printk.h"

mouse_app &mouse_app::handle()
{
    static mouse_app app{};
    return app;
}

void mouse_app::start(hid::protocol prot)
{
    // TODO start handling mouse events
    report_buffer_ = {};
    resolution_buffer_ = {};
    receive_report(&resolution_buffer_);
}

void mouse_app::set_report_state(const mouse_report_base<> &data)
{
    send({data.data(), sizeof(data)});
}

void mouse_app::set_report(hid::report::type type, const std::span<const uint8_t> &data)
{
    if (hid::report::selector(type, data.front()) != resolution_buffer_.selector()) {
        return;
    }
    resolution_buffer_ = *reinterpret_cast<const decltype(resolution_buffer_) *>(data.data());
    receive_report(&resolution_buffer_);
}

void mouse_app::get_report(hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (select == resolution_buffer_.selector()) {
        send_report(&resolution_buffer_);
        return;
    }
    app_base::get_report(select, buffer);
}
