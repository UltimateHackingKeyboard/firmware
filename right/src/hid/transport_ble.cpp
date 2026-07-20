#include "ble_app.hpp"
#include <bluetooth/hid_over_gatt.hpp>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>

using namespace magic_enum::bitwise_operators;

static auto &hog_service()
{
    using namespace magic_enum::bitwise_operators;
    using namespace bluetooth::hid_over_gatt;

    // TODO: set values as needed
    const auto sec_lvl = bluetooth::security::L2_UNAUTH_ENC; // TODO: bump to L3_AUTH_ENC?
    static constexpr auto features = flags::NORMALLY_CONNECTABLE | flags::REMOTE_WAKE;

    using hogp_type = service_instance<ble_app::report_desc(), hid::boot::mode::KEYBOARD>;
    static hogp_type hog{ble_app::handle(), sec_lvl, features};

    static const STRUCT_SECTION_ITERABLE(bt_gatt_service_static, keyboard_hogp) =
        hog.static_service();
    return hog;
}

ble_session *ble_session::lookup_by_conn(::bt_conn *conn)
{
    return static_cast<ble_session *>(hog_service().get_session(conn));
}

::bt_conn *ble_session::get_conn()
{
    return hog_service().get_session_conn(*this);
}

hid::session &ble_app::start(const hid::session_params &params)
{
    ::bt_conn *conn = static_cast<bluetooth::hid_over_gatt::session_params>(params).conn;
    ble_session *sess;
    // TODO
    // create session from memory pool (allocate + construct)
    // Connections_SetStateAsync

    // mouse_resolution_changed_callback(*sess, sess->resolution_report());
    return *sess;
}

void ble_app::stop(hid::session &sess)
{
    // TODO
    // return session to memory pool (destruct + dealloc)
    // Connections_SetStateAsync
}

extern "C" int HOGP_HealthCheck()
{
    // TODO: refactoring needed
    struct bt_conn *peer = nullptr;
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

    printk("HOGP HealthCheck: OK (registered, peer connected, interval %u)\n", info.le.interval_us);
    return 0;
}
