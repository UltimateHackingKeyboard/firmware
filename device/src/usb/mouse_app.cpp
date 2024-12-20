#include "mouse_app.hpp"
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
}

void mouse_app::set_report_state(const mouse_report_base<> &data)
{
    send({data.data(), sizeof(data)});
}
