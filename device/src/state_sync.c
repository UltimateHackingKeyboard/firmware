#include "state_sync.h"
#include "bt_conn.h"
#include "connections.h"
#include "device.h"
#include "device_state.h"
#include "event_scheduler.h"
#include "keyboard/charger.h"
#include "keyboard/key_scanner.h"
#include "keyboard/oled/widgets/widgets.h"
#include "config_manager.h"
#include "config_parser/config_globals.h"
#include "debug.h"
#include "event_scheduler.h"
#include "keymap.h"
#include "led_manager.h"
#include "ledmap.h"
#include "module.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slot.h"
#include "str_utils.h"
#include "stubs.h"
#include "utils.h"
#include "messenger.h"
#include "state_sync.h"
#include "usb/usb_compatibility.h"
#include <stdint.h>
#include <zephyr/kernel.h>
#include "peripherals/merge_sensor.h"
#include "power_mode.h"
#include "test_switches.h"
#include "dongle_leds.h"
#include "logger.h"
#include "versioning.h"

#define WAKE(TID) if (TID != 0) { k_wakeup(TID); }

#define STATE_SYNC_SEND_DELAY 1

#define THREAD_STACK_SIZE 2000
#define THREAD_PRIORITY 5
static K_THREAD_STACK_DEFINE(stack_area_left, THREAD_STACK_SIZE);
static struct k_thread thread_data_left;
static k_tid_t stateSyncThreadLeftId = 0;

static K_THREAD_STACK_DEFINE(stack_area_dongle, THREAD_STACK_SIZE);
static struct k_thread thread_data_dongle;
static k_tid_t stateSyncThreadDongleId = 0;

sync_generic_half_state_t SyncLeftHalfState;
sync_generic_half_state_t SyncRightHalfState;

scroll_multipliers_t DongleScrollMultipliers = {1, 1};
version_t DongleProtocolVersion = {0, 0, 0};

uint16_t StateSync_LeftResetCounter = 0;
uint16_t StateSync_DongleResetCounter = 0;

static void wake(k_tid_t tid) {
    if (tid != 0) {
        k_wakeup(tid);
        // if (DEBUG_MODE) {
        //     LogU("StateSync woke up %p\n", tid);
        // }
    } else {
        printk("Skipping wake up, tid is 0");
    }
}

static void receiveProperty(device_id_t src, state_sync_prop_id_t property, const uint8_t *data, uint8_t len);

