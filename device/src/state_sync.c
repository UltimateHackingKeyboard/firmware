#include "state_sync.h"
#include "device.h"
#include "device_state.h"
#include "event_scheduler.h"
#include "keyboard/oled/widgets/widgets.h"
#include "legacy/config_manager.h"
#include "legacy/config_parser/config_globals.h"
#include "legacy/debug.h"
#include "legacy/keymap.h"
#include "legacy/led_manager.h"
#include "legacy/ledmap.h"
#include "legacy/module.h"
#include "legacy/slave_drivers/uhk_module_driver.h"
#include "legacy/slot.h"
#include "legacy/str_utils.h"
#include "legacy/stubs.h"
#include "legacy/utils.h"
#include "messenger.h"
#include "state_sync.h"
#include "usb/usb_compatibility.h"
#include <stdint.h>
#include <zephyr/kernel.h>
#include "legacy/peripherals/merge_sensor.h"

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

static void receiveProperty(
    device_id_t src, state_sync_prop_id_t property, const uint8_t *data, uint8_t len);

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
    SIMPLE(ActiveLayer, SyncDirection_RightToLeft, DirtyState_Clean, &ActiveLayer),
    CUSTOM(Backlight, SyncDirection_RightToLeft, DirtyState_Clean),
    SIMPLE(ActiveKeymap, SyncDirection_RightToLeft, DirtyState_Clean, &CurrentKeymapIndex),
    SIMPLE(KeyboardLedsState, SyncDirection_DongleToRight, DirtyState_Clean, &KeyboardLedsState),
    DUAL_LR(Battery, SyncDirection_LeftToRight, DirtyState_Clean, &SyncLeftHalfState.battery,
        &SyncRightHalfState.battery),
    EMPTY(ResetRightLeftLink, SyncDirection_RightLeftBidir, DirtyState_Clean),
    EMPTY(ResetRightDongleLink, SyncDirection_RightDongleBidir, DirtyState_Clean),
    CUSTOM(ModuleStateLeftHalf, SyncDirection_LeftToRight, DirtyState_Clean),
    CUSTOM(ModuleStateLeftModule, SyncDirection_LeftToRight, DirtyState_Clean),
    EMPTY(LeftModuleDisconnected, SyncDirection_LeftToRight, DirtyState_Clean),
    SIMPLE(MergeSensor, SyncDirection_LeftToRight, DirtyState_Clean, &MergeSensor_HalvesAreMerged),
    SIMPLE(FunctionalColors, SyncDirection_RightToLeft, DirtyState_Clean, &Cfg.KeyActionColors),
};

static void invalidateProperty(state_sync_prop_id_t propId)
{
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
        k_wakeup(stateSyncThreadLeftId);
    }
    bool isRightDongleDevice =
        (DEVICE_ID == DeviceId_Uhk80_Right || DEVICE_ID == DeviceId_Uhk_Dongle);
    bool isRightDongleLink = (stateSyncProps[propId].direction &
                              (SyncDirection_DongleToRight | SyncDirection_RightToDongle));
    if (isRightDongleLink && isRightDongleDevice) {
        k_wakeup(stateSyncThreadDongleId);
    }
}

