#include "layer_switcher.h"
#include "layer.h"
#include "ledmap.h"
#include "timer.h"
#include "macros.h"
#include "debug.h"
#include "led_display.h"
#include "usb_report_updater.h"

uint16_t DoubleTapSwitchLayerTimeout = 400;
uint16_t DoubleTapSwitchLayerReleaseTimeout = 200;

layer_id_t ActiveLayer = LayerId_Base;
bool ActiveLayerHeld = false;
uint8_t ActiveLayerModifierMask = 0;

#define NONE 0xFF

static layer_id_t toggledLayer = NONE;
static layer_id_t heldLayer = NONE;


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
    // beware lower-upper case typos!

    // apply stock layer switching
    layer_id_t activeLayer = NONE;
    bool activeLayerHeld = false;
    if(activeLayer == NONE) {
        activeLayer = heldLayer;
    }
    if(activeLayer == NONE) {
        activeLayer = toggledLayer;
    }
    activeLayerHeld = activeLayer == heldLayer && activeLayer != NONE;

    // apply macro engine layer
    if(activeLayer == NONE) {
        activeLayer = Macros_ActiveLayer;
        activeLayerHeld = Macros_ActiveLayerHeld;
    }

    // apply default
    if(activeLayer == NONE) {
        activeLayer = LayerId_Base;
    }

    //(write actual ActiveLayer atomically, so that random observer is not confused)
    ActiveLayer = activeLayer;
    ActiveLayerHeld = activeLayerHeld;
    UpdateLayerLeds();
    LedDisplay_SetLayer(ActiveLayer);
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
        toggledLayer = NONE;
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
            toggledLayer = NONE;
            updateActiveLayer();
        }
    }
}

// If some other key is pressed between taps of a possible doubletap, discard the doubletap
// Also, doubleTapSwitchKey is used to cancel long hold toggle, so reset it only if no layer is locked
void LayerSwitcher_DoubleTapInterrupt(key_state_t* keyState) {
    if (doubleTapSwitchLayerKey != keyState && toggledLayer == NONE) {
        doubleTapSwitchLayerKey = NULL;
    }
}

void LayerSwitcher_ToggleLayer(layer_id_t layer) {
    if(toggledLayer == NONE) {
        toggledLayer = layer;
    } else {
        toggledLayer = NONE;
    }
    updateActiveLayer();
}


void LayerSwitcher_UnToggleLayerOnly(layer_id_t layer) {
    if (toggledLayer != NONE) {
        toggledLayer = NONE;
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

static bool heldLayers[LayerId_Count];

// Called by pressed hold-layer keys during every cycle
void LayerSwitcher_HoldLayer(layer_id_t layer, bool forceSwap) {
    heldLayers[layer] = true;
    //no other switcher is active, so we may as well activate it straight away.
    if(heldLayer == NONE || forceSwap) {
        heldLayer = layer;
        updateActiveLayer();
    }
}

// If modifier layer is active, maskOutput will be used as a negative sticky modifier
// mask in order to negate effect of the activation modifier.
static bool layerMeetsHoldConditions(uint8_t layer, uint8_t* maskOutput) {
    if (heldLayers[layer]) {
        return true;
    }
    layer_config_t* cfg = &LayerConfig[layer];
    if (cfg->layerIsDefined && cfg->modifierLayerMask) {
        uint8_t maskOverlap = cfg->modifierLayerMask & InputModifiersPrevious;
        if (maskOverlap == cfg->modifierLayerMask || (!cfg->exactModifierMatch && maskOverlap)) {
            if (maskOutput != NULL) {
                *maskOutput = maskOverlap;
            }
            return true;
        }
    }
    return false;
}

// Gathers states set by LayerSwitcher_HoldLayer during previous update cycle and updates heldLayer.
void LayerSwitcher_UpdateActiveLayer() {
    layer_id_t previousHeldLayer = heldLayer;

    // Include macro held layer into computation
    if (Macros_ActiveLayer != NONE && Macros_ActiveLayerHeld) {
        heldLayers[Macros_ActiveLayer] = true;
    }

    // Reset held layer if no longer relevant
    if (heldLayer != NONE && !layerMeetsHoldConditions(heldLayer, NULL)) {
        heldLayer = NONE;
    }

    // Find first relevant layer, and reset the array
    for (layer_id_t layerId = LayerId_Base; layerId < LayerId_Count; layerId++) {
        // normal layer should take precedence over modifier layers
        bool heldLayerCanBeOverriden = heldLayer == NONE || (IS_MODIFIER_LAYER(heldLayer) && !IS_MODIFIER_LAYER(layerId));
        if (heldLayerCanBeOverriden && layerMeetsHoldConditions(layerId, &ActiveLayerModifierMask)) {
            heldLayer = layerId;
        }
        heldLayers[layerId] = false;
    }
    if (!LayerConfig[heldLayer].modifierLayerMask) {
        ActiveLayerModifierMask = 0;
    }
    if (previousHeldLayer != heldLayer) {
        updateActiveLayer();
    }
}

/**
 * Fork extensions:
 */
void LayerSwitcher_RecalculateLayerComposition() {
    updateActiveLayer();
}
