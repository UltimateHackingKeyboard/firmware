#ifndef __BT_PAIR_H__
#define __BT_PAIR_H__

// Includes:

    #include <zephyr/bluetooth/bluetooth.h>
    #include <stdint.h>
    #include "device.h"
    #include <stdbool.h>
    #include "bt_defs.h"

// Macros:

// Typedefs:

    #define PAIRING_TIMEOUT 20000
    #define USER_PAIRING_TIMEOUT 120000

// Functions:

    struct bt_le_oob* BtPair_GetLocalOob();
    struct bt_le_oob* BtPair_GetRemoteOob();
    void BtPair_SetRemoteOob(const struct bt_le_oob* oob);
    void BtPair_PairCentral();
    void BtPair_PairPeripheral();
    void BtPair_EndPairing(bool success, const char* msg);
    void BtPair_Unpair(const bt_addr_le_t addr);
    bool BtPair_IsDeviceBonded(const bt_addr_le_t *addr);
    void BtManager_EnterMode(pairing_mode_t mode, bool toggle);
    void BtPair_ClearUnknownBonds();
    void BtPair_UnpairAllNonLR();

// Variables

    extern pairing_mode_t BtPair_PairingMode;
    extern bool BtPair_LastOobPairingSucceeded;
    extern bool BtPair_PairingAsCentral;

#endif // __BT_PAIR_H__
