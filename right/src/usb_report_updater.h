#ifndef __USB_REPORT_UPDATER_H__
#define __USB_REPORT_UPDATER_H__

// Includes:

    #include "layer.h"
    #include "config_parser/parse_keymap.h"
    #include "secondary_role_driver.h"
    #include "key_states.h"
    #include "key_action.h"

// Macros:

    #define USB_SEMAPHORE_TIMEOUT 100 // ms

// Typedefs:

// Variables:

    extern uint32_t UsbReportUpdateCounter;
    extern volatile uint8_t UsbReportUpdateSemaphore;
    extern bool TestUsbStack;
    extern uint8_t InputModifiers;
    extern uint8_t InputModifiersPrevious;
    extern uint8_t OutputModifiers;
    extern bool SuppressMods;
    extern bool PostponeKeys;
    extern sticky_strategy_t StickyModifierStrategy;
    extern uint16_t KeystrokeDelay;
    extern bool PendingPostponedAndReleased;
    extern bool ActivateOnRelease;
    extern key_state_t* EmergencyKey;
    extern uint8_t basicScancodeIndex;
    extern uint8_t StickyModifiers;
    extern uint8_t StickyModifiersNegative;
    extern uint32_t UpdateUsbReports_LastUpdateTime;

// Functions:

    void UpdateUsbReports(void);
    void ToggleMouseState(serialized_mouse_action_t action, bool activate);
    void ActivateKey(key_state_t *keyState, bool debounce);
    void ActivateStickyMods(key_state_t *keyState, uint8_t mods);
    void ApplyKeyAction(key_state_t *keyState, key_action_cached_t *cachedAction, key_action_t *actionBase);

    void RecordKeyTiming_ReportKeystroke(key_state_t *keyState, bool active, uint32_t pressTime, uint32_t activationTime);

#endif