#define DEFAULT_LAYER_PROP(NAME)                                                                   \
    [StateSyncPropertyId_##NAME] = {.leftData = NULL,                                              \
        .rightData = NULL,                                                                         \
        .dongleData = NULL,                                                                        \
        .len = 0,                                                                                  \
        .direction = SyncDirection_RightToLeft,                                                    \
        .dirtyState = DirtyState_Clean,                                                            \
        .defaultDirty = DirtyState_Clean,                                                          \
        .name = #NAME}

#define SIMPLE(NAME, DIRECTION, DEFAULT_DIRTY, DATA)                                               \
    [StateSyncPropertyId_##NAME] = {.leftData = DATA,                                              \
        .rightData = DATA,                                                                         \
        .dongleData = DATA,                                                                        \
        .len = sizeof(*DATA),                                                                      \
        .direction = DIRECTION,                                                    \
        .dirtyState = DEFAULT_DIRTY,                                                               \
        .defaultDirty = DEFAULT_DIRTY,                                                             \
        .name = #NAME}

#define SIMPLE_CUSTOM(NAME, DIRECTION, DEFAULT_DIRTY)                                              \
    [StateSyncPropertyId_##NAME] = {.leftData = NULL,                                              \
        .rightData = NULL,                                                                         \
        .dongleData = NULL,                                                                        \
        .len = 0,                                                                                  \
        .direction = SyncDirection_RightToLeft,                                                    \
        .dirtyState = DEFAULT_DIRTY,                                                               \
        .defaultDirty = DEFAULT_DIRTY,                                                             \
        .name = #NAME}

#define DUAL_LR(NAME, DIRECTION, DEFAULT_DIRTY, LEFT, RIGHT)                                       \
    [StateSyncPropertyId_##NAME] = {.leftData = LEFT,                                              \
        .rightData = RIGHT,                                                                        \
        .dongleData = NULL,                                                                        \
        .len = sizeof(*LEFT),                                                                      \
        .direction = SyncDirection_LeftToRight,                                                    \
        .dirtyState = DEFAULT_DIRTY,                                                               \
        .defaultDirty = DEFAULT_DIRTY,                                                             \
        .name = #NAME}

#ifdef EMPTY
    #undef EMPTY
#endif

#define EMPTY(NAME, DIRECTION, DEFAULT_DIRTY)                                                      \
    [StateSyncPropertyId_##NAME] = {.leftData = NULL,                                              \
        .rightData = NULL,                                                                         \
        .dongleData = NULL,                                                                        \
        .len = 0,                                                                                  \
        .direction = DIRECTION,                                                                    \
        .dirtyState = DEFAULT_DIRTY,                                                               \
        .defaultDirty = DEFAULT_DIRTY,                                                             \
        .name = #NAME}

#define CUSTOM(...) EMPTY(__VA_ARGS__)

static state_sync_prop_t stateSyncProps[StateSyncPropertyId_Count] = {
    CUSTOM(ZeroDummy, SyncDirection_RightToLeft, DirtyState_Clean),
    DEFAULT_LAYER_PROP(LayerActionsLayer1),
    DEFAULT_LAYER_PROP(LayerActionsLayer2),
    DEFAULT_LAYER_PROP(LayerActionsLayer3),
    DEFAULT_LAYER_PROP(LayerActionsLayer4),
    DEFAULT_LAYER_PROP(LayerActionsLayer5),
    DEFAULT_LAYER_PROP(LayerActionsLayer6),
    DEFAULT_LAYER_PROP(LayerActionsLayer7),
    DEFAULT_LAYER_PROP(LayerActionsLayer8),
    DEFAULT_LAYER_PROP(LayerActionsLayer9),
    DEFAULT_LAYER_PROP(LayerActionsLayer10),
    DEFAULT_LAYER_PROP(LayerActionsLayer11),
    DEFAULT_LAYER_PROP(LayerActionsLayer12),
    DEFAULT_LAYER_PROP(LayerActionsClear),
    SIMPLE(ActiveLayer,             SyncDirection_RightToLeft,        DirtyState_Clean,    &ActiveLayer),
    CUSTOM(Backlight,               SyncDirection_RightToLeft,        DirtyState_Clean),
    SIMPLE(ActiveKeymap,            SyncDirection_RightToLeft,        DirtyState_Clean,    &CurrentKeymapIndex),
    SIMPLE(KeyboardLedsState,       SyncDirection_DongleToRight,      DirtyState_Clean,    &KeyboardLedsState),
    DUAL_LR(Battery,                SyncDirection_LeftToRight,        DirtyState_Clean,    &SyncLeftHalfState.battery,      &SyncRightHalfState.battery),
    EMPTY(ResetRightLeftLink,       SyncDirection_RightLeftBidir,     DirtyState_Clean),
    EMPTY(ResetRightDongleLink,     SyncDirection_RightDongleBidir,   DirtyState_Clean),
    CUSTOM(ModuleStateLeftHalf,     SyncDirection_LeftToRight,        DirtyState_Clean),
    CUSTOM(ModuleStateLeftModule,   SyncDirection_LeftToRight,        DirtyState_Clean),
    EMPTY(LeftModuleDisconnected,   SyncDirection_LeftToRight,        DirtyState_Clean),
    SIMPLE(MergeSensor,             SyncDirection_LeftToRight,        DirtyState_Clean,    &MergeSensor_HalvesAreMerged),
    SIMPLE(FunctionalColors,        SyncDirection_RightToLeft,        DirtyState_Clean,    &Cfg.KeyActionColors),
    SIMPLE(PowerMode,               SyncDirection_RightToLeft,        DirtyState_Clean,    &CurrentPowerMode),
    CUSTOM(Config,                  SyncDirection_RightToLeft,        DirtyState_Clean),
    CUSTOM(SwitchTestMode,          SyncDirection_RightToLeft,        DirtyState_Clean),
    SIMPLE(DongleStandby,           SyncDirection_RightToDongle,      DirtyState_Clean,    &DongleStandby),
    SIMPLE(DongleScrollMultipliers, SyncDirection_DongleToRight,      DirtyState_Clean,    &DongleScrollMultipliers),
    CUSTOM(KeyStatesDummy,          SyncDirection_LeftToRight,        DirtyState_Clean),
    CUSTOM(DongleProtocolVersion,   SyncDirection_DongleToRight,      DirtyState_Clean),
};

static void invalidateProperty(state_sync_prop_id_t propId) {
    STATE_SYNC_LOG("<<< Invalidating property %s\n", stateSyncProps[propId].name);
    if (StateSyncPropertyId_LayerActionFirst <= propId &&
        propId <= StateSyncPropertyId_LayerActionLast) {
        stateSyncProps[propId].dirtyState = stateSyncProps[propId].defaultDirty;
    } else {
        stateSyncProps[propId].dirtyState = DirtyState_NeedsUpdate;
    }
    bool isRightLeftDevice =
        (DEVICE_ID == DeviceId_Uhk80_Left || DEVICE_ID == DeviceId_Uhk80_Right);
    bool isRightLeftLink = (stateSyncProps[propId].direction &
                            (SyncDirection_RightToLeft | SyncDirection_LeftToRight));
    if (isRightLeftLink && isRightLeftDevice) {
        wake(stateSyncThreadLeftId);
    }
    bool isRightDongleDevice =
        (DEVICE_ID == DeviceId_Uhk80_Right || DEVICE_ID == DeviceId_Uhk_Dongle);
    bool isRightDongleLink = (stateSyncProps[propId].direction &
                              (SyncDirection_DongleToRight | SyncDirection_RightToDongle));
    if (isRightDongleLink && isRightDongleDevice) {
        wake(stateSyncThreadDongleId);
    }
}

void StateSync_UpdateProperty(state_sync_prop_id_t propId, void *data) {
    state_sync_prop_t *prop = &stateSyncProps[propId];

    if (DEVICE_ID == DeviceId_Uhk80_Left && prop->leftData && data) {
        memcpy(prop->leftData, data, prop->len);
    }

    if (DEVICE_ID == DeviceId_Uhk80_Right && prop->rightData && data) {
        memcpy(prop->rightData, data, prop->len);
    }

    if (DEVICE_ID == DeviceId_Uhk_Dongle && prop->dongleData && data) {
        memcpy(prop->dongleData, data, prop->len);
    }

    sync_direction_t srcMask, dstMask;
    switch (DEVICE_ID) {
    case DeviceId_Uhk80_Left:
        srcMask = SyncDirection_LeftToRight;
        dstMask = SyncDirection_RightToLeft;
        break;
    case DeviceId_Uhk80_Right:
        srcMask = SyncDirection_RightToLeft | SyncDirection_RightToDongle;
        dstMask = SyncDirection_LeftToRight | SyncDirection_DongleToRight;
        break;
    case DeviceId_Uhk_Dongle:
        srcMask = SyncDirection_DongleToRight;
        dstMask = SyncDirection_RightToDongle;
        break;
    }

    if (prop->direction & srcMask) {
        invalidateProperty(propId);
    }
    if (prop->direction & (srcMask | dstMask)) {
        receiveProperty(DEVICE_ID, propId, NULL, 0);
    }
}

void receiveLayerActionsClear(layer_id_t layerId) {
    memset(&CurrentKeymap[layerId][SlotId_LeftKeyboardHalf][0], 0,
        sizeof(key_action_t) * MAX_KEY_COUNT_PER_MODULE);
}

void receiveLayerModuleActions(sync_command_action_t* actions, uint8_t layerId, uint8_t slotId, uint8_t bufferOffset, uint8_t actionOffset, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        uint8_t actionIdx = actionOffset + i;
        uint8_t bufferIdx = bufferOffset + i;
        key_action_t *dstAction = &CurrentKeymap[layerId][slotId][actionIdx];
        dstAction->color = actions[bufferIdx].color;
        dstAction->colorOverridden = actions[bufferIdx].colorOverriden;
        dstAction->type = actions[bufferIdx].type;
        dstAction->keystroke.modifiers = actions[bufferIdx].modifierPresent ? 0xff : 0;
        dstAction->keystroke.scancode = actions[bufferIdx].scancode;
    }
}

void receiveLayerActions(sync_command_layer_t *buffer)
{
    if (buffer->actionCount) {
        receiveLayerModuleActions(buffer->actions, buffer->layerId, SlotId_LeftKeyboardHalf, 0, buffer->startOffset, buffer->actionCount);
    }
    if (buffer->moduleActionCount) {
        receiveLayerModuleActions(buffer->actions, buffer->layerId, SlotId_LeftModule, buffer->actionCount, 0, buffer->moduleActionCount);
    }
}

void receiveBacklight(sync_command_backlight_t *buffer) {
    Cfg.BacklightingMode = buffer->BacklightingMode;
    KeyBacklightBrightness = buffer->KeyBacklightBrightness;
    DisplayBrightness = buffer->DisplayBacklightBrightness;
    Cfg.LedMap_ConstantRGB = buffer->LedMap_ConstantRGB;
    if (HardwareConfig->isIso != buffer->isIso) {
        HardwareConfig->isIso = buffer->isIso;
        Ledmap_InitLedLayout();
        Ledmap_UpdateBacklightLeds();
    }
}

void StateSync_CheckFirmwareVersions() {
    #if DEVICE_IS_UHK80_RIGHT

    uint8_t driverId = UhkModuleSlaveDriver_SlotIdToDriverId(SlotId_LeftKeyboardHalf);
    uhk_module_state_t *moduleState = &UhkModuleStates[driverId];

    bool versionsMatch = VERSIONS_EQUAL(moduleState->firmwareVersion, firmwareVersion);
    bool leftChecksumMatches = memcmp(moduleState->firmwareChecksum, DeviceMD5Checksums[DeviceId_Uhk80_Left], MD5_CHECKSUM_LENGTH) == 0;
    bool gitTagsMatch = strcmp(moduleState->gitTag, gitTag) == 0;

    bool anyVersionZero = (moduleState->firmwareVersion.major == 0 || firmwareVersion.major == 0);
    bool anyChecksumZero = memcmp(DeviceMD5Checksums[DeviceId_Uhk80_Right], DeviceMD5Checksums[DeviceId_Uhk80_Left], MD5_CHECKSUM_LENGTH) == 0;

    if (anyChecksumZero) {
        // Don't spam development builds.
        return;
    }

    const char* universal = "Please flash both halves to the same version!";

    if (!versionsMatch) {
        LogUOS("Error: Left and right keyboard halves have different firmware versions (Left: %d.%d.%d, Right: %d.%d.%d)!\n",
            moduleState->firmwareVersion.major, moduleState->firmwareVersion.minor, moduleState->firmwareVersion.patch, firmwareVersion.major, firmwareVersion.minor, firmwareVersion.patch
        );
    }
    if (!gitTagsMatch) {
        LogUOS("Error: Left and right keyboard halves have different git tags (Left: %s, Right: %s)!\n", moduleState->gitTag, gitTag);
    }
    if (!leftChecksumMatches) {
        LogUOS("Error: Left checksum differs from the expected! Expected '%s', got '%s'!\n", DeviceMD5Checksums[DeviceId_Uhk80_Left], moduleState->firmwareChecksum);
    }
    if (!versionsMatch || !gitTagsMatch || !leftChecksumMatches) {
        LogUOS("    %s", universal);
    }
    if (anyVersionZero) {
        LogUOS("Warning: Keyboard halves have zero versions! %s\n", universal);
    }
    if (anyChecksumZero) {
        LogUOS("Warning: Keyboard halves have zero checksums! %s\n", universal);
    }

    #endif
}

static void checkDongleProtocolVersion() {
    if (!VERSIONS_EQUAL(DongleProtocolVersion, dongleProtocolVersion)) {
        LogUOS("Dongle and right half run different dongle protocol versios (dongle: %d.%d.%d, right: %d.%d.%d), please upgrade!\n",
                DongleProtocolVersion.major, DongleProtocolVersion.minor, DongleProtocolVersion.patch,
                dongleProtocolVersion.major, dongleProtocolVersion.minor, dongleProtocolVersion.patch
        );
        return;
    }
}

static void receiveModuleStateData(sync_command_module_state_t *buffer) {
    uint8_t driverId = UhkModuleSlaveDriver_SlotIdToDriverId(buffer->slotId);
    uhk_module_state_t *moduleState = &UhkModuleStates[driverId];

    // once we have multiple left modules, reload keymap here
    bool leftModuleChanged = buffer->slotId != SlotId_LeftKeyboardHalf && moduleState->moduleId != buffer->moduleId;

    module_connection_state_t *moduleConnectionState = &ModuleConnectionStates[driverId];
    moduleConnectionState->moduleId = buffer->moduleId;
    moduleState->moduleId = buffer->moduleId;
    moduleState->moduleProtocolVersion = buffer->moduleProtocolVersion;
    moduleState->firmwareVersion = buffer->firmwareVersion;
    moduleState->keyCount = buffer->keyCount;
    moduleState->pointerCount = buffer->pointerCount;
    Utils_SafeStrCopy(moduleState->gitRepo, buffer->gitRepo, MAX_STRING_PROPERTY_LENGTH);
    Utils_SafeStrCopy(moduleState->gitTag, buffer->gitTag, MAX_STRING_PROPERTY_LENGTH);
    memcpy(moduleState->firmwareChecksum, buffer->firmwareChecksum, MD5_CHECKSUM_LENGTH);

    if (DEVICE_IS_UHK80_RIGHT && leftModuleChanged) {
        EventVector_Set(EventVector_KeymapReloadNeeded);
    }
}

static void receiveProperty(device_id_t src, state_sync_prop_id_t propId, const uint8_t *data, uint8_t len) {
    state_sync_prop_t *prop = &stateSyncProps[propId];
    bool isLocalUpdate = src == DEVICE_ID;

    if (isLocalUpdate) {
        STATE_SYNC_LOG("=== Updating local property %s\n", prop->name);
    } else {
        STATE_SYNC_LOG(">>> Received remote property %s from %s\n", prop->name, Utils_DeviceIdToString(src));
    }

    if (src == DeviceId_Uhk80_Left && prop->leftData && data) {
        memcpy(prop->leftData, data, prop->len);
    }

    if (src == DeviceId_Uhk80_Right && prop->rightData && data) {
        memcpy(prop->rightData, data, prop->len);
    }

    if (src == DeviceId_Uhk_Dongle && prop->dongleData && data) {
        memcpy(prop->dongleData, data, prop->len);
    }

    switch (propId) {
    case StateSyncPropertyId_LayerActionsClear:
        receiveLayerActionsClear(*((layer_id_t *)data));
        if (*((layer_id_t *)data) == ActiveLayer) {
            Ledmap_UpdateBacklightLeds();
        }
        break;
    case StateSyncPropertyId_LayerActionFirst ... StateSyncPropertyId_LayerActionLast:
        receiveLayerActions((sync_command_layer_t *)data);
        if ((propId - StateSyncPropertyId_LayerActionFirst) == ActiveLayer) {
            EventVector_Set(EventVector_LedMapUpdateNeeded);
        }
        break;
    case StateSyncPropertyId_ActiveLayer:
        if (!isLocalUpdate) {
            if (ActiveLayer >= LayerId_Count) {
                LogU("Received invalid active layer %d --- %d %d %d %d %d | %d %d | %d %d\n", ActiveLayer, data[-5], data[-4], data[-3], data[-2], data[-1], data[0], data[1], data[2], data[3]);
                ActiveLayer = LayerId_Base;
            }
            EventVector_Set(EventVector_LedMapUpdateNeeded);
        }
        break;
    case StateSyncPropertyId_Backlight:
        if (!isLocalUpdate) {
            receiveBacklight((sync_command_backlight_t *)data);
        }
        EventVector_Set(EventVector_LedMapUpdateNeeded);
        break;
    case StateSyncPropertyId_Battery:
        WIDGET_REFRESH(&StatusWidget);
        {
            bool newRunningOnBattery = !SyncLeftHalfState.battery.powered || !SyncRightHalfState.battery.powered;
            bool newRightRunningOnBattery = !SyncRightHalfState.battery.powered;
            if (RunningOnBattery != newRunningOnBattery) {
                RunningOnBattery = newRunningOnBattery;
                RightRunningOnBattery = newRightRunningOnBattery;
                EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
            } else if (RightRunningOnBattery != newRightRunningOnBattery) {
                RightRunningOnBattery = newRightRunningOnBattery;
                LedManager_UpdateSleepModes();
            }
        }
        break;
    case StateSyncPropertyId_ActiveKeymap:
        // TODO
        break;
    case StateSyncPropertyId_KeyboardLedsState:
        if (!isLocalUpdate) {
            WIDGET_REFRESH(&StatusWidget);
            if (DongleProtocolVersion.major == 0) {
                LogUOS("Dongle protocol version doesn't seem to have been reported. Is your dongle firmware up to date?\n");
            }
        }
        break;
    case StateSyncPropertyId_ResetRightLeftLink:
        StateSync_ResetRightLeftLink(false);
        break;
    case StateSyncPropertyId_ResetRightDongleLink:
        StateSync_ResetRightDongleLink(false);
        break;
    case StateSyncPropertyId_LeftModuleDisconnected:
        module_connection_state_t *moduleConnectionState = &ModuleConnectionStates[UhkModuleDriverId_LeftModule];
        moduleConnectionState->moduleId = 0;
        break;
    case StateSyncPropertyId_ModuleStateLeftHalf:
    case StateSyncPropertyId_ModuleStateLeftModule:
        if (!isLocalUpdate) {
            receiveModuleStateData((sync_command_module_state_t *)data);
        }
        break;
    case StateSyncPropertyId_FunctionalColors:
        if (!isLocalUpdate) {
            EventVector_Set(EventVector_LedMapUpdateNeeded);
        }
        break;
    case StateSyncPropertyId_Config:
        if (!isLocalUpdate) {
            sync_command_config_t* buffer = (sync_command_config_t*)data;
            DataModelVersion = buffer->dataModelVersion;
        }
        break;
    case StateSyncPropertyId_MergeSensor:
        break;
    case StateSyncPropertyId_SwitchTestMode:
        if (!isLocalUpdate) {
            bool newMode = *(bool*)data;
            if (newMode != TestSwitches) {
                newMode ? TestSwitches_Activate() : TestSwitches_Deactivate();
                Main_Wake();
            }
        }
    case StateSyncPropertyId_DongleStandby:
        if (DEVICE_IS_UHK_DONGLE) {
            DongleLeds_Update();
        }
        break;
    case StateSyncPropertyId_KeyStatesDummy:
        break;
    case StateSyncPropertyId_DongleScrollMultipliers:
        if (!isLocalUpdate) {
            DongleScrollMultipliers = *(scroll_multipliers_t*)data;
        }
        break;
    case StateSyncPropertyId_DongleProtocolVersion:
        if (!isLocalUpdate) {
            DongleProtocolVersion = *(version_t*)data;
            checkDongleProtocolVersion();
        }
        break;
    case StateSyncPropertyId_ZeroDummy:
        printk("Received an invalid state sync property message: %d %d %d | %d %d | %d %d %d %d %d\n", data[-5], data[-4], data[-3], data[-2], data[-1], data[0], data[1], data[2], data[3], data[4]);
        break;
    case StateSyncPropertyId_PowerMode:
        break;
    default:
        printk("Property %i ('%s') has no receive handler. If this is correct, please add a "
               "separate empty case...\n",
            propId, prop->name);
        break;
    }
}

void StateSync_ReceiveStateUpdate(device_id_t src, const uint8_t *data, uint8_t len) {
    ATTR_UNUSED message_id_t messageId = *(data++);
    ATTR_UNUSED state_sync_prop_id_t propId = *(data++);

    receiveProperty(src, propId, data, len - 2);
}

static void submitPreparedData(device_id_t dst, state_sync_prop_id_t propId, const uint8_t *data, uint8_t len) {
    STATE_SYNC_LOG("    Sending %s data to %s\n", stateSyncProps[propId].name, Utils_DeviceIdToString(dst));
    Messenger_Send2(dst, MessageId_StateSync, propId, data, len);
}

static void prepareLayerActions(layer_id_t layerId, uint8_t slotId, uint8_t bufferOffset, uint8_t actionOffset, uint8_t count, sync_command_layer_t *buffer) {
    for (uint8_t i = 0; i < count; i++) {
        uint8_t actionIdx = actionOffset + i;
        uint8_t bufferIdx = bufferOffset + i;
        key_action_t *action = &CurrentKeymap[layerId][slotId][actionIdx];
        buffer->actions[bufferIdx].type = action->type;
        buffer->actions[bufferIdx].scancode = action->keystroke.scancode;
        buffer->actions[bufferIdx].color = action->color;
        buffer->actions[bufferIdx].colorOverriden = action->colorOverridden;
        buffer->actions[bufferIdx].modifierPresent = action->keystroke.modifiers != 0;
    }
}

static void prepareAndSubmitLayer(device_id_t dst, state_sync_prop_id_t propId, layer_id_t layerId) {
    sync_command_layer_t buffer;

    const uint8_t firstPacketLeftHalfActionCount = KEY_COUNT_PER_UPDATE;
    const uint8_t secondPacketLeftHalfActionCount = MAX_KEY_COUNT_PER_MODULE - KEY_COUNT_PER_UPDATE;
    const uint8_t secondPacketLeftModuleActionCount = MAX_BACKLIT_KEY_COUNT_PER_LEFT_MODULE;

    buffer.layerId = layerId;
    buffer.startOffset = 0;
    buffer.actionCount = firstPacketLeftHalfActionCount;
    buffer.moduleActionCount = 0;

    prepareLayerActions(layerId, SlotId_LeftKeyboardHalf, 0, 0, firstPacketLeftHalfActionCount, &buffer);
    submitPreparedData(dst, propId, (const uint8_t *)&buffer, sizeof(buffer));

    buffer.layerId = layerId;
    buffer.startOffset = firstPacketLeftHalfActionCount;
    buffer.actionCount = secondPacketLeftHalfActionCount;
    buffer.moduleActionCount = secondPacketLeftModuleActionCount;

    prepareLayerActions(layerId, SlotId_LeftKeyboardHalf, 0, firstPacketLeftHalfActionCount, secondPacketLeftHalfActionCount, &buffer);
    prepareLayerActions(layerId, SlotId_LeftModule, secondPacketLeftHalfActionCount, 0, secondPacketLeftModuleActionCount, &buffer);
    submitPreparedData(dst, propId, (const uint8_t *)&buffer, sizeof(buffer));
}

static void prepareBacklight(sync_command_backlight_t *buffer) {
    buffer->BacklightingMode = Ledmap_GetEffectiveBacklightMode();
    buffer->KeyBacklightBrightness = KeyBacklightBrightness;
    buffer->DisplayBacklightBrightness = DisplayBrightness;
    buffer->LedMap_ConstantRGB = Cfg.LedMap_ConstantRGB;
    buffer->isIso = HardwareConfig->isIso;
}

static void prepareLeftHalfStateData(sync_command_module_state_t *buffer) {
#if DEVICE_IS_UHK80_LEFT
    buffer->slotId = SlotId_LeftKeyboardHalf;
    buffer->moduleId = MODULE_ID;
    buffer->moduleProtocolVersion = moduleProtocolVersion;
    buffer->firmwareVersion = firmwareVersion;
    buffer->keyCount = MODULE_KEY_COUNT;
    buffer->pointerCount = MODULE_POINTER_COUNT;
    Utils_SafeStrCopy(buffer->gitRepo, gitRepo, MAX_STRING_PROPERTY_LENGTH);
    Utils_SafeStrCopy(buffer->gitTag, gitTag, MAX_STRING_PROPERTY_LENGTH);
    memcpy(buffer->firmwareChecksum, DeviceMD5Checksums[DEVICE_ID], MD5_CHECKSUM_LENGTH);
#endif
}

static void prepareLeftModuleStateData(sync_command_module_state_t *buffer) {
    uhk_module_state_t *moduleState = UhkModuleStates + UhkModuleSlaveDriver_SlotIdToDriverId(SlotId_LeftModule);;

    buffer->slotId = SlotId_LeftModule;
    buffer->moduleId = moduleState->moduleId;
    buffer->moduleProtocolVersion = moduleState->moduleProtocolVersion;
    buffer->firmwareVersion = moduleState->firmwareVersion;
    buffer->keyCount = moduleState->keyCount;
    buffer->pointerCount = moduleState->pointerCount;
    memcpy(buffer->gitRepo, moduleState->gitRepo, MAX_STRING_PROPERTY_LENGTH);
    memcpy(buffer->gitTag, moduleState->gitTag, MAX_STRING_PROPERTY_LENGTH);
    memcpy(buffer->firmwareChecksum, moduleState->firmwareChecksum, MD5_CHECKSUM_LENGTH);
}

static void prepareData(device_id_t dst, const uint8_t *propDataPtr, state_sync_prop_id_t propId) {
    state_sync_prop_t *prop = &stateSyncProps[propId];
    const uint8_t *data = propDataPtr;

    uint8_t len = prop->len;

    STATE_SYNC_LOG("<<< Preparing %s data for %s\n", prop->name, Utils_DeviceIdToString(dst));

    prop->dirtyState = DirtyState_Clean;

    switch (propId) {
    case StateSyncPropertyId_LayerActionFirst ... StateSyncPropertyId_LayerActionLast: {
        layer_id_t layerId = propId - StateSyncPropertyId_LayerActionFirst + LayerId_First;

        if (prop->dirtyState == DirtyState_NeedsClearing) {
            submitPreparedData(dst, StateSyncPropertyId_LayerActionsClear, &layerId, sizeof(layerId));
            return;
        } else {
            // 2 packets!
            prepareAndSubmitLayer(dst, propId, layerId);
            return;
        }
    } break;
    case StateSyncPropertyId_ModuleStateLeftHalf: {
        sync_command_module_state_t buffer = {};
        prepareLeftHalfStateData(&buffer);
        submitPreparedData(dst, propId, (const uint8_t *)&buffer, sizeof(buffer));
        return;
    } break;
    case StateSyncPropertyId_ModuleStateLeftModule: {
        sync_command_module_state_t buffer = {};
        prepareLeftModuleStateData(&buffer);
        submitPreparedData(dst, propId, (const uint8_t *)&buffer, sizeof(buffer));
        return;
    } break;
    case StateSyncPropertyId_Backlight: {
        sync_command_backlight_t buffer;
        prepareBacklight(&buffer);
        submitPreparedData(dst, propId, (const uint8_t *)&buffer, sizeof(buffer));
        return;
    } break;
    case StateSyncPropertyId_Config: {
        sync_command_config_t buffer;
        buffer.dataModelVersion = DataModelVersion;
        submitPreparedData(dst, propId, (const uint8_t *)&buffer, sizeof(buffer));
        return;
    } break;
    case StateSyncPropertyId_SwitchTestMode: {
        submitPreparedData(dst, propId, (const uint8_t *)&TestSwitches, sizeof(TestSwitches));
        return;
    }
    case StateSyncPropertyId_DongleProtocolVersion: {
        submitPreparedData(dst, propId, (const uint8_t *)&dongleProtocolVersion, sizeof(dongleProtocolVersion));
        return;
    }
    case StateSyncPropertyId_KeyStatesDummy: {
#if DEVICE_IS_KEYBOARD
        KeyScanner_ResendKeyStates = true;
        UhkModuleDriver_ResendKeyStates = true;
#endif
        return;
    }
    default:
        break;
    }

    submitPreparedData(dst, propId, data, len);
}

static void updateProperty(state_sync_prop_id_t propId) {
    device_id_t dst;
    const uint8_t *dataPtr;
    state_sync_prop_t *prop = &stateSyncProps[propId];

    if (DEVICE_ID == DeviceId_Uhk80_Left && (prop->direction & SyncDirection_LeftToRight)) {
        dst = DeviceId_Uhk80_Right;
        dataPtr = prop->leftData;
        prepareData(dst, dataPtr, propId);
    }
    if (DEVICE_ID == DeviceId_Uhk80_Right && (prop->direction & SyncDirection_RightToLeft)) {
        dst = DeviceId_Uhk80_Left;
        dataPtr = prop->rightData;
        prepareData(dst, dataPtr, propId);
    }
    if (DEVICE_ID == DeviceId_Uhk80_Right && (prop->direction & SyncDirection_RightToDongle)) {
        dst = DeviceId_Uhk_Dongle;
        dataPtr = prop->rightData;
        prepareData(dst, dataPtr, propId);
    }
    if (DEVICE_ID == DeviceId_Uhk_Dongle && (prop->direction & SyncDirection_DongleToRight)) {
        dst = DeviceId_Uhk80_Right;
        dataPtr = prop->dongleData;
        prepareData(dst, dataPtr, propId);
    }
}

typedef enum {
    UpdateResult_AllUpToDate,
    UpdateResult_UpdatedHighPrio,
    UpdateResult_UpdatedLowPrio,
    UpdateResult_UpdatedDelayed,
} update_result_t;

#define UPDATE_AND_RETURN_IF_DIRTY(propId, res)                                                    \
    if (stateSyncProps[propId].dirtyState != DirtyState_Clean) {                                   \
        updateProperty(propId);                                                                    \
        return res;                                                                                \
    }

static update_result_t handlePropertyUpdateRightToLeft() {
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ResetRightLeftLink, UpdateResult_UpdatedLowPrio);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_Config, UpdateResult_UpdatedHighPrio);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_SwitchTestMode, UpdateResult_UpdatedHighPrio);

    if (KeyBacklightBrightness != 0) {
        // Update relevant data
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_PowerMode, UpdateResult_UpdatedHighPrio);
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_FunctionalColors, UpdateResult_UpdatedHighPrio);
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_LayerActionFirst + ActiveLayer, UpdateResult_UpdatedHighPrio);
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ActiveKeymap, UpdateResult_UpdatedHighPrio);
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ActiveLayer, UpdateResult_UpdatedHighPrio);
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_Backlight, UpdateResult_UpdatedHighPrio);

        // Update rest of layers
        state_sync_prop_id_t first = StateSyncPropertyId_LayerActionFirst;
        state_sync_prop_id_t last = StateSyncPropertyId_LayerActionLast;
        for (state_sync_prop_id_t propId = first; propId <= last; propId++) {
            UPDATE_AND_RETURN_IF_DIRTY(propId, UpdateResult_UpdatedLowPrio);
        }
    } else {
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_Backlight, UpdateResult_UpdatedHighPrio);
    }

    return UpdateResult_AllUpToDate;
}

