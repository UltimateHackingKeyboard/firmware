#include "key_history.h"
#include "secondary_role_driver.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "postponer.h"
#include "timer.h"
#include "utils.h"
#ifndef __ZEPHYR__
#include "led_display.h"
#endif
#include "math.h"
#include "debug.h"
#include "macros/key_timing.h"
#include "config_manager.h"
#include "event_scheduler.h"

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
 * - Secondary role is activated if timeout (i.e., Timeout) is reached.
 * - Secondary role is activated if action key is pressed and released within the span of press of the dual role key. (If triggeringEvent is Release.)
 * - Long hold of primary action can be achieved by doubletapping the key. (If doubletapActivatedPrimary is active.)
 *
 * Configuration params:
 *
 * - Timeout configures the basic timeout of the dual-role key.
 * - TimeoutAction - after Timeout, the key is defaulted to timeoutAction. Typically to secondary role.
 * - SafetyMargin decreases probability of unintentional secondary role, by offsetting release time of the keys by the amount.
 * - TriggeringEvent - dictates which action key action, if any, will trigger secondary role
 * - DoubletapTime - if dual-role key is tapped and press within this time (press-to-press), then primary role is activated.
 * - DoubletapToPrimary - if set to false, doubletap logic is disabled. Useful if you prefer tap and hold evaluate to primary role.
 * - MinimumHoldTime - The minimum time that a key must be held for in order to allow it to trigger as secondary.
 * - AcceptTriggersFromSameHalf - Whether or not action keys can be from the same half as the resolution key.
 *
 * This yields two most basic activation modes:
 *
 * - Via timeout. Triggered via distinct and slightly prolonged press of the shortcut.
 *   - Pros: works with short timeout.
 *   - Cons: some typing styles may cause unwanted activations of secondary role.
 *   - Recommended params:
 *     - Timeout = 200
 *     - SafetyMargin = 0
 *     - TriggeringEvent = None
 *
 * - Via release order. Triggered mainly by correct release sequence.
 *   - Pros: is more reliable in suppressing unwanted secondary role activations.
 *   - Cons: activation might require a bit longer hold.
 *   - Recommended params:
 *     - Timeout = 350
 *     - SafetyMargin = 50
 *     - TriggeringEvent = Release
 */

static bool resolutionCallerIsMacroEngine = false;
static bool acceptTriggersFromSameHalf = true;
static key_state_t *resolutionKey;
static uint32_t resolutionStartTime;
static bool isDoubletap = false;
static bool currentlyResolving = false;


static bool activateSecondaryImmediately;

static inline void fakeActivation()
{
    resolutionKey->current = true;
    resolutionKey->previous = false;
}

/*
 * Conservative settings (safely preventing collisions) (triggered via distinct && correct release sequence):
 * Timeout = 350
 * SafetyMargin = 50
 * TriggerByRelease = true
 *
 * Less conservative (triggered via prolonged press of modifier):
 * Timeout = 200
 * SafetyMargin = 0
 * TriggerByRelease = false
 */

static void sleepTimeoutStrategy(uint16_t wakeTimeOffset) {
    //register wakeups
    if (resolutionCallerIsMacroEngine) {
        Macros_SleepTillKeystateChange();
        Macros_SleepTillTime(resolutionStartTime + wakeTimeOffset, "SecondaryRoles - Macros");
    } else {
        EventScheduler_Schedule(resolutionStartTime + wakeTimeOffset, EventSchedulerEvent_NativeActions, "NativeActions - SecondaryRoles");
    }
}


#define RESOLVED(resolution)    KEY_TIMING(KeyTiming_RecordComment(resolutionKey, resolution, __LINE__))  \
                                return resolution;
#define AWAITEVENT(timeout)        sleepTimeoutStrategy(timeout);                                  \
                                return SecondaryRoleState_DontKnowYet;

