#ifndef __USB_REPORT_UPDATER_H__
#define __USB_REPORT_UPDATER_H__

// Macros:

    #define ACTIVE_MOUSE_STATES_COUNT (SerializedMouseAction_Last + 1)

    #define IS_SECONDARY_ROLE_MODIFIER(secondaryRole) (SecondaryRole_LeftCtrl <= (secondaryRole) && (secondaryRole) <= SecondaryRole_RightSuper)
    #define IS_SECONDARY_ROLE_LAYER_SWITCHER(secondaryRole) (SecondaryRole_Mod <= (secondaryRole) && (secondaryRole) <= SecondaryRole_Mouse)
    #define SECONDARY_ROLE_MODIFIER_TO_HID_MODIFIER(secondaryRoleModifier) (1 << ((secondaryRoleModifier) - 1))
    #define SECONDARY_ROLE_LAYER_TO_LAYER_ID(secondaryRoleLayer) ((secondaryRoleLayer) - SecondaryRole_RightSuper)

// Typedefs:

    typedef enum {
        SecondaryRoleState_Released,
        SecondaryRoleState_Pressed,
        SecondaryRoleState_Triggered,
    } secondary_role_state_t;

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
        SecondaryRole_Mouse
    } secondary_role_t;

    typedef enum {
        MouseSpeed_Normal,
        MouseSpeed_Accelerated,
        MouseSpeed_Decelerated,
    } mouse_speed_t;

// Functions:

    void UpdateUsbReports(void);

#endif
