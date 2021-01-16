#include "secondary_role_driver.h"
#include "postponer.h"
#include "led_display.h"

static key_state_t* resolutionKey;
static secondary_role_state_t resolutionState;

static void activatePrimary()
{
    // Activate the key "again", but now in "SecondaryRoleState_Primary".
    resolutionKey->current = true;
    resolutionKey->previous = false;
    // Give the key two cycles (this and next) of activity before allowing postponer to replay any events (esp., the key's own release).
    PostponerCore_PostponeNCycles(1);
}

static void activateSecondary()
{
    // Activate the key "again", but now in "SecondaryRoleState_Secondary".
    resolutionKey->current = true;
    resolutionKey->previous = false;
    // Let the secondary role take place before allowing the affected key to execute. Postponing rest of this cycle should suffice.
    PostponerCore_PostponeNCycles(0); //just for aesthetics - we are already postponed for this cycle so this is no-op
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
            return SecondaryRoleState_Secondary;
        }
    }
}

