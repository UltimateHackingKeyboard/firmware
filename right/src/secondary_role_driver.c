#include "secondary_role_driver.h"
#include "postponer.h"

key_state_t* resolutionKey;
secondary_role_state_t resolutionState;

uint8_t secondaryRoleLayer;
key_state_t* secondaryRoleLayerKey;


static void activatePrimary()
{
    //ensure that the key is active in Primary state for at least next two cycles.
    PostponerCore_PostponeNCycles(POSTPONER_MIN_CYCLES_PER_ACTIVATION);
    resolutionKey->current = true;
    resolutionKey->previous = false;
}

static void activateSecondary()
{
    resolutionKey->current = true;
    resolutionKey->previous = false;
}

static secondary_role_state_t resolveCurrentKeyRoleIfDontKnow()
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
        return resolveCurrentKeyRoleIfDontKnow();
    default:
        return SecondaryRoleState_DontKnowYet; // prevent warning
    }
}

static secondary_role_state_t startResolution(key_state_t* keyState)
{
    resolutionKey = keyState;
    return SecondaryRoleState_DontKnowYet;
}

secondary_role_state_t SecondaryRoles_ResolveState(key_state_t* keyState)
{
    // Since postponer is active during resolutions, KeyState_ActivatedNow can happen only after previous
    // resolution has finished - i.e., if primary action has been activated, carried out and
    // released, or if previous resolution has been resolved as secondary. Therefore,
    // it suffices to deal with the `resolutionKey` only. Any other queried key is an active secondary role.

    if ( KeyState_ActivatedNow(keyState) ) {
        resolutionState = startResolution(keyState);
        resolutionState = resolveCurrentKey();
        return resolutionState;
    } else {
        if (keyState == resolutionKey) {
            resolutionState = resolveCurrentKey();
            return resolutionState;
        } else {
            return SecondaryRoleState_Secondary;
        }
    }
}

