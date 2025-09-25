#include "mouse_app.hpp"
#include "app_base.hpp"
#include "zephyr/sys/printk.h"

extern "C" {
#include "state_sync.h"
}

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
    report_buffer_ = {};
    receive_report(&resolution_buffer_);
}

void mouse_app::stop()
{
    app_base::stop();
    set_resolution_report({});
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
    set_resolution_report(*reinterpret_cast<const decltype(resolution_buffer_) *>(data.data()));
    receive_report(&resolution_buffer_);
}

void mouse_app::set_resolution_report(const scroll_resolution_report &report)
{
    resolution_buffer_ = report;
    // When running on dongle, update the scroll multiplier state
    if (DEVICE_IS_UHK_DONGLE) {
        DongleScrollMultipliers.vertical = resolution_buffer_.vertical_scroll_multiplier();
        DongleScrollMultipliers.horizontal = resolution_buffer_.horizontal_scroll_multiplier();
        StateSync_UpdateProperty(StateSyncPropertyId_DongleScrollMultipliers, NULL);
    }
}

void mouse_app::get_report(hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (select == resolution_buffer_.selector()) {
        send_report(&resolution_buffer_);
        return;
    }
    app_base::get_report(select, buffer);
}