void StateSync_UpdateProperty(state_sync_prop_id_t propId, void *data)
{
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

void receiveLayerActionsClear(layer_id_t layerId)
{
    memset(&CurrentKeymap[layerId][SlotId_LeftKeyboardHalf][0], 0,
        sizeof(key_action_t) * MAX_KEY_COUNT_PER_MODULE);
}

void receiveLayerActions(sync_command_layer_t *buffer)
{
    layer_id_t layerId = buffer->layerId;
    for (uint8_t i = 0; i < buffer->actionCount; i++) {
        key_action_t *dstAction =
            &CurrentKeymap[layerId][SlotId_LeftKeyboardHalf][i + buffer->startOffset];
        dstAction->color = buffer->actions[i].color;
        dstAction->colorOverridden = buffer->actions[i].colorOverriden;
        dstAction->type = buffer->actions[i].type;
        dstAction->keystroke.modifiers = buffer->actions[i].modifierPresent ? 0xff : 0;
        dstAction->keystroke.scancode = buffer->actions[i].scancode;
    }
}

void receiveBacklight(sync_command_backlight_t *buffer)
{
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

static void receiveModuleStateData(sync_command_module_state_t *buffer)
{
    uint8_t driverId = UhkModuleSlaveDriver_SlotIdToDriverId(buffer->slotId);
    uhk_module_state_t *moduleState = &UhkModuleStates[driverId];
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
}

static void receiveProperty(
    device_id_t src, state_sync_prop_id_t propId, const uint8_t *data, uint8_t len)
{
    state_sync_prop_t *prop = &stateSyncProps[propId];
    bool isLocalUpdate = src == DEVICE_ID;

    if (isLocalUpdate) {
        STATE_SYNC_LOG("=== Updating local property %s\n", prop->name);
    } else {
        STATE_SYNC_LOG(
            ">>> Received remote property %s from %s\n", prop->name, Utils_DeviceIdToString(src));
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
            bool newRunningOnBattery =
                !SyncLeftHalfState.battery.powered || !SyncRightHalfState.battery.powered;
            if (RunningOnBattery != newRunningOnBattery) {
                RunningOnBattery = newRunningOnBattery;
                LedManager_FullUpdate();
            }
        }
        break;
    case StateSyncPropertyId_ActiveKeymap:
        // TODO
        break;
    case StateSyncPropertyId_KeyboardLedsState:
        WIDGET_REFRESH(&StatusWidget);
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
    case StateSyncPropertyId_MergeSensor:
        break;
    default:
        printk("Property %i ('%s') has no receive handler. If this is correct, please add a "
               "separate empty case...\n",
            propId, prop->name);
        break;
    }
}

void StateSync_ReceiveStateUpdate(device_id_t src, const uint8_t *data, uint8_t len)
{
    ATTR_UNUSED message_id_t messageId = *(data++);
    ATTR_UNUSED state_sync_prop_id_t propId = *(data++);

    receiveProperty(src, propId, data, len - 2);
}

static void submitPreparedData(
    device_id_t dst, state_sync_prop_id_t propId, const uint8_t *data, uint8_t len)
{
    STATE_SYNC_LOG(
        "    Sending %s data to %s\n", stateSyncProps[propId].name, Utils_DeviceIdToString(dst));
    Messenger_Send2(dst, MessageId_StateSync, propId, data, len);
}

static void prepareLayer(
    layer_id_t layerId, uint8_t offset, uint8_t count, sync_command_layer_t *buffer)
{
    buffer->layerId = layerId;
    buffer->startOffset = offset;
    buffer->actionCount = count;
    for (uint8_t dstKeyId = 0; dstKeyId < count; dstKeyId++) {
        key_action_t *action = &CurrentKeymap[layerId][SlotId_LeftKeyboardHalf][offset + dstKeyId];
        buffer->actions[dstKeyId].type = action->type;
        buffer->actions[dstKeyId].scancode = action->keystroke.scancode;
        buffer->actions[dstKeyId].color = action->color;
        buffer->actions[dstKeyId].colorOverriden = action->colorOverridden;
        buffer->actions[dstKeyId].modifierPresent = action->keystroke.modifiers != 0;
    }
}

static void prepareAndSubmitLayer(
    device_id_t dst, state_sync_prop_id_t propId, layer_id_t layerId, uint8_t offset, uint8_t count)
{
    sync_command_layer_t buffer;
    prepareLayer(layerId, offset, count, &buffer);
    submitPreparedData(dst, propId, (const uint8_t *)&buffer, sizeof(buffer));
}

static void prepareBacklight(sync_command_backlight_t *buffer)
{
    buffer->BacklightingMode = Ledmap_GetEffectiveBacklightMode();
    buffer->KeyBacklightBrightness = KeyBacklightBrightness;
    buffer->DisplayBacklightBrightness = DisplayBrightness;
    buffer->LedMap_ConstantRGB = Cfg.LedMap_ConstantRGB;
    buffer->isIso = HardwareConfig->isIso;
}

static void prepareLeftHalfStateData(sync_command_module_state_t *buffer)
{
#if DEVICE_IS_UHK80_LEFT
    buffer->slotId = SlotId_LeftKeyboardHalf;
    buffer->moduleId = MODULE_ID;
    buffer->moduleProtocolVersion = moduleProtocolVersion;
    buffer->firmwareVersion = firmwareVersion;
    buffer->keyCount = MODULE_KEY_COUNT;
    buffer->pointerCount = MODULE_POINTER_COUNT;
    Utils_SafeStrCopy(buffer->gitRepo, gitRepo, MAX_STRING_PROPERTY_LENGTH);
    Utils_SafeStrCopy(buffer->gitTag, gitTag, MAX_STRING_PROPERTY_LENGTH);
    memcpy(buffer->firmwareChecksum, ModuleMD5Checksums[MODULE_ID], MD5_CHECKSUM_LENGTH);
#endif
}

static void prepareLeftModuleStateData(sync_command_module_state_t *buffer)
{
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

static void prepareData(device_id_t dst, const uint8_t *propDataPtr, state_sync_prop_id_t propId)
{
    state_sync_prop_t *prop = &stateSyncProps[propId];
    const uint8_t *data = propDataPtr;

    uint8_t len = prop->len;

    STATE_SYNC_LOG("<<< Preparing %s data for %s\n", prop->name, Utils_DeviceIdToString(dst));

    prop->dirtyState = DirtyState_Clean;

    switch (propId) {
    case StateSyncPropertyId_LayerActionFirst ... StateSyncPropertyId_LayerActionLast: {
        layer_id_t layerId = propId - StateSyncPropertyId_LayerActionFirst + LayerId_First;

        if (prop->dirtyState == DirtyState_NeedsClearing) {
            submitPreparedData(
                dst, StateSyncPropertyId_LayerActionsClear, &layerId, sizeof(layerId));
            return;
        } else {
            prepareAndSubmitLayer(dst, propId, layerId, 0, MAX_KEY_COUNT_PER_UPDATE);
            prepareAndSubmitLayer(dst, propId, layerId, MAX_KEY_COUNT_PER_UPDATE,
                MAX_KEY_COUNT_PER_MODULE - MAX_KEY_COUNT_PER_UPDATE);
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
    default:
        break;
    }

    submitPreparedData(dst, propId, data, len);
}

static void updateProperty(state_sync_prop_id_t propId)
{
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

#define UPDATE_AND_RETURN_IF_DIRTY(propId)                                                         \
    if (stateSyncProps[propId].dirtyState != DirtyState_Clean) {                                   \
        updateProperty(propId);                                                                    \
        return false;                                                                              \
    }

static bool handlePropertyUpdateRightToLeft()
{
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ResetRightLeftLink);

    if (KeyBacklightBrightness != 0 && Cfg.BacklightingMode != BacklightingMode_ConstantRGB) {
        // Update relevant data
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_FunctionalColors);
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_LayerActionFirst + ActiveLayer);
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ActiveKeymap);
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ActiveLayer);
        UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_Backlight);

        // Update rest of layers
        state_sync_prop_id_t first = StateSyncPropertyId_LayerActionFirst;
        state_sync_prop_id_t last = StateSyncPropertyId_LayerActionLast;
        for (state_sync_prop_id_t propId = first; propId <= last; propId++) {
            UPDATE_AND_RETURN_IF_DIRTY(propId);
        }
    }
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_Backlight);

    return true;
}