static bool handlePropertyUpdateLeftToRight() {
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ResetRightLeftLink, UpdateResult_UpdatedLowPrio);

    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ModuleStateLeftHalf, UpdateResult_UpdatedHighPrio);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ModuleStateLeftModule, UpdateResult_UpdatedHighPrio);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_KeyStatesDummy, UpdateResult_UpdatedHighPrio);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_LeftModuleDisconnected, UpdateResult_UpdatedHighPrio);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_MergeSensor, UpdateResult_UpdatedHighPrio);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_Battery, UpdateResult_UpdatedHighPrio);

    return UpdateResult_AllUpToDate;
}

static bool handlePropertyUpdateDongleToRight() {
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ResetRightDongleLink, UpdateResult_UpdatedLowPrio);

    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_DongleProtocolVersion, UpdateResult_UpdatedHighPrio);

    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_KeyboardLedsState, UpdateResult_UpdatedHighPrio);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_DongleScrollMultipliers, UpdateResult_UpdatedHighPrio);

    return UpdateResult_AllUpToDate;
}

static bool handlePropertyUpdateRightToDongle() {
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ResetRightDongleLink, UpdateResult_UpdatedLowPrio);

    return UpdateResult_AllUpToDate;
}

static void updateLoopRightLeft() {
    update_result_t res;

    if (DEVICE_ID == DeviceId_Uhk80_Left) {
        while (true) {
            bool isConnected = DeviceState_IsDeviceConnected(DeviceId_Uhk80_Right);
            STATE_SYNC_LOG("--- Left to right update loop, connected: %i\n", isConnected);

            if (!isConnected || (res = handlePropertyUpdateLeftToRight()) == UpdateResult_AllUpToDate) {
                k_sleep(K_FOREVER);
            } else {
                uint32_t delay = res == UpdateResult_UpdatedHighPrio ? STATE_SYNC_SEND_DELAY : STATE_SYNC_SEND_DELAY*10;
                k_sleep(K_MSEC(delay));
            }
        }
    }

    if (DEVICE_ID == DeviceId_Uhk80_Right) {
        while (true) {
            bool isConnected = DeviceState_IsDeviceConnected(DeviceId_Uhk80_Left);
            STATE_SYNC_LOG("--- Right to left update loop, connected: %i\n", isConnected);
            if (!isConnected || (res = handlePropertyUpdateRightToLeft()) == UpdateResult_AllUpToDate) {
                k_sleep(K_FOREVER);
            } else {
                uint32_t delay = res == UpdateResult_UpdatedHighPrio ? STATE_SYNC_SEND_DELAY : STATE_SYNC_SEND_DELAY*10;
                k_sleep(K_MSEC(delay));
            }
        }
    }
}

