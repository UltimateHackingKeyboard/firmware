#include "layer_switcher.h"
#include "keymap.h"
#include "layer.h"
#include "layer_stack.h"
#include "ledmap.h"
#include "macro_events.h"
#include "timer.h"
#include "macros/core.h"
#include "debug.h"
#include "led_display.h"
#include "usb_report_updater.h"
#include "config_manager.h"
#include "event_scheduler.h"

#ifdef __ZEPHYR__
#include "keyboard/oled/widgets/text_widget.h"
#include "keyboard/oled/widgets/widget_store.h"
#include "state_sync.h"
#endif


layer_id_t ActiveLayer = LayerId_Base;
bool ActiveLayerHeld = false;
uint8_t ActiveLayerModifierMask = 0;

static layer_id_t heldLayer = LayerId_None;


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
 * - if possible, secondary role holds behave as standard holds.
 *   Otherwise:
 *   - Secondary layer hold is superior to standard holds.
 *   - Secondary layer hold is triggered by press/release and behaves as standard actions.
 *
 */

// Recompute active layer whenever state of the layer locks changes.
void updateActiveLayer() {
    // beware lower-upper case typos!

    // apply stock layer switching
    layer_id_t previousLayer = ActiveLayer;
    layer_id_t activeLayer = LayerId_None;
    bool activeLayerHeld = false;

    // apply toggles and macro holds
    if(activeLayer == LayerId_None) {
        activeLayer = LayerStack_ActiveLayer;
        activeLayerHeld = LayerStack_ActiveLayerHeld;
    }

    // apply native holds
    if(activeLayer == LayerId_None || activeLayer == LayerId_Base) {
        activeLayer = heldLayer;
        activeLayerHeld = activeLayer != LayerId_None;
    }

    // apply default
    if(activeLayer == LayerId_None) {
        activeLayer = LayerId_Base;
    }

    //(write actual ActiveLayer atomically, so that random observer is not confused)
    ActiveLayer = activeLayer;
    ActiveLayerHeld = activeLayerHeld;

    if (ActiveLayer != previousLayer) {
#if DEVICE_HAS_OLED
        Widget_Refresh(&LayerWidget);
        Widget_Refresh(&KeymapLayerWidget);
#endif
#ifdef __ZEPHYR__
        StateSync_UpdateProperty(StateSyncPropertyId_ActiveLayer, NULL);
#endif
        LedDisplay_SetLayer(ActiveLayer);
        EventVector_Set(EventVector_LedMapUpdateNeeded);
        MacroEvent_OnLayerChange(activeLayer);
    }
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
        LayerStack_LegacyPop(layer);
        if (doubleTapSwitchLayerKey == keyState && Timer_GetElapsedTimeAndSetCurrent(&doubleTapSwitchLayerStartTime) < Cfg.DoubletapTimeout) {
            LayerStack_LegacyPush(layer);
            doubleTapSwitchLayerTriggerTime = CurrentTime;
            doubleTapSwitchLayerStartTime = CurrentTime;
        } else {
            doubleTapSwitchLayerKey = keyState;
            doubleTapSwitchLayerStartTime = CurrentTime;
        }
    }

    if(KeyState_DeactivatedNow(keyState)) {
        //If current press is too long, cancel current toggle
        if ( doubleTapSwitchLayerKey == keyState && Timer_GetElapsedTime(&doubleTapSwitchLayerTriggerTime) > Cfg.DoubletapSwitchLayerReleaseTimeout)
        {
            LayerStack_LegacyPop(layer);
        }
    }
}

// If some other key is pressed between taps of a possible doubletap, discard the doubletap
// Also, doubleTapSwitchKey is used to cancel long hold toggle, so reset it only if no layer is locked
void LayerSwitcher_DoubleTapInterrupt(key_state_t* keyState) {
    bool noLayerIsToggled = LayerStack_Size == 1;
    if (doubleTapSwitchLayerKey != keyState && noLayerIsToggled) {
        doubleTapSwitchLayerKey = NULL;
    }
}

void LayerSwitcher_ToggleLayer(layer_id_t layer) {
    if(LayerStack[LayerStack_TopIdx].held == false && LayerStack[LayerStack_TopIdx].layer == layer) {
        LayerStack_LegacyPop(layer);
    } else {
        LayerStack_LegacyPush(layer);
    }
}


void LayerSwitcher_UnToggleLayerOnly(layer_id_t layer) {
    LayerStack_LegacyPop(layer);
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
    if(heldLayer == LayerId_None || forceSwap) {
        heldLayer = layer;
        updateActiveLayer();
    }
}

// If modifier layer is active, maskOutput will be used as a negative sticky modifier
// mask in order to negate effect of the activation modifier.
static bool layerMeetsHoldConditions(uint8_t layer, uint8_t* maskOutput) {
    if (layer == LayerStack_ActiveLayer && LayerStack_ActiveLayer != LayerId_None && LayerStack_ActiveLayerHeld) {
        return true;
    }
    if (heldLayers[layer]) {
        return true;
    }
    layer_config_t* cfg = &Cfg.LayerConfig[layer];
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
void LayerSwitcher_UpdateHeldLayer() {
    EventVector_Unset(EventVector_LayerHolds);
    layer_id_t previousHeldLayer = heldLayer;

    // Reset held layer if no longer relevant
    if (heldLayer != LayerId_None && !layerMeetsHoldConditions(heldLayer, NULL)) {
        heldLayer = LayerId_None;
    }

    // Find first relevant layer, and reset the array
    for (layer_id_t layerId = LayerId_Base; layerId < LayerId_Count; layerId++) {
        // normal layer should take precedence over modifier layers
        bool heldLayerCanBeOverriden = heldLayer == LayerId_None || (IS_MODIFIER_LAYER(heldLayer) && !IS_MODIFIER_LAYER(layerId));
        if (heldLayerCanBeOverriden && layerMeetsHoldConditions(layerId, &ActiveLayerModifierMask)) {
            heldLayer = layerId;
        }
    }
    if (!Cfg.LayerConfig[heldLayer].modifierLayerMask) {
        ActiveLayerModifierMask = 0;
    }
    if (previousHeldLayer != heldLayer) {
        updateActiveLayer();
    }
}

void LayerSwitcher_ResetHolds() {
    for (uint8_t i = 0; i < LayerId_Count; i++) {
        heldLayers[i] = false;
    }
}

/**
 * Fork extensions:
 */
void LayerSwitcher_RecalculateLayerComposition() {
    updateActiveLayer();
}
