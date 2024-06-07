#ifndef __STATE_SYNC_H__
#define __STATE_SYNC_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "legacy/layer_switcher.h"
    #include "legacy/key_action.h"
    #include "legacy/module.h"
    #include "keyboard/charger.h"

// Macros:

// Typedefs:

    typedef struct {
        battery_state_t battery;
    } sync_generic_half_state_t;

    typedef struct {
        uint8_t type : 5;
        uint8_t scancodePresent : 1;
        uint8_t modifierPresent : 1;
        uint8_t colorOverriden : 1;
        rgb_t color;
    } ATTR_PACKED sync_command_action_t;

    typedef struct {
        layer_id_t layerId;
        sync_command_action_t actions[MAX_KEY_COUNT_PER_MODULE];
    } sync_command_layer_t;

    typedef struct {
        uint8_t BacklightingMode;
        uint8_t KeyBacklightBrightness;
        uint8_t DisplayBacklightBrightness;
        rgb_t LedMap_ConstantRGB;
    } ATTR_PACKED sync_command_backlight_t;

    // Draft for generic properties
    typedef enum {
        StateSyncPropertyId_LayerActionsLayer1,
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
        StateSyncPropertyId_Count,
    } state_sync_prop_id_t;

    typedef enum {
        SyncDirection_LeftToRight,
        SyncDirection_RightToLeft,
        SyncDirection_MasterToSlave,
        SyncDirection_SlaveToMaster,
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
    void StateSync_Reset();

#endif