static void updateStandbys() {
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        uint8_t connectionId = Peers[peerId].connectionId;
        if (Connections_Type(connectionId) == ConnectionType_NusDongle) {
            bool standby = !(ActiveHostConnectionId == connectionId);
            Messenger_Send2Via(DeviceId_Uhk_Dongle, connectionId, MessageId_StateSync, StateSyncPropertyId_DongleStandby, (const uint8_t*)&standby, 1);
        }
    }
}

static void updateLoopRightDongle() {
    update_result_t res;

    if (DEVICE_ID == DeviceId_Uhk80_Right) {
        while (true) {
            bool isConnected = DeviceState_IsDeviceConnected(DeviceId_Uhk_Dongle);
            STATE_SYNC_LOG("--- Right to dongle update loop, connected: %i\n", isConnected);

            if (stateSyncProps[StateSyncPropertyId_DongleStandby].dirtyState != DirtyState_Clean) {                                   \
                updateStandbys();                                                                    \
            }

            if (!isConnected || (res = handlePropertyUpdateRightToDongle()) == UpdateResult_AllUpToDate) {
                k_sleep(K_FOREVER);
            } else {
                uint32_t delay = res == UpdateResult_UpdatedHighPrio ? STATE_SYNC_SEND_DELAY : STATE_SYNC_SEND_DELAY*10;
                k_sleep(K_MSEC(delay));
            }
        }
    }

    if (DEVICE_ID == DeviceId_Uhk_Dongle) {
        while (true) {
            bool isConnected = DeviceState_IsDeviceConnected(DeviceId_Uhk80_Right);
            STATE_SYNC_LOG("--- Dongle update loop, connected: %i\n", isConnected);
            if (!isConnected || DongleStandby || (res = handlePropertyUpdateDongleToRight()) == UpdateResult_AllUpToDate) {
                k_sleep(K_FOREVER);
            } else {
                uint32_t delay = res == UpdateResult_UpdatedHighPrio ? STATE_SYNC_SEND_DELAY : STATE_SYNC_SEND_DELAY*10;
                k_sleep(K_MSEC(delay));
            }
        }
    }
}

