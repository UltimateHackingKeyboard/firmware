#ifndef __STATE_SYNC_H__
#define __STATE_SYNC_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "legacy/layer_switcher.h"
    #include "legacy/key_action.h"
    #include "legacy/module.h"
    #include "keyboard/charger.h"
    #include "shared/attributes.h"
    #include "legacy/slave_drivers/uhk_module_driver.h"
    #include "versioning.h"

// Macros:

    #define MAX_KEY_COUNT_PER_UPDATE ((MAX_KEY_COUNT_PER_MODULE)/2+1)

// Typedefs:

    typedef struct {
        battery_state_t battery;
    } sync_generic_half_state_t;

    typedef struct {
        uint8_t type : 6;
        uint8_t modifierPresent : 1;
        uint8_t colorOverriden : 1;
        uint8_t scancode;
        rgb_t color;
    } ATTR_PACKED sync_command_action_t;

    typedef struct {
        layer_id_t layerId;
        uint8_t startOffset;
        uint8_t actionCount;
        sync_command_action_t actions[MAX_KEY_COUNT_PER_UPDATE];
    } sync_command_layer_t;

    typedef struct {
        uint8_t BacklightingMode;
        uint8_t KeyBacklightBrightness;
        uint8_t DisplayBacklightBrightness;
        rgb_t LedMap_ConstantRGB;
        bool isIso;
    } ATTR_PACKED sync_command_backlight_t;

    typedef struct {
        uint8_t slotId;
        uint8_t moduleId;
        version_t moduleProtocolVersion;
        version_t firmwareVersion;
        uint8_t keyCount;
        uint8_t pointerCount;
        char gitRepo[MAX_STRING_PROPERTY_LENGTH];
        char gitTag[MAX_STRING_PROPERTY_LENGTH];
        char firmwareChecksum[MD5_CHECKSUM_LENGTH];
    } ATTR_PACKED sync_command_module_state_t;

    // Draft for generic properties
    typedef enum {
        StateSyncPropertyId_LayerActionsLayer1 = 1,
        StateSyncPropertyId_LayerActionsLayer2,
        StateSyncPropertyId_LayerActionsLayer3,
        StateSyncPropertyId_LayerActionsLayer4,
        StateSyncPropertyId_LayerActionsLayer5,
        StateSyncPropertyId_LayerActionsLayer6,
        StateSyncPropertyId_LayerActionsLayer7,
        StateSyncPropertyId_LayerActionsLayer8,
        StateSyncPropertyId_LayerActionsLayer9,
        StateSyncPropertyId_LayerActionsLayer10,
        StateSyncPropertyId_LayerActionsLayer11,
        StateSyncPropertyId_LayerActionsLayer12,
        StateSyncPropertyId_LayerActionFirst = StateSyncPropertyId_LayerActionsLayer1,
        StateSyncPropertyId_LayerActionLast = StateSyncPropertyId_LayerActionsLayer12,
        StateSyncPropertyId_LayerActionsClear,
        StateSyncPropertyId_ActiveLayer,
        StateSyncPropertyId_Backlight,
        StateSyncPropertyId_ActiveKeymap,
        StateSyncPropertyId_Battery,
        StateSyncPropertyId_KeyboardLedsState,
        StateSyncPropertyId_ResetRightLeftLink,
        StateSyncPropertyId_ResetRightDongleLink,
        StateSyncPropertyId_ModuleStateLeftHalf,
        StateSyncPropertyId_ModuleStateLeftModule,
        StateSyncPropertyId_LeftModuleDisconnected,
        StateSyncPropertyId_MergeSensor,
        StateSyncPropertyId_FunctionalColors,
        StateSyncPropertyId_Count,
    } state_sync_prop_id_t;

    typedef enum {
        SyncDirection_LeftToRight = 1 << 0,
        SyncDirection_RightToLeft = 1 << 1,
        SyncDirection_DongleToRight = 1 << 2,
        SyncDirection_RightToDongle = 1 << 3,
        SyncDirection_RightDongleBidir = SyncDirection_RightToDongle | SyncDirection_DongleToRight,
        SyncDirection_RightLeftBidir = SyncDirection_LeftToRight | SyncDirection_RightToLeft,
    } sync_direction_t;

    typedef enum {
        DirtyState_Clean,
        DirtyState_NeedsUpdate,
        DirtyState_NeedsClearing,
    } dirty_state_t;

    typedef struct {
        const char* name;
        void* leftData;
        void* rightData;
        void* dongleData;
        uint8_t len;
        sync_direction_t direction;
        dirty_state_t dirtyState;
        dirty_state_t defaultDirty;
    } ATTR_PACKED state_sync_prop_t;

// Variables:

    extern sync_generic_half_state_t SyncLeftHalfState;
    extern sync_generic_half_state_t SyncRightHalfState;

// Functions:

    void StateSync_UpdateLayer(layer_id_t layerId, bool fullUpdate);
    void StateSync_Init();
    void StateSync_ReceiveProperty(device_id_t src, state_sync_prop_id_t property, void* data, uint8_t len);
    void StateSync_UpdateProperty(state_sync_prop_id_t propId, void* data);
    void StateSync_ReceiveStateUpdate(device_id_t src, const uint8_t* data, uint8_t len);

    void StateSync_ResetRightLeftLink(bool bidirectional);
    void StateSync_ResetRightDongleLink(bool bidirectional);
    void StateSync_ResetConfig();

#endif
