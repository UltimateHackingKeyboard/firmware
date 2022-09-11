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

    #define IS_SECONDARY_ROLE_MODIFIER(secondaryRole) (SecondaryRole_LeftCtrl <= (secondaryRole) && (secondaryRole) <= SecondaryRole_RightSuper)
    #define IS_SECONDARY_ROLE_LAYER_SWITCHER(secondaryRole) (SecondaryRole_LayerFirst <= (secondaryRole) && (secondaryRole) <= SecondaryRole_LayerLast)
    #define SECONDARY_ROLE_MODIFIER_TO_HID_MODIFIER(secondaryRoleModifier) (1 << ((secondaryRoleModifier) - 1))
    #define SECONDARY_ROLE_LAYER_TO_LAYER_ID(secondaryRoleLayer) ((secondaryRoleLayer) - SecondaryRole_RightSuper)

// Typedefs:

    typedef enum {
        SecondaryRole_LeftCtrl = 1,
        SecondaryRole_LeftShift,
        SecondaryRole_LeftAlt,
        SecondaryRole_LeftSuper,
        SecondaryRole_RightCtrl,
        SecondaryRole_RightShift,
        SecondaryRole_RightAlt,
        SecondaryRole_RightSuper,
        SecondaryRole_Mod,
        SecondaryRole_Fn,
        SecondaryRole_Mouse,
        SecondaryRole_Fn2,
        SecondaryRole_Fn3,
        SecondaryRole_Fn4,
        SecondaryRole_Fn5,
        SecondaryRole_LayerFirst = SecondaryRole_Mod,
        SecondaryRole_LayerLast = SecondaryRole_Fn5,
    } secondary_role_t;

    typedef enum {
        Stick_Never,
        Stick_Smart,
        Stick_Always
    } sticky_strategy_t;

    typedef enum {
        SecondaryRoleState_DontKnowYet,
        SecondaryRoleState_Secondary,
        SecondaryRoleState_Primary,
    } secondary_role_state_t;

// Variables:

    extern secondary_role_t SecondaryRolePreview;

// Functions:

    secondary_role_state_t SecondaryRoles_ResolveState(key_state_t* keyState, secondary_role_t rolePreview);



#endif /* SRC_SECONDARY_ROLE_DRIVER_H_ */
