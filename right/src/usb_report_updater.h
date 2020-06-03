#ifndef __USB_REPORT_UPDATER_H__
#define __USB_REPORT_UPDATER_H__

// Includes:

    #include "layer.h"
    #include "config_parser/parse_keymap.h"
    #include "key_states.h"

// Macros:

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
        Stick_Never,
        Stick_Smart,
        Stick_Always
    } sticky_strategy_t;

// Variables:

    extern uint32_t UsbReportUpdateCounter;
    extern volatile uint8_t UsbReportUpdateSemaphore;
    extern bool TestUsbStack;
    extern uint8_t HardwareModifierState;
    extern uint8_t HardwareModifierStatePrevious;
    extern bool SuppressMods;
    extern bool PostponeKeys;
    extern sticky_strategy_t StickyModifierStrategy;
    extern uint16_t KeystrokeDelay;
    extern bool PendingPostponedAndReleased;
    extern bool ActivateOnRelease;
    extern key_state_t* EmergencyKey;

// Functions:

    void UpdateUsbReports(void);
    void ToggleMouseState(serialized_mouse_action_t action, bool activate);
    void ActivateKey(key_state_t *keyState, bool debounce);
    void ActivateStickyMods(key_state_t *keyState, uint8_t mods);

#endif