static secondary_role_state_t resolveCurrentKeyRoleIfDontKnowTimeout()
{
    postponer_buffer_record_type_t *dummy;
    postponer_buffer_record_type_t *dualRoleRelease;
    PostponerQuery_InfoByKeystate(resolutionKey, &dummy, &dualRoleRelease);
    const int32_t activeTime = (dualRoleRelease == NULL ? Timer_GetCurrentTime() : dualRoleRelease->time) - resolutionStartTime;
    bool mouseSecondaryRequested = activateSecondaryImmediately;
    // sometimes, the balls in keycluster or trackball modules will trigger on vibration on keypress
    // for this reason, we ignore all such activations until the key has been pressed long enough to be allowed secondary
    // the request is reset here to prevent the unintentional vibration activation from just activating when minimum hold time is reached
    activateSecondaryImmediately = false;

    // handle things we can know without another key
    // doubletap and active timeout
    const bool reachedTimeout = activeTime > Cfg.SecondaryRoles_AdvancedStrategyTimeout;
    const bool isActiveTimeout = Cfg.SecondaryRoles_AdvancedStrategyTimeoutType == SecondaryRoleTimeoutType_Active;

    if (reachedTimeout) {
        if (isDoubletap && Cfg.SecondaryRoles_AdvancedStrategyDoubletapToPrimary) {
            RESOLVED(SecondaryRoleState_Primary);
        }
        if (isActiveTimeout) {
            RESOLVED(Cfg.SecondaryRoles_AdvancedStrategyTimeoutAction);
        }
    }

    // see if we have an activating key event
    const postponer_buffer_record_type_t *actionEvent = NULL;
    const key_state_t * const opposingKey = acceptTriggersFromSameHalf ? NULL : resolutionKey;

    if (Cfg.SecondaryRoles_AdvancedStrategyTriggeringEvent == SecondaryRoleTriggeringEvent_Release) {
        PostponerQuery_FindFirstReleased(&actionEvent, opposingKey);
    }
    else if (Cfg.SecondaryRoles_AdvancedStrategyTriggeringEvent == SecondaryRoleTriggeringEvent_Press) {
        PostponerQuery_FindFirstPressed(&actionEvent, opposingKey);
    }

    // handle positive safety margin part 1: is action key allowed to trigger secondary yet
    const bool safetyBlockSecondary = actionEvent != NULL 
        && (int32_t)(Timer_GetCurrentTime() - actionEvent->time) < Cfg.SecondaryRoles_AdvancedStrategySafetyMargin;
    if (safetyBlockSecondary) {
        // prevent the action from triggering secondary
        actionEvent = NULL;
    }

    // handle release of the dual role key
    const bool heldTooShortForSecondary = activeTime < Cfg.SecondaryRoles_AdvancedStrategyMinimumHoldTime;
    if (dualRoleRelease != NULL) {
        // released before it was allowed to become secondary
        // overrides any safety margin concerns
        if (heldTooShortForSecondary) {
            RESOLVED(SecondaryRoleState_Primary);
        }

        // released before another key triggered activation
        if (actionEvent == NULL) {
            // if safety margin requires us to wait for a bit
            // flipping signs to simplify checks
            const bool safetyWaitForAction = 
                (int32_t)(dualRoleRelease->time - Timer_GetCurrentTime()) > Cfg.SecondaryRoles_AdvancedStrategySafetyMargin;
            if (safetyWaitForAction) {
                AWAITEVENT(activeTime - Cfg.SecondaryRoles_AdvancedStrategySafetyMargin);
            }
            else {
                // see if passive timeout alters release action
                const secondary_role_state_t releaseAction = 
                    reachedTimeout ? Cfg.SecondaryRoles_AdvancedStrategyTimeoutAction : SecondaryRoleState_Primary;
                RESOLVED(releaseAction);
            }
        }
    }

    // now we want to trigger secondary, but are we allowed?
    // handle safety margin part 2: wait for the safety margin?
    const bool safetyWaitForRelease = safetyBlockSecondary && dualRoleRelease == NULL;
    uint32_t waitUntil = safetyWaitForRelease ? activeTime + Cfg.SecondaryRoles_AdvancedStrategySafetyMargin : 0;
    // wait for being allowed to trigger secondary?
    if (heldTooShortForSecondary) {
        // pick the longer wait time
        // we're waiting to trigger secondary once both timespans have passed, and the only thing
        // which can prevent that resolution is dual role release which triggers reevaluation on it's own
        waitUntil = MAX(Cfg.SecondaryRoles_AdvancedStrategyMinimumHoldTime, waitUntil);
    }
    // wait until we are allowed to go secondary
    if (waitUntil != 0) {
        AWAITEVENT(waitUntil);
    }

    // handle mouse activation
    if (mouseSecondaryRequested) {
        RESOLVED(SecondaryRoleState_Secondary);
    }

    // handle action key activation
    if (actionEvent != NULL) {
        RESOLVED(SecondaryRoleState_Secondary);
    }

    // see if we should set a timer to wake up to actively time out
    if (isActiveTimeout || isDoubletap) {
        AWAITEVENT(Cfg.SecondaryRoles_AdvancedStrategyTimeout);
    }

    // otherwise, keep postponing until key action
    return SecondaryRoleState_DontKnowYet;
}

