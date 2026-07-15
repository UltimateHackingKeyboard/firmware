#ifndef __LINK_PROTOCOL_H__
#define __LINK_PROTOCOL_H__

// Includes:

#include <stdbool.h>
#include <stdint.h>

// Macros:

// Maximum deserialized message length (header + payload) that fits in a single
// BLE notification / UART bridge frame. We deliberately keep this well below the
// DLE maximum (244 with a 251-byte ACL buffer) to save RAM (messenger-queue
// regions, nus/uart buffers) and to shorten BLE packets for lower latency.
// 4 bytes reserved for L2CAP header
// 3 bytes reserved for ATT header
// https://devzone.nordicsemi.com/f/nordic-q-a/111900/maximum-nus-packet-payload-with-ble-data-length-extensio
// Should equal `CONFIG_BT_BUF_ACL_RX_SIZE - L2CAP_HEADER_SIZE - ATT_HEADER_SIZE`
// (i.e. 135 - 4 - 3), otherwise something is wrong.
#define MAX_LINK_PACKET_LENGTH 128

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
