#ifndef __USB_REPORT_UPDATER_H__
#define __USB_REPORT_UPDATER_H__

// Includes:

    #include "layer.h"
    #include "config_parser/parse_keymap.h"

// Macros:

    #define ACTIVE_MOUSE_STATES_COUNT (SerializedMouseAction_Last + 1)

    #define IS_SECONDARY_ROLE_MODIFIER(secondaryRole) (SecondaryRole_LeftCtrl <= (secondaryRole) && (secondaryRole) <= SecondaryRole_RightSuper)
    #define IS_SECONDARY_ROLE_LAYER_SWITCHER(secondaryRole) (SecondaryRole_Mod <= (secondaryRole) && (secondaryRole) <= SecondaryRole_Mouse)
    #define SECONDARY_ROLE_MODIFIER_TO_HID_MODIFIER(secondaryRoleModifier) (1 << ((secondaryRoleModifier) - 1))
    #define SECONDARY_ROLE_LAYER_TO_LAYER_ID(secondaryRoleLayer) ((secondaryRoleLayer) - SecondaryRole_RightSuper)

    #define USB_SEMAPHORE_TIMEOUT 100 // ms

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
        SecondaryRole_Mouse
    } secondary_role_t;

    typedef enum {
        MouseSpeed_Normal,
        MouseSpeed_Accelerated,
        MouseSpeed_Decelerated,
    } mouse_speed_t;

    typedef struct {
        bool isScroll;
        bool wasMoveAction;
        serialized_mouse_action_t upState;
        serialized_mouse_action_t downState;
        serialized_mouse_action_t leftState;
        serialized_mouse_action_t rightState;
        mouse_speed_t prevMouseSpeed;
        float intMultiplier;
        float currentSpeed;
        float targetSpeed;
        uint8_t initialSpeed;
        uint8_t acceleration;
        uint8_t deceleratedSpeed;
        uint8_t baseSpeed;
        uint8_t acceleratedSpeed;
        float xSum;
        float ySum;
        int16_t xOut;
        int16_t yOut;
        int8_t verticalStateSign;
        int8_t horizontalStateSign;
    } mouse_kinetic_state_t;

// Variables:

    extern layer_id_t PreviousLayer;
    extern uint16_t DoubleTapSwitchLayerTimeout;
    extern mouse_kinetic_state_t MouseMoveState;
    extern mouse_kinetic_state_t MouseScrollState;
    extern uint32_t UsbReportUpdateCounter;
    extern volatile uint8_t UsbReportUpdateSemaphore;
    extern bool TestUsbStack;

// Functions:

    void UpdateUsbReports(void);

#endif
