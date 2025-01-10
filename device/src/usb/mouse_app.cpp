#include "mouse_app.hpp"
#include "zephyr/sys/printk.h"

mouse_app &mouse_app::usb_handle()
{
    static mouse_app app{};
    return app;
}
#if DEVICE_IS_UHK80_RIGHT
mouse_app &mouse_app::ble_handle()
{
    static mouse_app ble_app{};
    return ble_app;
}
#endif

void mouse_app::start(hid::protocol prot)
{
    // TODO start handling mouse events
    report_buffer_ = {};
}

void mouse_app::set_report_state(const mouse_report_base<> &data)
{
    send({data.data(), sizeof(data)});
}
