#include "device_state.h"
#include "bt_conn.h"
#include "device.h"
#include "keyboard/uart.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/state_sync.h"

static connection_type_t isConnected[ConnectionId_Count] = {};

bool DeviceState_IsDeviceConnected(device_id_t deviceId) {
    return deviceId == DEVICE_ID || isConnected[deviceId - DEVICE_STATE_FIRST_DEVICE] != ConnectionType_None;
}

bool DeviceState_IsConnected(connection_id_t connectionId) {
    return isConnected[connectionId] != ConnectionType_None;
}

void handleStateTransition(connection_id_t remoteId, bool isConnected) {
        switch (DEVICE_ID) {
            case DeviceId_Uhk80_Left:
                if (remoteId == ConnectionId_Right && isConnected) {
                    StateSync_Reset();
                }
                break;
            case DeviceId_Uhk80_Right:
                switch (remoteId) {
                    case ConnectionId_Left:
                        if (isConnected) {
                            Widget_Refresh(&StatusWidget);
                            StateSync_Reset();
                        }
                        break;
                    case ConnectionId_Dongle:
                    case ConnectionId_UsbHid:
                    case ConnectionId_BluetoothHid:
                        Widget_Refresh(&TargetWidget);
                        break;
                    default:
                        break;
                }
                break;
            case DeviceId_Uhk_Dongle:
                break;
            default:
                break;
        }
}

void DeviceState_SetConnection(connection_id_t connection, connection_type_t type) {
    if (isConnected[connection] != type) {
        isConnected[connection] = type;
        handleStateTransition(connection, isConnected[connection] != ConnectionType_None);
    }
}

static connection_id_t deviceToConnection(device_id_t deviceId) {
    switch (deviceId) {
        case DeviceId_Uhk80_Left:
            return ConnectionId_Left;
        case DeviceId_Uhk80_Right:
            return ConnectionId_Right;
        case DeviceId_Uhk_Dongle:
            return ConnectionId_Dongle;
        default:
            return ConnectionId_Invalid;
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

        connection_id_t conId = deviceToConnection(devId);
        connection_type_t oldIsConnected = isConnected[conId];
        isConnected[conId] = newIsConnected;
        if (newIsConnected != oldIsConnected) {
            handleStateTransition(conId, newIsConnected);
        }
    }
}

