#include "secondary_role_driver.h"
#include "postponer.h"
#include "led_display.h"
#include "timer.h"
#include "math.h"

/*
 * ## Strategies:
 *
 * - simple
 * - timeout (advanced alphanumeric-friendly)
 *
 * ## Simple strategy:
 *
 * Simple strategy works simply with press events (i.e., keyDown events). If there is any action pressed when a dual-role key is held, the secondary role goes to secondary role mode. Otherwise, it produces its primary action on its release event.
 *
 * ## Timeout strategy:
 *
 * Timeout strategy works mainly with key releases:
 *
 * - Secondary role is activated if timeout (i.e., triggerTime) is reached.
 * - Secondary role is activated if action key is pressed and released within the span of press of the dual role key. (If allowTriggerByRelease is active.)
 * - Long hold of primary action can be achieved by doubletapping the key. (If doubletapActivatedPrimary is active.)
 *
 * Configuration params:
 *
 * - DoubletapTime - if dual-role key is tapped and press within this time (press-to-press), then primary role is activated.
 * - TriggerTime configures the basic timeout of the dual-role key.
 * - TimeoutAction - after triggerTime, the key is defaulted to timeoutAction. Typically to secondary role.
 * - SafetyMargin decreases probability of unintentional secondary role, by offsetting release time of the keys by the amount.
 * - AllowTriggerByRelease - if set to false, the only way to trigger secondary role is to press it for more than triggerTime.
 * - DoubletapActivatesPrimary - if set to false, doubletap logic is disabled. Useful if you prefer tap and hold evaluate to primary role.
 *
 * This yields two most basic activation modes:
 *
 * - Via timeout. Triggered via distinct and slightly prolonged press of the shortcut.
 *   - Pros: works with short timeout.
 *   - Cons: some typing styles may cause unwanted activations of secondary role.
 *   - Recommended params:
 *     - TriggerTime = 200
 *     - SafetyMargin = 0
 *     - AllowTriggerByRelease = false
 *
 * - Via release order. Triggered mainly by correct release sequence.
 *   - Pros: is more reliable in suppressing unwanted secondary role activations.
 *   - Cons: activation might require a bit longer hold.
 *   - Recommended params:
 *     - TriggerTime = 350
 *     - SafetyMargin = 50
 *     - AllowTriggerByRelease = true
 */

static key_state_t* resolutionKey;
static secondary_role_state_t resolutionState;
static uint32_t resolutionStartTime;

static secondary_role_strategy_t strategy = SecondaryRoleStrategy_Simple;

static key_state_t* previousResolutionKey;
static uint32_t previousResolutionTime;

static void activatePrimary()
{
    // Activate the key "again", but now in "SecondaryRoleState_Primary".
    resolutionKey->current = true;
    resolutionKey->previous = false;
    resolutionKey->secondary = false;
    // Give the key two cycles (this and next) of activity before allowing postponer to replay any events (esp., the key's own release).
    PostponerCore_PostponeNCycles(1);
}

static void activateSecondary()
{
    // Activate the key "again", but now in "SecondaryRoleState_Secondary".
    resolutionKey->current = true;
    resolutionKey->previous = false;
    resolutionKey->secondary = true;
    // Let the secondary role take place before allowing the affected key to execute. Postponing rest of this cycle should suffice.
    PostponerCore_PostponeNCycles(0); //just for aesthetics - we are already postponed for this cycle so this is no-op
}


/*
 * Conservative settings (safely preventing collisions) (triggered via distinct && correct release sequence):
 * TriggerTime = 350
 * SafetyMargin = 50
 * AllowTriggerByRelease = true
 *
 * Less conservative (triggered via prolonged press of modifier):
 * TriggerTime = 200
 * SafetyMargin = 0
 * AllowTriggerByRelease = false
 */

uint16_t timeoutStrategyDoubletapTime = 200;
uint16_t timeoutStrategyTriggerTime = 350;
uint16_t timeoutStrategySafetyMargin = 50;
bool timeoutStrategyAllowTriggerByRelease = true;
bool timeoutStrategyDoubletapActivatesPrimary = true;
secondary_role_state_t timeoutStrategyTimeoutAction = SecondaryRoleState_Secondary;

