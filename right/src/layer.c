#include "layer.h"
#include "slot.h"
#include "module.h"
#include "key_states.h"
#include "keymap.h"

static bool heldLayers[LAYER_COUNT];
static bool pressedLayers[LAYER_COUNT];

void updateLayerStates(void)
{
    memset(heldLayers, false, LAYER_COUNT);
    memset(pressedLayers, false, LAYER_COUNT);

    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];
            if (keyState->current) {
                key_action_t action = CurrentKeymap[LayerId_Base][slotId][keyId];
                if (action.type == KeyActionType_SwitchLayer) {
                    if (!action.switchLayer.isToggle) {
                        heldLayers[action.switchLayer.layer] = true;
                    } else if (!keyState->previous && keyState->current) {
                        pressedLayers[action.switchLayer.layer] = true;
                    }
                }
            }
        }
    }
}

layer_id_t PreviousHeldLayer = LayerId_Base;

layer_id_t GetActiveLayer()
{
    updateLayerStates();

    // Handle toggled layers

    static layer_id_t toggledLayer = LayerId_Base;

    for (layer_id_t layerId=LayerId_Mod; layerId<=LayerId_Mouse; layerId++) {
        if (pressedLayers[layerId]) {
            if (toggledLayer == layerId) {
                toggledLayer = LayerId_Base;
                break;
            } else if (toggledLayer == LayerId_Base) {
                toggledLayer = layerId;
                break;
            }
        }
    }

    if (toggledLayer != LayerId_Base) {
        return toggledLayer;
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
