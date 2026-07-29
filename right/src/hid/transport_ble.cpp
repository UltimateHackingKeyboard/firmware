#include "ble_app.hpp"
#include <bluetooth/hid_over_gatt.hpp>
#include <optional>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
extern "C" {
#include "bt_conn.h"
}

using namespace magic_enum::bitwise_operators;

// One HOGP session per host peer. Kept here rather than in peer_t so that the C
// header stays free of C++ layout requirements, and so the left/dongle builds -
// which never instantiate a ble_session - don't carry the storage.
static std::optional<ble_session> hidSessions[PeerIdLastHost - PeerIdFirstHost + 1];

static std::optional<ble_session> &sessionSlot(int8_t peerId)
{
    return hidSessions[peerId - PeerIdFirstHost];
}

static auto &hog_service()
{
    using namespace magic_enum::bitwise_operators;
    using namespace bluetooth::hid_over_gatt;

    // TODO: set values as needed
    const auto sec_lvl = bluetooth::security::L4_LE_SC;
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

hid::session &ble_app::start(const hid::session::params &params)
{
    ::bt_conn *conn = static_cast<const bluetooth::hid_over_gatt::session_params &>(params).conn;
    int8_t peerId = GetPeerIdByConn(conn);
    if (peerId < PeerIdFirstHost || peerId > PeerIdLastHost) {
        // A HOGP session should only ever start for a connected host peer
        printk("ble_app::start: no host peer for conn (peerId %d)\n", peerId);
        assert(false && "ble_app::start: no host peer for conn");
        static ble_session fallback{params};
        return fallback;
    }
    peer_t *peer = &Peers[peerId];

    // destroy any stale session left in this slot
    auto &slot = sessionSlot(peerId);
    slot.reset();
    ble_session *sess = &slot.emplace(params);

    // Mirror mouse_app::start: initialise the scroll multiplier from the new session.
    mouse_resolution_changed_callback(*sess, sess->resolution_report());

    if (peer->connectionId != ConnectionId_Invalid) {
        printk("ble_app::start: marking BtHid connection %d (peer %d) ready\n", peer->connectionId,
            peerId);
        Connections_SetStateAsync((connection_id_t)peer->connectionId, ConnectionState_Ready);
    } else {
        printk("ble_app::start: peer %d has invalid connectionId, cannot mark ready\n", peerId);
    }
    return *sess;
}

void ble_app::stop(hid::session &sess)
{
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        auto &slot = sessionSlot(peerId);
        if (!slot.has_value()) {
            continue;
        }
        if (static_cast<hid::session *>(&*slot) == &sess) {
            peer_t *peer = &Peers[peerId];
            // On a real disconnect bt_conn.c may already have cleared connectionId.
            if (peer->connectionId != ConnectionId_Invalid) {
                printk("ble_app::stop: marking BtHid connection %d (peer %d) disconnected\n",
                    peer->connectionId, peerId);
                Connections_SetStateAsync(
                    (connection_id_t)peer->connectionId, ConnectionState_Disconnected);
            } else {
                printk("ble_app::stop: peer %d has invalid connectionId\n", peerId);
            }
            slot.reset();
            return;
        }
    }
}

extern "C" void HOGP_Register()
{
    // Forces the function-local static HOGP service (and its bt_gatt_service_static
    // entry) to be constructed so bt_enable() can register it.
    (void)hog_service();
}

extern "C" int HOGP_HealthCheck()
{
    int activeSessions = 0;
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        peer_t *peer = &Peers[peerId];
        if (!sessionSlot(peerId).has_value()) {
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
            printk(
                "HOGP HealthCheck: peer %s has INVALID conn pointer (err %d)\n", peer->name, err);
            return -3;
        }
    }

    printk("HOGP HealthCheck: OK (%d active HID session(s))\n", activeSessions);
    return 0;
}
