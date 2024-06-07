#ifndef __LINK_PROTOCOL_H__
#define __LINK_PROTOCOL_H__

// Includes:

#include <stdbool.h>
#include <stdint.h>

// Macros:

// 251 = maximum BLE packet length with data length extension
// 4 bytes reserved for L2CAP header
// 3 bytes reserved for ATT header
// https://devzone.nordicsemi.com/f/nordic-q-a/111900/maximum-nus-packet-payload-with-ble-data-length-extensio
#define MAX_LINK_PACKET_LENGTH 244

// Typedefs:

typedef enum {
    SyncablePropertyId_UserConfiguration,
    SyncablePropertyId_CurrentKeymapId,
    SyncablePropertyId_CurrentLayerId,
    SyncablePropertyId_KeyboardReport,
    SyncablePropertyId_MouseReport,
    SyncablePropertyId_GamepadReport,
    SyncablePropertyId_LeftHalfKeyStates,
    SyncablePropertyId_LeftModuleKeyStates,
    SyncablePropertyId_LeftBatteryPercentage,
    SyncablePropertyId_LeftBatteryChargingState,
    SyncablePropertyId_ControlsReport,
} syncable_property_id_t;

typedef struct {
    syncable_property_id_t id;
    bool dirty;
} syncable_property_t;


#endif // __LINK_PROTOCOL_H__
