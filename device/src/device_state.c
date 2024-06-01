#include "device_state.h"
#include "bt_conn.h"
#include "device.h"
#include "keyboard/uart.h"
#include "keyboard/oled/widgets/widgets.h"
#include "state_sync.h"

typedef enum {
    ConnectionType_None,
    ConnectionType_Uart,
    ConnectionType_Bt,
} connection_type_t;

static connection_type_t isConnected[DEVICE_STATE_LAST_DEVICE - DEVICE_STATE_FIRST_DEVICE + 1] = {};

bool DeviceState_IsConnected(device_id_t deviceId) {
    return isConnected[deviceId - DEVICE_STATE_FIRST_DEVICE] != ConnectionType_None;
}

void handleStateTransition(device_id_t remoteId, bool isConnected) {
    if (isConnected) {
        switch (DEVICE_ID) {
            case DeviceId_Uhk80_Left:
                break;
            case DeviceId_Uhk80_Right:
                if (remoteId == DeviceId_Uhk80_Left) {
                    TextWidget_Refresh(&StatusWidget);
                    StateSync_ResetState();
                }
                break;
            case DeviceId_Uhk_Dongle:
                break;
            default:
                break;
        }
    }
}

void DeviceState_TriggerUpdate() {
    for (uint8_t devId = DEVICE_STATE_FIRST_DEVICE; devId <= DEVICE_STATE_LAST_DEVICE; devId++) {
        connection_type_t newIsConnected = ConnectionType_None;

        if (Bt_DeviceIsConnected(devId)) {
            newIsConnected = ConnectionType_Bt;
        }

        if (devId == DeviceId_Uhk80_Left && DEVICE_ID == DeviceId_Uhk80_Right && Uart_IsConnected()) {
            newIsConnected = ConnectionType_Uart;
        }

        if (devId == DeviceId_Uhk80_Right && DEVICE_ID == DeviceId_Uhk80_Left && Uart_IsConnected()) {
            newIsConnected = ConnectionType_Uart;
        }

        connection_type_t oldIsConnected = isConnected[devId - DEVICE_STATE_FIRST_DEVICE];
        isConnected[devId - DEVICE_STATE_FIRST_DEVICE] = newIsConnected;
        if (newIsConnected != oldIsConnected) {
            handleStateTransition(devId, newIsConnected);
        }
    }
}