static secondary_role_state_t resolveCurrentKeyRoleIfDontKnowSimple()
{
    if (activateSecondaryImmediately) {
        RESOLVED(SecondaryRoleState_Secondary);
    }
    if (PostponerQuery_PendingKeypressCount() > 0 && !PostponerQuery_IsKeyReleased(resolutionKey)) {
        RESOLVED(SecondaryRoleState_Secondary);
    } else if (PostponerQuery_IsKeyReleased(resolutionKey) /*assume PostponerQuery_PendingKeypressCount() == 0, but gather race conditions too*/) {
        RESOLVED(SecondaryRoleState_Primary);
    } else {
        if (resolutionCallerIsMacroEngine) {
            Macros_SleepTillKeystateChange();
        }
        return SecondaryRoleState_DontKnowYet;
    }
}

static secondary_role_state_t resolveCurrentKey(secondary_role_strategy_t strategy)
{
    switch (strategy) {
        case SecondaryRoleStrategy_Simple:
            return resolveCurrentKeyRoleIfDontKnowSimple();
        default:
        case SecondaryRoleStrategy_Advanced:
            return resolveCurrentKeyRoleIfDontKnowTimeout();
    }
}


static void startResolution(
    key_state_t *keyState,
    secondary_role_strategy_t strategy,
    bool isMacroResolution,
    secondary_role_same_half_t actionFromSameHalf)
{
    // store current state
    currentlyResolving = true;
    isDoubletap = KeyHistory_WasLastDoubletap();
    resolutionKey = keyState;
    resolutionStartTime = CurrentPostponedTime;
    resolutionCallerIsMacroEngine = isMacroResolution;
    activateSecondaryImmediately = false;
    switch (actionFromSameHalf) {
        case SecondaryRole_AcceptTriggersFromSameHalf:
            acceptTriggersFromSameHalf = true;
            break;
        case SecondaryRole_IgnoreTriggersFromSameHalf:
            acceptTriggersFromSameHalf = false;
            break;
        default:
            acceptTriggersFromSameHalf = Cfg.SecondaryRoles_AdvancedStrategyAcceptTriggersFromSameHalf;
            break;
    }

    switch (strategy) {
        case SecondaryRoleStrategy_Simple:
            if(Cfg.SecondaryRoles_AdvancedStrategyTriggerByMouse) {
                PostponerExtended_BlockMouse();
            }
            break;
        default:
        case SecondaryRoleStrategy_Advanced:
            if (Cfg.SecondaryRoles_AdvancedStrategyTriggerByMouse) {
                PostponerExtended_BlockMouse();
            }
            break;
    }
}

static void finishResolution(secondary_role_state_t res)
{
    resolutionKey->secondaryState = res;
    if(!resolutionCallerIsMacroEngine) {
        fakeActivation();
    }
    currentlyResolving = false;
    PostponerExtended_UnblockMouse();
}

void SecondaryRoles_ActivateSecondaryImmediately() {
    if (currentlyResolving && !activateSecondaryImmediately) {
        if (RecordKeyTiming) {
            Macros_SetStatusNumSpaced(Timer_GetCurrentTime(), false);
            Macros_SetStatusString(" Got resolve request.\n", NULL);
        }
        activateSecondaryImmediately = true;
        if (resolutionCallerIsMacroEngine) {
            Macros_WakeBecauseOfKeystateChange();
        }
        else {
            EventVector_Set(EventVector_NativeActions);
        }
    }
}

secondary_role_state_t SecondaryRoles_ResolveState(key_state_t* keyState, secondary_role_strategy_t strategy, bool isMacroResolution, secondary_role_same_half_t actionFromSameHalf)
{
    // Since postponer is active during resolutions, KeyState_ActivatedNow can happen only after previous
    // resolution has finished - i.e., if primary action has been activated, carried out and
    // released, or if previous resolution has been resolved as secondary. Therefore,
    // it suffices to deal with the `resolutionKey` only. Any other queried key is a finished resoluton.

    if (!currentlyResolving && keyState->secondaryState == SecondaryRoleState_DontKnowYet) {
        startResolution(keyState, strategy, isMacroResolution, actionFromSameHalf);
        secondary_role_state_t res = resolveCurrentKey(strategy);
        if (res != SecondaryRoleState_DontKnowYet) {
            finishResolution(res);
        }
        return res;
    }
    else {
        if (currentlyResolving && keyState == resolutionKey) {
            secondary_role_state_t res = resolveCurrentKey(strategy);
            if (res != SecondaryRoleState_DontKnowYet) {
                finishResolution(res);
            }
            return res;
        } else {
            return keyState->secondaryState;
        }
    }
}

