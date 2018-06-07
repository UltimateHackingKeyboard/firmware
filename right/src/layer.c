#include "layer.h"
#include "slot.h"
#include "module.h"
#include "key_states.h"
#include "keymap.h"

static bool heldLayers[LAYER_COUNT];
static bool toggledLayers[LAYER_COUNT];

void updateLayerStates(void)
{
    memset(heldLayers, false, LAYER_COUNT);
    memset(toggledLayers, false, LAYER_COUNT);

    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];
            if (keyState->current) {
                key_action_t action = CurrentKeymap[LayerId_Base][slotId][keyId];
                if (action.type == KeyActionType_SwitchLayer) {
                    if (action.switchLayer.mode != SwitchLayerMode_Toggle) {
                        heldLayers[action.switchLayer.layer] = true;
                    } else if (!keyState->previous) {
                        toggledLayers[action.switchLayer.layer] = true;
                    }
                }
            }
        }
    }
}

layer_id_t PreviousHeldLayer = LayerId_Base;
layer_id_t ToggledLayer = LayerId_Base;

layer_id_t GetActiveLayer()
{
    updateLayerStates();

    // Handle toggled layers

    for (layer_id_t layerId=LayerId_Mod; layerId<=LayerId_Mouse; layerId++) {
        if (toggledLayers[layerId]) {
            if (ToggledLayer == layerId) {
                ToggledLayer = LayerId_Base;
                break;
            } else if (ToggledLayer == LayerId_Base && toggledLayers[layerId] == SwitchLayerMode_Toggle) {
                ToggledLayer = layerId;
                break;
            }
        }
    }

    if (ToggledLayer != LayerId_Base) {
        return ToggledLayer;
    }

    // Handle held layers

    layer_id_t heldLayer = LayerId_Base;

    for (layer_id_t layerId=LayerId_Mod; layerId<=LayerId_Mouse; layerId++) {
        if (heldLayers[layerId]) {
            heldLayer = layerId;
            break;
        }
    }

    heldLayer = heldLayer != LayerId_Base && heldLayers[PreviousHeldLayer] ? PreviousHeldLayer : heldLayer;
    PreviousHeldLayer = heldLayer;

    return heldLayer;
}
