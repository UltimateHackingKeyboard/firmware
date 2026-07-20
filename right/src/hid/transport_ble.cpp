#include "ble_app.hpp"
#include <bluetooth/hid_over_gatt.hpp>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <new>
extern "C" {
#include "bt_conn.h"
}

using namespace magic_enum::bitwise_operators;

static_assert(sizeof(ble_session) <= BLE_HID_SESSION_STORAGE_SIZE, "BLE_HID_SESSION_STORAGE_SIZE too small for ble_session");
static_assert(alignof(ble_session) <= 8, "ble_session alignment exceeds peer storage alignment");

static ble_session *peerSession(peer_t *peer)
{
    return std::launder(reinterpret_cast<ble_session *>(peer->hidSessionStorage));
}

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
    ::bt_conn *conn = static_cast<const bluetooth::hid_over_gatt::session_params &>(params).conn;
    int8_t peerId = GetPeerIdByConn(conn);
    if (peerId < PeerIdFirstHost || peerId > PeerIdLastHost) {
        // A HOGP session should only ever start for a connected host peer 
        printk("ble_app::start: no host peer for conn (peerId %d)\n", peerId);
        assert(false);
        static ble_session fallback;
        return fallback;
    }
    peer_t *peer = &Peers[peerId];

    // Defensive: destroy a stale session still occupying this peer's storage.
    if (peer->hidSessionActive) {
        peerSession(peer)->~ble_session();
        peer->hidSessionActive = false;
    }

    ble_session *sess = new (peer->hidSessionStorage) ble_session();
    peer->hidSessionActive = true;
    return *sess;
}

void ble_app::stop(hid::session &sess)
{
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        peer_t *peer = &Peers[peerId];
        if (!peer->hidSessionActive) {
            continue;
        }
        ble_session *ps = peerSession(peer);
        if (static_cast<hid::session *>(ps) == &sess) {
            ps->~ble_session();
            peer->hidSessionActive = false;
            return;
        }
    }
}

extern "C" int HOGP_HealthCheck()
{
    int activeSessions = 0;
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        peer_t *peer = &Peers[peerId];
        if (!peer->hidSessionActive) {
            continue;
        }
        activeSessions++;
        if (peer->conn == nullptr) {
            printk("HOGP HealthCheck: peer %s has an active session but no conn\n", peer->name);
            return -2;
        }
        struct bt_conn_info info;
        int err = bt_conn_get_info(peer->conn, &info);
        if (err) {
            printk("HOGP HealthCheck: peer %s has INVALID conn pointer (err %d)\n", peer->name, err);
            return -3;
        }
    }

    printk("HOGP HealthCheck: OK (%d active HID session(s))\n", activeSessions);
    return 0;
}
