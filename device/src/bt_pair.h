#ifndef __BT_PAIR_H__
#define __BT_PAIR_H__

// Includes:

    #include <zephyr/bluetooth/bluetooth.h>
    #include <stdint.h>
    #include "device.h"
    #include <stdbool.h>

// Macros:

// Typedefs:

    #define PAIRING_TIMEOUT 20000

// Functions:

    struct bt_le_oob* BtPair_GetLocalOob();
    struct bt_le_oob* BtPair_GetRemoteOob();
    void BtPair_SetRemoteOob(const struct bt_le_oob* oob);
    void BtPair_PairCentral();
    void BtPair_PairPeripheral();
    void BtPair_EndPairing(bool success, const char* msg);
    void BtPair_Unpair(const bt_addr_le_t addr);
    bool BtPair_IsDeviceBonded(const bt_addr_le_t *addr);
    void BtManager_EnterPairingMode();
    void BtPair_ClearUnknownBonds();
    void BtPair_UnpairAllNonLR();

// Variables

    extern bool BtPair_OobPairingInProgress;
    extern bool BtPair_LastPairingSucceeded;

#endif // __BT_PAIR_H__