static bool handlePropertyUpdateLeftToRight()
{
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ResetRightLeftLink);

    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ModuleStateLeftHalf);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ModuleStateLeftModule);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_LeftModuleDisconnected);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_MergeSensor);
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_Battery);

    return true;
}

static bool handlePropertyUpdateDongleToRight()
{
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ResetRightDongleLink);

    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_KeyboardLedsState);

    return true;
}

static bool handlePropertyUpdateRightToDongle()
{
    UPDATE_AND_RETURN_IF_DIRTY(StateSyncPropertyId_ResetRightDongleLink);

    return true;
}

static void updateLoopRightLeft()
{
    if (DEVICE_ID == DeviceId_Uhk80_Left) {
        while (true) {
            bool isConnected = DeviceState_IsDeviceConnected(DeviceId_Uhk80_Right);
            STATE_SYNC_LOG("--- Left to right update loop, connected: %i\n", isConnected);
            if (!isConnected || handlePropertyUpdateLeftToRight()) {
                k_sleep(K_FOREVER);
            } else {
                k_sleep(K_MSEC(1));
            }
        }
    }

    if (DEVICE_ID == DeviceId_Uhk80_Right) {
        while (true) {
            bool isConnected = DeviceState_IsDeviceConnected(DeviceId_Uhk80_Left);
            STATE_SYNC_LOG("--- Right to left update loop, connected: %i\n", isConnected);
            if (!isConnected || handlePropertyUpdateRightToLeft()) {
                k_sleep(K_FOREVER);
            } else {
                k_sleep(K_MSEC(1));
            }
        }
    }
}

