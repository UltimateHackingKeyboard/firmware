#include "device_state.h"
#include "bt_conn.h"
#include "device.h"
#include "keyboard/uart.h"
#include "keyboard/oled/widgets/widgets.h"
#include "legacy/slave_drivers/uhk_module_driver.h"
#include "state_sync.h"
#include "shared/slave_protocol.h"
#include "legacy/event_scheduler.h"
#include "legacy/power_mode.h"
#include "dongle_leds.h"

static connection_type_t isConnected[ConnectionId_Count] = {};

bool DeviceState_IsDeviceConnected(device_id_t deviceId) {
    return deviceId == DEVICE_ID || isConnected[deviceId - DEVICE_STATE_FIRST_DEVICE] != ConnectionType_None;
}

bool DeviceState_IsConnected(connection_id_t connectionId) {
    return isConnected[connectionId] != ConnectionType_None;
}

void handleStateTransition(connection_id_t remoteId, bool connected) {
        switch (DEVICE_ID) {
            case DeviceId_Uhk80_Left:
                if (remoteId == ConnectionId_Right && connected) {
                    StateSync_ResetRightLeftLink(true);
                }
                break;
            case DeviceId_Uhk80_Right:
                switch (remoteId) {
                    case ConnectionId_Left:
                        Widget_Refresh(&StatusWidget);
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
                        }
                        break;
                    case ConnectionId_Dongle:
                        if (connected) {
                            StateSync_ResetRightDongleLink(true);
                        }
                        Widget_Refresh(&TargetWidget);
                        EventScheduler_Reschedule(CurrentTime + POWER_MODE_UPDATE_DELAY, EventSchedulerEvent_PowerMode, "update sleep mode from device state dongle");
                        break;
                    case ConnectionId_UsbHid:
                        Widget_Refresh(&TargetWidget);
                        break;
                    case ConnectionId_BluetoothHid:
                        Widget_Refresh(&TargetWidget);
                        EventScheduler_Reschedule(CurrentTime + POWER_MODE_UPDATE_DELAY, EventSchedulerEvent_PowerMode, "update sleep mode from device state bluetooth hid");
                        break;
                    default:
                        break;
                }
                break;
            case DeviceId_Uhk_Dongle:
                if (remoteId == ConnectionId_Right && connected) {
                    StateSync_ResetRightDongleLink(true);
                }
                DongleLeds_Update();
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

