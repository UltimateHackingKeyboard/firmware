#include "command_app.hpp"
#include "controls_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#include "port/zephyr/bluetooth/hid.hpp"
#include <magic_enum.hpp>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>

using namespace magic_enum::bitwise_operators;

template <typename... Args>
class multi_hid : public hid::multi_application {
  public:
    static constexpr auto report_desc() { return hid::rdf::descriptor((Args::report_desc(), ...)); }

    static multi_hid &handle()
    {
        static multi_hid s;
        return s;
    }

  private:
    std::array<hid::application *, sizeof...(Args) + 1> app_array_{
        (&Args::ble_handle())..., nullptr};
    constexpr multi_hid()
        : multi_application(hid::report_protocol::from_descriptor<report_desc()>(), app_array_)
    {}
};

using multi_hid_nopad = multi_hid<keyboard_app, mouse_app, command_app, controls_app>;

struct hogp_manager {
    static hogp_manager &instance()
    {
        static hogp_manager hm;
        return hm;
    }

    static bool active() { return instance().hogp_nopad_.active(); }

    bluetooth::zephyr::hid::service &main_service() { return hogp_nopad_; }

  private:
    hogp_manager() {}

    static const auto security = bluetooth::zephyr::hid::security::ENCRYPT;
    static const auto features = bluetooth::zephyr::hid::flags::NORMALLY_CONNECTABLE |
                                 bluetooth::zephyr::hid::flags::REMOTE_WAKE;

    bluetooth::zephyr::hid::service_instance<multi_hid_nopad::report_desc(),
        bluetooth::zephyr::hid::boot_protocol_mode::KEYBOARD>
        hogp_nopad_{multi_hid_nopad::handle(), security, features};
};

extern "C" void HOGP_Enable()
{
    hogp_manager::instance().main_service().start();
}

extern "C" void HOGP_Disable()
{
    hogp_manager::instance().main_service().stop();
}

extern "C" int HOGP_HealthCheck()
{
    auto &hm = hogp_manager::instance();
    auto &svc = hm.main_service();

    if (!hm.active()) {
        printk("HOGP HealthCheck: GATT service NOT registered\n");
        return -1;
    }

    struct bt_conn *peer = svc.peer();
    if (!peer) {
        printk("HOGP HealthCheck: service registered, no active peer\n");
        return -2;
    }

    struct bt_conn_info info;
    int err = bt_conn_get_info(peer, &info);
    if (err) {
        printk("HOGP HealthCheck: active peer has INVALID conn pointer (err %d)\n", err);
        return -3;
    }

    printk("HOGP HealthCheck: OK (registered, peer connected, interval %u)\n",
           info.le.interval);
    return 0;
}