static void updateLoopRightDongle()
{
    if (DEVICE_ID == DeviceId_Uhk80_Right) {
        while (true) {
            bool isConnected = DeviceState_IsDeviceConnected(DeviceId_Uhk_Dongle);
            STATE_SYNC_LOG("--- Right to dongle update loop, connected: %i\n", isConnected);
            if (!isConnected || handlePropertyUpdateRightToDongle()) {
                k_sleep(K_FOREVER);
            } else {
                k_sleep(K_MSEC(1));
            }
        }
    }

    if (DEVICE_ID == DeviceId_Uhk_Dongle) {
        while (true) {
            bool isConnected = DeviceState_IsDeviceConnected(DeviceId_Uhk80_Right);
            STATE_SYNC_LOG("--- Dongle update loop, connected: %i\n", isConnected);
            if (!isConnected || handlePropertyUpdateDongleToRight()) {
                k_sleep(K_FOREVER);
            } else {
                k_sleep(K_MSEC(1));
            }
        }
    }
}

void StateSync_UpdateLayer(layer_id_t layerId, bool fullUpdate)
{
    state_sync_prop_id_t propId = StateSyncPropertyId_LayerActionFirst + layerId - LayerId_First;

    stateSyncProps[propId].dirtyState =
        fullUpdate ? DirtyState_NeedsUpdate : DirtyState_NeedsClearing;
    stateSyncProps[propId].defaultDirty = stateSyncProps[propId].dirtyState;

    k_wakeup(stateSyncThreadLeftId);
}

void StateSync_Init()
{
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

void StateSync_ResetRightLeftLink(bool bidirectional)
{
    printk("Resetting left right link! %s\n", bidirectional ? "Bidirectional" : "Unidirectional");
    if (bidirectional) {
        invalidateProperty(StateSyncPropertyId_ResetRightLeftLink);
    }
    if (DEVICE_ID == DeviceId_Uhk80_Right) {
        state_sync_prop_id_t first = StateSyncPropertyId_LayerActionFirst;
        state_sync_prop_id_t last = StateSyncPropertyId_LayerActionLast;
        for (state_sync_prop_id_t propId = first; propId <= last; propId++) {
            invalidateProperty(propId);
        }
        invalidateProperty(StateSyncPropertyId_ActiveLayer);
        invalidateProperty(StateSyncPropertyId_Backlight);
    }
    if (DEVICE_ID == DeviceId_Uhk80_Left) {
        invalidateProperty(StateSyncPropertyId_Battery);
        invalidateProperty(StateSyncPropertyId_ModuleStateLeftHalf);
        invalidateProperty(StateSyncPropertyId_ModuleStateLeftModule);
        invalidateProperty(StateSyncPropertyId_MergeSensor);
    }
}

void StateSync_ResetRightDongleLink(bool bidirectional)
{
    printk("Resetting dongle right link! %s\n", bidirectional ? "Bidirectional" : "Unidirectional");
    if (bidirectional) {
        invalidateProperty(StateSyncPropertyId_ResetRightDongleLink);
    }
    if (DEVICE_ID == DeviceId_Uhk_Dongle) {
        invalidateProperty(StateSyncPropertyId_KeyboardLedsState);
    }
}

void StateSync_ResetConfig()
{
    invalidateProperty(StateSyncPropertyId_FunctionalColors);
}
