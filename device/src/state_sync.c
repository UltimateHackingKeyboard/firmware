#include "state_sync.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "device.h"
#include "legacy/key_action.h"
#include "legacy/layer.h"
#include "legacy/slave_drivers/is31fl3xxx_driver.h"
#include "legacy/slot.h"
#include "messenger.h"
#include "shared/attributes.h"
#include "legacy/ledmap.h"
#include "legacy/layer_switcher.h"
#include "legacy/config_manager.h"
#include "keyboard/leds.h"
#include "legacy/keymap.h"
#include "legacy/key_action.h"

typedef enum {
    LayerState_Good,
    LayerState_NeedsClearing,
    LayerState_NeedsUpdate,
} layer_state_t;

typedef struct {
    layer_state_t layerState[LayerId_Count];
    bool activeLayerNeedsUpdate;
    bool backlightNeedsUpdate;
} syncable_state_right_t;

typedef enum {
    SyncCommand_ClearLayer = 1,
    SyncCommand_SyncLayer,
    SyncCommand_SyncActiveLayer,
    SyncCommand_SyncBacklight,
} sync_command_type_t;

syncable_state_right_t syncState;

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

#if DEVICE_IS_UHK80_RIGHT
static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;
#endif

static k_tid_t stateSyncThreadId = 0;

#define STATE_SYNC_LOG(fmt, ...) //printk(fmt, __VA_ARGS__)

typedef struct {
    uint8_t type : 5;
    uint8_t scancodePresent : 1;
    uint8_t modifierPresent : 1;
    uint8_t colorOverriden : 1;
    rgb_t color;
} ATTR_PACKED sync_command_action_t;

typedef struct {
    sync_command_type_t command;
    layer_id_t layerId;
    sync_command_action_t action[MAX_KEY_COUNT_PER_MODULE];
} layer_sync_command_t;

static void sendClearLayer(layer_id_t layerId) {
    uint8_t data[] = { SyncCommand_ClearLayer, layerId };
    Messenger_Send(DeviceId_Uhk80_Left, MessageId_StateSync, data, sizeof(data));
    STATE_SYNC_LOG("State sync sending clear layer %i\n", layerId);
}

static void sendSyncLayer(layer_id_t layerId) {
    layer_sync_command_t command;
    command.command = SyncCommand_SyncLayer;
    command.layerId = layerId;
    for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
        key_action_t* action = &CurrentKeymap[layerId][SlotId_LeftKeyboardHalf][keyId];
        command.action[keyId].type = action->type;
        command.action[keyId].color = action->color;
        command.action[keyId].colorOverriden = action->colorOverridden;
        command.action[keyId].modifierPresent = action->keystroke.modifiers != 0;
        command.action[keyId].scancodePresent = action->keystroke.scancode != 0;
    }
    Messenger_Send(DeviceId_Uhk80_Left, MessageId_StateSync, (const uint8_t*)&command, sizeof(command));
    STATE_SYNC_LOG("State sync sending sync layer %i\n", layerId);
}

void StateSync_LeftReceiveStateUpdate(const uint8_t* data, uint16_t len) {
    data++;
    sync_command_type_t command = *(data++);

    switch (command) {
        case SyncCommand_ClearLayer: {
                uint8_t layerId = *(data++);
                STATE_SYNC_LOG("Clearing layer %i\n", layerId);
                memset(&CurrentKeymap[layerId][SlotId_LeftKeyboardHalf][0], 0, sizeof(key_action_t)*MAX_KEY_COUNT_PER_MODULE);
                if (layerId == ActiveLayer) {
                    Ledmap_UpdateBacklightLeds();
                }
            }
            break;
        case SyncCommand_SyncLayer: {
                uint8_t layerId = *(data++);
                STATE_SYNC_LOG("Setting layer %i\n", layerId);
                sync_command_action_t* actions = (sync_command_action_t*)data;
                for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
                    key_action_t* action = &CurrentKeymap[layerId][SlotId_LeftKeyboardHalf][keyId];
                    action->color = actions[keyId].color;
                    action->colorOverridden = actions[keyId].colorOverriden;
                    action->type = actions[keyId].type;
                    action->keystroke.modifiers = actions[keyId].modifierPresent ? 0xff : 0;
                    action->keystroke.scancode = actions[keyId].scancodePresent ? 0xff : 0;
                }
                if (layerId == ActiveLayer) {
                    Ledmap_UpdateBacklightLeds();
                }
            }
            break;
        case SyncCommand_SyncActiveLayer: {
                ActiveLayer = *(data++);
                Ledmap_UpdateBacklightLeds();
            }
            break;
        case SyncCommand_SyncBacklight: {
                Cfg.BacklightingMode = *(data++);
                KeyBacklightBrightness = *(data++);
                Cfg.LedMap_ConstantRGB.red = *(data++);
                Cfg.LedMap_ConstantRGB.green = *(data++);
                Cfg.LedMap_ConstantRGB.blue = *(data++);
                Ledmap_UpdateBacklightLeds();
            }
            break;
        default:
            printk("Unrecognized sync command %i\n", command);
            break;
    }
}

