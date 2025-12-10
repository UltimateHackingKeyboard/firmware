#include "device_state.h"
#include "bt_conn.h"
#include "bt_manager.h"
#include "connections.h"
#include "device.h"
#include "keyboard/uart_bridge.h"
#include "keyboard/oled/widgets/widgets.h"
#include "slave_drivers/uhk_module_driver.h"
#include "state_sync.h"
#include "shared/slave_protocol.h"
#include "event_scheduler.h"
#include "power_mode.h"
#include "dongle_leds.h"

static connection_id_t targetsConnection[ConnectionTarget_Count] = {};

bool DongleStandby = false;

// void handleStateTransition(connection_id_t remoteId, bool connected) {
void handleStateTransition(connection_target_t remote, connection_id_t connectionId, bool connected) {

        connection_target_t local = Connections_DeviceToTarget(DEVICE_ID);
        switch (local) {
            case ConnectionTarget_Left:
                if (remote == ConnectionTarget_Right && connected) {
                    StateSync_ResetRightLeftLink(true);
                }
                break;
            case ConnectionTarget_Right:
                switch (remote) {
                    case ConnectionTarget_Left:
#if DEVICE_HAS_OLED
                        Widget_Refresh(&StatusWidget);
#endif
                        if (connected) {
                            StateSync_ResetRightLeftLink(true);
                            ModuleConnectionStates[UhkModuleDriverId_LeftKeyboardHalf].moduleId = ModuleId_LeftKeyboardHalf;
                        } else {
                            ModuleConnectionStates[UhkModuleDriverId_LeftKeyboardHalf].moduleId = 0;
                            ModuleConnectionStates[UhkModuleDriverId_LeftModule].moduleId = 0;

                            bool changed = false;
                            for (uint8_t slotId = SlotId_LeftKeyboardHalf; slotId <= SlotId_LeftModule; slotId++) {
                                for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
                                    if (KeyStates[slotId][keyId].hardwareSwitchState) {
                                        KeyStates[slotId][keyId].hardwareSwitchState = false;
                                        changed = true;
                                    }
                                }
                            }

                            if (changed) {
                                EventVector_Set(EventVector_StateMatrix);
                                EventVector_WakeMain();
                            }

                            BtManager_StartScanningAndAdvertisingAsync("StartScanningAndAdvertisingAsync in handleStateTransition - left was disconnected");
                        }
                        break;
                    case ConnectionTarget_Host:
                        if (HostConnection(connectionId)->type == HostConnectionType_Dongle && connected) {
                            StateSync_ResetRightDongleLink(true);
                        }
#if DEVICE_HAS_OLED
                        Widget_Refresh(&TargetWidget);
#endif
                        EventScheduler_Reschedule(Timer_GetCurrentTime() + POWER_MODE_UPDATE_DELAY, EventSchedulerEvent_PowerMode, "update sleep mode from device state");
                        break;
                    default:
                        break;
                }
                break;
            case ConnectionTarget_Host:
                // Dongle actually
                if (remote == ConnectionTarget_Right && connected) {
                    StateSync_ResetRightDongleLink(true);
                }
                DongleLeds_Update();
                break;
            default:
                break;
        }
}

#define RETURN_IF_CONNECTED(CONNECTION_ID) if (Connections[CONNECTION_ID].state == ConnectionState_Ready) { return CONNECTION_ID; }

static connection_id_t findPreferredConnection(connection_target_t target) {
    switch (target) {
        case ConnectionTarget_Left:
            if (DEVICE_IS_UHK80_LEFT) {
                return true;
            }
            RETURN_IF_CONNECTED(ConnectionId_UartLeft);
            RETURN_IF_CONNECTED(ConnectionId_NusServerLeft);
            return ConnectionId_Invalid;
        case ConnectionTarget_Right:
            if (DEVICE_IS_UHK80_RIGHT) {
                return true;
            }
            RETURN_IF_CONNECTED(ConnectionId_UartRight);
            RETURN_IF_CONNECTED(ConnectionId_NusServerRight);
            RETURN_IF_CONNECTED(ConnectionId_NusClientRight);
            return ConnectionId_Invalid;
        case ConnectionTarget_Host:
            return ActiveHostConnectionId;
        default:
            return ConnectionId_Invalid;
    }
}

void DeviceState_Update(uint8_t connectionTarget) {
    connection_id_t oldConnection = targetsConnection[connectionTarget];
    connection_id_t newConnection = findPreferredConnection(connectionTarget);

    if (oldConnection != newConnection) {
        targetsConnection[connectionTarget] = newConnection;

        handleStateTransition(connectionTarget, newConnection, Connections[newConnection].state == ConnectionState_Ready);
    }
}

bool DeviceState_IsDeviceConnected(device_id_t deviceId) {
    if (deviceId == DEVICE_ID) {
        return true;
    }

    connection_target_t target = Connections_DeviceToTarget(deviceId);
    connection_id_t connectionId = findPreferredConnection(target);

    if (deviceId == DeviceId_Uhk_Dongle) {
        host_connection_t* hostConnection = HostConnection(connectionId);
        if (hostConnection == NULL || hostConnection->type != HostConnectionType_Dongle) {
            return false;
        }
    }

    return connectionId != ConnectionId_Invalid && Connections[connectionId].state == ConnectionState_Ready;
}

bool DeviceState_IsTargetConnected(uint8_t target) {

    if (target == Connections_DeviceToTarget(DEVICE_ID)) {
        return true;
    } else {
        connection_id_t connectionId = findPreferredConnection(target);
        return connectionId != ConnectionId_Invalid && Connections[connectionId].state == ConnectionState_Ready;
    }
}