static secondary_role_state_t resolveCurrentKeyRoleIfDontKnowTimeout()
{
    //gather data
    uint32_t dualRolePressTime = resolutionStartTime;
    struct postponer_buffer_record_type_t *dummy;
    struct postponer_buffer_record_type_t *dualRoleRelease;
    struct postponer_buffer_record_type_t *actionPress;
    struct postponer_buffer_record_type_t *actionRelease;

    PostponerQuery_InfoByKeystate(resolutionKey, &dummy, &dualRoleRelease);
    PostponerQuery_InfoByQueueIdx(0, &actionPress, &actionRelease);

    //handle doubletap logic
    if (
        timeoutStrategyDoubletapActivatesPrimary
        && resolutionKey == previousResolutionKey
        && resolutionStartTime - previousResolutionTime < timeoutStrategyDoubletapTime) {
        activatePrimary();
        return SecondaryRoleState_Primary;
    }

    //action key has not been pressed yet -> timeout scenarios
    if (actionPress == NULL) {
        if (dualRoleRelease != NULL) {
            activatePrimary();
            return SecondaryRoleState_Primary;
        } else if (CurrentTime - dualRolePressTime > timeoutStrategyTriggerTime) {
            switch (timeoutStrategyTimeoutAction) {
            case SecondaryRoleState_Primary:
                activatePrimary();
                return SecondaryRoleState_Primary;
            case SecondaryRoleState_Secondary:
                activateSecondary();
                return SecondaryRoleState_Secondary;
            default:
                return SecondaryRoleState_DontKnowYet;
            }
        } else {
            return SecondaryRoleState_DontKnowYet;
        }
    }

    //handle trigger by release
    if(timeoutStrategyAllowTriggerByRelease) {
        bool actionKeyWasReleasedButDualkeyNot = actionRelease != NULL && (dualRoleRelease == NULL && CurrentTime - actionRelease->time > timeoutStrategySafetyMargin);
        bool actionKeyWasReleasedFirst = actionRelease != NULL && (actionRelease->time < dualRoleRelease->time - timeoutStrategySafetyMargin);

        if (actionKeyWasReleasedFirst || actionKeyWasReleasedButDualkeyNot) {
            activateSecondary();
            return SecondaryRoleState_Secondary;
        }
    }

    uint32_t activeTime = (dualRoleRelease == NULL ? CurrentTime : dualRoleRelease->time) - dualRolePressTime;

    if ( activeTime > timeoutStrategyTriggerTime + timeoutStrategySafetyMargin ) {
        activateSecondary();
        return SecondaryRoleState_Secondary;
    } else {
        if ( dualRoleRelease != NULL ) {
            activatePrimary();
            return SecondaryRoleState_Primary;
        } else {
            return SecondaryRoleState_DontKnowYet;
        }
    }
}

static secondary_role_state_t resolveCurrentKeyRoleIfDontKnowSimple()
{
    if ( PostponerQuery_PendingKeypressCount() > 0 && !PostponerQuery_IsKeyReleased(resolutionKey) ) {
        activateSecondary();
        return SecondaryRoleState_Secondary;
    } else if ( PostponerQuery_IsKeyReleased(resolutionKey) /*assume PostponerQuery_PendingKeypressCount() == 0, but gather race conditions too*/ ) {
        activatePrimary();
        return SecondaryRoleState_Primary;
    } else {
        return SecondaryRoleState_DontKnowYet;
    }
}

static secondary_role_state_t resolveCurrentKey()
{
    switch (resolutionState) {
    case SecondaryRoleState_Primary:
    case SecondaryRoleState_Secondary:
        return resolutionState;
    case SecondaryRoleState_DontKnowYet:
        switch(strategy) {
        case SecondaryRoleStrategy_Simple:
            return resolveCurrentKeyRoleIfDontKnowSimple();
        default:
        case SecondaryRoleStrategy_Timeout:
            return resolveCurrentKeyRoleIfDontKnowTimeout();
        }
    default:
        return SecondaryRoleState_DontKnowYet; // prevent warning
    }
}

static secondary_role_state_t startResolution(key_state_t* keyState)
{
    previousResolutionKey = resolutionKey;
    previousResolutionTime = resolutionStartTime;
    resolutionKey = keyState;
    resolutionStartTime = CurrentPostponedTime;
    return SecondaryRoleState_DontKnowYet;
}

secondary_role_state_t SecondaryRoles_ResolveState(key_state_t* keyState)
{
    // Since postponer is active during resolutions, KeyState_ActivatedNow can happen only after previous
    // resolution has finished - i.e., if primary action has been activated, carried out and
    // released, or if previous resolution has been resolved as secondary. Therefore,
    // it suffices to deal with the `resolutionKey` only. Any other queried key is a finished resoluton.

    if ( KeyState_ActivatedNow(keyState) ) {
        //start new resolution
        resolutionState = startResolution(keyState);
        resolutionState = resolveCurrentKey();
        return resolutionState;
    } else {
        //handle old resolution
        if (keyState == resolutionKey) {
            resolutionState = resolveCurrentKey();
            return resolutionState;
        } else {
            return keyState->secondary ? SecondaryRoleState_Secondary : SecondaryRoleState_Primary;
        }
    }
}