void StateSync_RightReceiveStateUpdate(const uint8_t* data, uint16_t len) {
    message_id_t msgId = *(data++);
    sync_command_type_t command = *(data++);

    switch (command) {
        default:
            printk("Unrecognized sync command [%i, %i, ...]\n", msgId, command);
            break;
    }
}


static bool transmitStateUpdate() {
    if (KeyBacklightBrightness != 0 && Cfg.BacklightingMode != BacklightingMode_ConstantRGB) {
        for (uint8_t layerId = 0; layerId < LayerId_Count; layerId++) {
            if (syncState.layerState[layerId] != LayerState_Good) {
                layer_state_t oldState = syncState.layerState[layerId];
                syncState.layerState[layerId] = LayerState_Good;
                if (oldState == LayerState_NeedsClearing) {
                    sendClearLayer(layerId);
                } else {
                    sendSyncLayer(layerId);
                }
                return false;
            }
        }

        if (syncState.activeLayerNeedsUpdate) {
            syncState.activeLayerNeedsUpdate = false;
            uint8_t data[] = { SyncCommand_SyncActiveLayer, ActiveLayer };
            Messenger_Send(DeviceId_Uhk80_Left, MessageId_StateSync, data, sizeof(data));
            STATE_SYNC_LOG("State sync sending sync active layer\n");
            return false;
        }
    }

    if (syncState.backlightNeedsUpdate) {
        syncState.backlightNeedsUpdate = false;
        uint8_t data[] = {
            SyncCommand_SyncBacklight,
            Cfg.BacklightingMode,
            KeyBacklightBrightness,
            Cfg.LedMap_ConstantRGB.red,
            Cfg.LedMap_ConstantRGB.green,
            Cfg.LedMap_ConstantRGB.blue,
        };
        Messenger_Send(DeviceId_Uhk80_Left, MessageId_StateSync, data, sizeof(data));
        STATE_SYNC_LOG("State sync sending sync backlight\n");
        return false;
    }
    return true;
}

static ATTR_UNUSED void stateSynceUpdaterRight() {
    while(true) {
        if (transmitStateUpdate()) {
            k_sleep(K_FOREVER);
        }
    }
}

void StateSync_UpdateActiveLayer() {
    syncState.activeLayerNeedsUpdate = true;
    k_wakeup(stateSyncThreadId);
}

void StateSync_UpdateLayer(layer_id_t layer, bool justClear) {
    syncState.layerState[layer] = MAX(syncState.layerState[layer], justClear ? LayerState_NeedsClearing : LayerState_NeedsUpdate);
    k_wakeup(stateSyncThreadId);
}

void StateSync_UpdateBacklight() {
    syncState.backlightNeedsUpdate |= true;
    k_wakeup(stateSyncThreadId);
}

void StateSync_Init() {
#if DEVICE_IS_UHK80_RIGHT
        stateSyncThreadId = k_thread_create(
                &thread_data, stack_area,
                K_THREAD_STACK_SIZEOF(stack_area),
                stateSynceUpdaterRight,
                NULL, NULL, NULL,
                THREAD_PRIORITY, 0, K_NO_WAIT
                );
        k_thread_name_set(&thread_data, "state_sync_updater");
#endif
}
