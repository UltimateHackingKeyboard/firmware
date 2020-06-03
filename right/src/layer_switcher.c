#include "layer_switcher.h"
#include "timer.h"
#include "macros.h"

static uint16_t DoubleTapSwitchLayerTimeout = 300;
static uint16_t DoubleTapSwitchLayerReleaseTimeout = 200;

layer_id_t ActiveLayer = LayerId_Base;
bool ActiveLayerHeld = false;

static layer_id_t toggledLayer = LayerId_Base;
static layer_id_t heldLayer = LayerId_Base;

/**
 * General logic.
 *
 * There are three types of actions:
 * - layer holds
 * - layer toggles
 * - secondary layer hold
 *
 * Requirements:
 * - Superiority:
 *   - Toggled layers are superior to layer holds.
 * - Evaluation:
 *   - Toggles are triggered by key presses/releases and behave as standard actions.
 *   - Standard holds are evaluated continuously in "background" irrespectively
 *     of currently active layer.
 * - Behaviour expectations:
 *   - Layer switch should never prevent non-base-layer action from happening.
 *   - If two holds are activated concurrently, the first one is superior to the second one.
 *     Yet, when the first one is released, the second one takes place.
 *
 * Secondary roles:
 * - if possible, secondary role holds behave as standard holds (experimental, toggled by SECONDARY_AS_REGULAR_HOLD).
 *   Otherwise:
 *   - Secondary layer hold is superior to standard holds.
 *   - Secondary layer hold is triggered by press/release and behaves as standard actions.
 *
 */

// Recompute active layer whenever state of the layer locks changes.
void updateActiveLayer() {
    layer_id_t activeLayer = LayerId_Base;
    layer_id_t activeLayerHeld = LayerId_Base;
    if(activeLayer == LayerId_Base) {
        activeLayer = toggledLayer;
    }
    if(activeLayer == LayerId_Base) {
        activeLayer = heldLayer;
    }
    activeLayerHeld = heldLayer == ActiveLayer && ActiveLayer != LayerId_Base;

    if(activeLayer == LayerId_Base) {
        activeLayer = Macros_ActiveLayer;
        activeLayerHeld = Macros_ActiveLayer;
    }
    //(write actual ActiveLayer atomically, so that random observer is not confused)
    ActiveLayer = activeLayer;
    ActiveLayerHeld = activeLayerHeld;
}

/*
 * Toggle handlers
 *
 * Further requirements:
 * - Any layer switch action untoggles corresponding toggled layer.
 *   (However, weaker untoggling policies would make sense too.)
 * - Any layer switch action untoggles only its own layer (e.g., mod switch untoggles only toggled mod layer).
 *   (Unlocking other layers would be in conflict with activation of an action on non-base layer, if
 *   such action was bound on a key whose base layer action hapenned to be a layer switch.)
 */

key_state_t *doubleTapSwitchLayerKey;

void LayerSwitcher_DoubleTapToggle(layer_id_t layer, key_state_t* keyState) {
    static uint32_t doubleTapSwitchLayerStartTime = 0;
    static uint32_t doubleTapSwitchLayerTriggerTime = 0;

    if(KeyState_ActivatedNow(keyState)) {
        toggledLayer = LayerId_Base;
        if (doubleTapSwitchLayerKey == keyState && Timer_GetElapsedTimeAndSetCurrent(&doubleTapSwitchLayerStartTime) < DoubleTapSwitchLayerTimeout) {
            toggledLayer = layer;
            doubleTapSwitchLayerTriggerTime = CurrentTime;
            doubleTapSwitchLayerStartTime = CurrentTime;
        } else {
            doubleTapSwitchLayerKey = keyState;
            doubleTapSwitchLayerStartTime = CurrentTime;
        }
        updateActiveLayer();
    }

    if(KeyState_DeactivatedNow(keyState)) {
        //If current press is too long, cancel current toggle
        if ( doubleTapSwitchLayerKey == keyState && Timer_GetElapsedTime(&doubleTapSwitchLayerTriggerTime) > DoubleTapSwitchLayerReleaseTimeout)
        {
            toggledLayer = LayerId_Base;
            updateActiveLayer();
        }
    }
}

// If some other key is pressed between taps of a possible doubletap, discard the doubletap
void LayerSwitcher_DoubleTapInterrupt(key_state_t* keyState) {
    if (doubleTapSwitchLayerKey != keyState) {
        doubleTapSwitchLayerKey = NULL;
    }
}

void LayerSwitcher_ToggleLayer(layer_id_t layer) {
    if(toggledLayer == LayerId_Base) {
        toggledLayer = layer;
    } else {
        toggledLayer = LayerId_Base;
    }
    updateActiveLayer();
}


void LayerSwitcher_UnToggleLayerOnly(layer_id_t layer) {
    if (toggledLayer != LayerId_Base) {
        toggledLayer = LayerId_Base;
        updateActiveLayer();
    }
}

/*
 * Hold handlers
 *
 * Holds are evaluated in a continuous manner, and updated once per every update cycle.
 * Their behaviour might be a bit nonintuitive due to expected outcome on overlap of held keys.
 *
 * To be explicit - base-layer holds are called even when non-base layer is active!
 *
 * In corner case, we expect following behaviour (A/B stand for physical keys with hold layer action to layers A/B):
 * A pressed   ; //layer A activates
 * B pressed   ; //layer A is active   B's A-Layer action activates
 * A released  ; //layer A is active   B's A-Layer action is active
 * ...         ; //layer B activates   B's A-Layer action is active //Note that at this point, B has two active actions at the same time from different layers
 * B released  ; //layer B is active   B's A-Layer action is active
 */

static bool heldLayers[LAYER_COUNT];

// Called by pressed hold-layer keys during every cycle
void LayerSwitcher_HoldLayer(layer_id_t layer) {
    heldLayers[layer] = true;
    //no other switcher is active, so we may as well activate it straight away.
    if(heldLayer == LayerId_Base) {
        heldLayer = layer;
        updateActiveLayer();
    }
}

// Gathers states set by LayerSwitcher_HoldLayer during previous update cycle and updates heldLayer.
void LayerSwitcher_UpdateActiveLayer() {
    layer_id_t previousHeldLayer = heldLayer;
    if(!heldLayers[heldLayer]) {
        heldLayer = LayerId_Base;
    }
    for (layer_id_t layerId = LayerId_Mod; layerId <= LayerId_Mouse; layerId++) {
        if (heldLayers[layerId] && heldLayer == LayerId_Base) {
            heldLayer = layerId;
        }
        heldLayers[layerId] = false;
    }
    if(previousHeldLayer != heldLayer) {
        updateActiveLayer();
    }
}

/**
 * Fork extensions:
 */
void LayerSwitcher_RecalculateLayerComposition() {
    updateActiveLayer();
}