void StateSync_UpdateLayer(layer_id_t layerId, bool fullUpdate) {
    state_sync_prop_id_t propId = StateSyncPropertyId_LayerActionFirst + layerId - LayerId_First;

    stateSyncProps[propId].dirtyState = fullUpdate ? DirtyState_NeedsUpdate : DirtyState_NeedsClearing;
    stateSyncProps[propId].defaultDirty = stateSyncProps[propId].dirtyState;

    WAKE(stateSyncThreadLeftId);
}

void StateSync_Init() {
    if (DEVICE_ID == DeviceId_Uhk80_Left || DEVICE_ID == DeviceId_Uhk80_Right) {
        stateSyncThreadLeftId = k_thread_create(&thread_data_left, stack_area_left,
            K_THREAD_STACK_SIZEOF(stack_area_left), updateLoopRightLeft, NULL, NULL, NULL,
            THREAD_PRIORITY, 0, K_NO_WAIT);
        k_thread_name_set(&thread_data_left, "state_sync_left_right");
    }

    if (DEVICE_ID == DeviceId_Uhk80_Right || DEVICE_ID == DeviceId_Uhk_Dongle) {
        stateSyncThreadDongleId = k_thread_create(&thread_data_dongle, stack_area_dongle,
            K_THREAD_STACK_SIZEOF(stack_area_dongle), updateLoopRightDongle, NULL, NULL, NULL,
            THREAD_PRIORITY, 0, K_NO_WAIT);
        k_thread_name_set(&thread_data_dongle, "state_sync_dongle_right");
    }
}

