#ifndef SRC_SECONDARY_ROLE_DRIVER_H_
#define SRC_SECONDARY_ROLE_DRIVER_H_

/*
 * Secondary role driver's purpose is to deal with deciding whether primary action
 * or secondary role of a key should be used. It also does some auxiliary work, but
 * it does not carry out the actions. Once the primary/secondary question has been
 * decided, actions are carried out as usual in usb_report updater.
 *
 * Resolution roughly goes as as this:
 * - start in DontKnowYet state
 * - initiate postponer - from this point on, all keypresses are queued rather than executed
 * - look into postponer queue for as long as needed
 * - when decided, change to the corresponding state and activate the corresponding role
 * - once postponer's cycles_until_activation reach zero, postponer itself will start replaying
 *   the affected keys (e.g., action keys on a "secondary" layer)
 */

// Includes:

    #include "key_states.h"

// Macros:

// Typedefs:

    typedef enum {
        SecondaryRoleState_DontKnowYet,
        SecondaryRoleState_Secondary,
        SecondaryRoleState_Primary,
    } secondary_role_state_t;


// Functions:

    secondary_role_state_t SecondaryRoles_ResolveState(key_state_t* keyState);



#endif /* SRC_SECONDARY_ROLE_DRIVER_H_ */
