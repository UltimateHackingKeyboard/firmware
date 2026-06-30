#ifndef __USB_REPORT_UPDATER_H__
#define __USB_REPORT_UPDATER_H__

// Includes:

    #include "hid/keyboard_report.h"
    #include "hid/controls_report.h"
    #include "hid/mouse_report.h"
    #include "layer.h"
    #include "config_parser/parse_keymap.h"
    #include "secondary_role_driver.h"
    #include "key_states.h"
    #include "key_action.h"

// Typedefs:

    typedef struct {
        // these are metadata that should never be zeroed!
        uint32_t recomputeStateVectorMask; // mask that should be set in order to recompute them in the next update
        uint32_t reportsUsedVectorMask; // mask that indicates that these reports were used and should be merged
        uint32_t postponeMask; // mask that indicates that these reports are initiating postponing
        // these are the working data
        hid_keyboard_report_t basic;
        hid_controls_report_t controls;
        uint8_t inputModifiers;
    } ATTR_PACKED usb_keyboard_reports_t;

// Variables:

    extern uint32_t UsbReportUpdateCounter;
    extern bool TestUsbStack;
    extern uint8_t InputModifiers;
    extern uint8_t InputModifiersPrevious;
    extern uint8_t OutputModifiers;
    extern bool SuppressMods;
    extern bool PendingPostponedAndReleased;
    extern uint8_t basicScancodeIndex;
    extern uint8_t StickyModifiers;
    extern uint8_t StickyModifiersNegative;
    extern uint32_t UpdateUsbReports_LastUpdateTime;
    extern usb_keyboard_reports_t NativeKeyboardReports;
    extern uint32_t LastUsbGetKeyboardStateRequestTimestamp;
    extern uint32_t UsbReportUpdater_LastActivityTime;
    extern uint32_t UsbReportUpdater_LastMouseActivityTime;

    // Report double-buffers + their active-side pointers, exposed for the report sender (which
    // compares both halves for changes, sends the active report, and switches the baseline).
    extern hid_keyboard_report_t keyboardReports[2];
    extern hid_controls_report_t controlsReports[2];
    extern hid_mouse_report_t mouseReports[2];
    extern hid_keyboard_report_t* ActiveKeyboardReport;
    extern hid_controls_report_t* ActiveControlsReport;
    extern hid_mouse_report_t* ActiveMouseReport;

// Functions:

    key_action_cached_t* RetrieveCurrentActiveAction(key_state_t *keyState);

    void ToggleMouseState(serialized_mouse_action_t action, bool activate);
    void ActivateKey(key_state_t *keyState, bool debounce);
    void ActivateStickyMods(key_state_t *keyState, uint8_t mods);
    void ApplyKeyAction(key_state_t *keyState, key_action_cached_t *cachedAction, key_action_t *actionBase, usb_keyboard_reports_t* reports);
    void StickyMods_ResetLater(key_action_cached_t *cachedAction);

    void UsbReportUpdater_ResetKeyboardReports(usb_keyboard_reports_t* reports);
    void RecordKeyTiming_ReportKeystroke(key_state_t *keyState, bool active, uint32_t pressTime, uint32_t activationTime);

    hid_keyboard_report_t* GetInactiveKeyboardReport(void);

    // Advance a report's double-buffer baseline. Called by the report sender on delivery
    // confirmation (so the just-sent report becomes the new baseline for change detection).
    void UsbReportUpdater_SwitchActiveKeyboardReport(void);
    void UsbReportUpdater_SwitchActiveMouseReport(void);
    void UsbReportUpdater_SwitchActiveControlsReport(void);

    // Report construction, exposed for the report sender's orchestration loop.
    void UsbReportUpdater_UpdateActiveReports(void);
    void UsbReportUpdater_MergeReports(void);
    void justPreprocessInput(bool forcePostponer);

#endif
