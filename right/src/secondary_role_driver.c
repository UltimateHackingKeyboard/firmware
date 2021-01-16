#include "secondary_role_driver.h"
#include "postponer.h"
#include "led_display.h"
#include "timer.h"
#include "math.h"

static key_state_t* resolutionKey;
static secondary_role_state_t resolutionState;
static uint32_t resolutionStartTime;

static secondary_role_strategy_t strategy = SecondaryRoleStrategy_Timeout;

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

uint16_t timeoutStrategyTriggerTime = 200;
bool timeoutStrategyAllowTriggerByRelease = true;

static secondary_role_state_t resolveCurrentKeyRoleIfDontKnowTimeout()
{
    uint32_t dualRolePressTime = resolutionStartTime;
    struct postponer_buffer_record_type_t* dummy;
    struct postponer_buffer_record_type_t* dualRoleRelease;
    struct postponer_buffer_record_type_t* actionPress;
    struct postponer_buffer_record_type_t* actionRelease;

    PostponerQuery_InfoByKeystate(resolutionKey, &dummy, &dualRoleRelease);
    PostponerQuery_InfoByQueueIdx(0, &actionPress, &actionRelease);

    if(actionPress == NULL) {
        if(dualRoleRelease != NULL) {
            activatePrimary();
            return SecondaryRoleState_Primary;
        } else if(CurrentTime - dualRolePressTime > timeoutStrategyTriggerTime) {
            activatePrimary();
            return SecondaryRoleState_Primary;
        } else {
            return SecondaryRoleState_DontKnowYet;
        }
    }

    if(timeoutStrategyAllowTriggerByRelease) {
        if (actionRelease != NULL && (dualRoleRelease == NULL || actionRelease->time < dualRoleRelease->time)) {
            activateSecondary();
            return SecondaryRoleState_Secondary;
        }
    }

    uint32_t activeTime = (dualRoleRelease == NULL ? CurrentTime : dualRoleRelease->time) - dualRolePressTime;

    if ( activeTime > timeoutStrategyTriggerTime ) {
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