void StateSync_ResetRightLeftLink(bool bidirectional) {
    printk("Resetting left right link! %s\n", bidirectional ? "Bidirectional" : "Unidirectional");
    StateSync_LeftResetCounter++;
    if (bidirectional) {
        invalidateProperty(StateSyncPropertyId_ResetRightLeftLink);
    }
    if (DEVICE_ID == DeviceId_Uhk80_Right) {
        invalidateProperty(StateSyncPropertyId_Config);
        state_sync_prop_id_t first = StateSyncPropertyId_LayerActionFirst;
        state_sync_prop_id_t last = StateSyncPropertyId_LayerActionLast;
        for (state_sync_prop_id_t propId = first; propId <= last; propId++) {
            invalidateProperty(propId);
        }
        invalidateProperty(StateSyncPropertyId_ActiveLayer);
        invalidateProperty(StateSyncPropertyId_Backlight);
        invalidateProperty(StateSyncPropertyId_FunctionalColors);
        invalidateProperty(StateSyncPropertyId_PowerMode);
        // Wait sufficiently log so the firmware check isnt triggered during firmware upgrade
        EventScheduler_Schedule(CurrentTime + 60000, EventSchedulerEvent_CheckFwChecksums, "Reset left right link");
    }
    if (DEVICE_ID == DeviceId_Uhk80_Left) {
        invalidateProperty(StateSyncPropertyId_Battery);
        invalidateProperty(StateSyncPropertyId_ModuleStateLeftHalf);
        invalidateProperty(StateSyncPropertyId_ModuleStateLeftModule);
        invalidateProperty(StateSyncPropertyId_KeyStatesDummy);
        invalidateProperty(StateSyncPropertyId_MergeSensor);
    }
}

void StateSync_ResetRightDongleLink(bool bidirectional) {
    StateSync_DongleResetCounter++;
    printk("Resetting dongle right link! %s\n", bidirectional ? "Bidirectional" : "Unidirectional");
    if (bidirectional) {
        invalidateProperty(StateSyncPropertyId_ResetRightDongleLink);
    }
    if (DEVICE_ID == DeviceId_Uhk_Dongle) {
        DongleStandby = false;
        invalidateProperty(StateSyncPropertyId_KeyboardLedsState);
        invalidateProperty(StateSyncPropertyId_DongleProtocolVersion);
        invalidateProperty(StateSyncPropertyId_DongleScrollMultipliers);
    }
}

void StateSync_ResetConfig() {
    // For simplicity, update all for now
    StateSync_ResetRightLeftLink(false);
}

const char* StateSync_PropertyIdToString(state_sync_prop_id_t propId) {
    return stateSyncProps[propId].name;
}
