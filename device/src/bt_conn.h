#ifndef __BT_CONN_H__
#define __BT_CONN_H__

// Includes:

    #include <zephyr/bluetooth/bluetooth.h>
    #include <stdint.h>
    #include "connections.h"

// Macros:

    #define PeerNameMaxLength 8

    // please edit this in prj.conf
    #define PERIPHERAL_CONNECTION_COUNT CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT

    #define ACTUAL_PERIPHERAL_CONNECTION_COUNT (MIN(PERIPHERAL_CONNECTION_COUNT, Cfg.Bt_MaxPeripheralConnections))

    #define PeerIdUnknown 0
    #define PeerIdLeft 1
    #define PeerIdFirst 1
    #define PeerIdRight 2
    #define PeerIdFirstHost (PeerIdRight+1)
    #define PeerIdLastHost (PeerIdFirstHost+PERIPHERAL_CONNECTION_COUNT-1)
    #define PeerCount (PeerIdLastHost+1)


    #define BLE_ADDR_LEN 6
    #define BLE_KEY_LEN 16


// Typedefs:

    typedef enum {
        PeerType_Left,
        PeerType_Right,
        PeerType_Dongle,
        PeerType_BleHid,
    } peer_type_t;

    typedef struct {
        uint8_t id;
        uint8_t connectionId;
        char name[PeerNameMaxLength + 1];
        bt_addr_le_t addr;
        struct bt_conn* conn;
        uint32_t lastSwitchover;
    } peer_t;

typedef enum {
    LatencyMode_NUS,
    LatencyMode_BleHid,
} latency_mode_t;

// Variables:

    extern peer_t Peers[];
    extern bool Bt_NewPairedDevice;

// Functions:

    int8_t GetPeerIdByConn(const struct bt_conn *conn);
    char *GetPeerStringByAddr(const bt_addr_le_t *addr);
    char *GetPeerStringByConn(const struct bt_conn *conn);
    char *GetPeerStringByConnId(uint8_t connectionId);
    char* GetAddrString(const bt_addr_le_t *addr);
    void num_comp_reply(int passkey);

    void BtConn_Init(void);
    void BtConn_DisconnectAll();
    void BtConn_DisconnectOne(connection_id_t connectionId);

    void BtConn_ReserveConnections();
    void Bt_SetConnectionConfigured(struct bt_conn* conn);
    uint8_t BtConn_UnusedPeripheralConnectionCount();

    void BtConn_ListCurrentConnections();
    void BtConn_ListAllBonds();
    void Bt_SetEnabled(bool enabled);
    void BtConn_MakeSpaceForHid();

    uint8_t BtConn_ConnectedHidCount(const bt_addr_le_t* excludeAddr);
    void BtConn_KickHid(void);

    static inline bool BtAddrEq(const bt_addr_le_t *a, const bt_addr_le_t *b) {
        return 0 == memcmp(a->a.val, b->a.val, sizeof(a->a.val));
    }

#endif // __BT_CONN_H__
