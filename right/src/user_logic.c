#include "user_logic.h"
#include "keymap.h"
#include "ledmap.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "segment_display.h"
#include "usb_commands/usb_command_exec_macro_command.h"
#include "usb_report_updater.h"
#include "macro_events.h"
#include "debug.h"
#include "event_scheduler.h"
#include "user_logic.h"
#include "led_manager.h"
#include "usb_commands/usb_command_apply_config.h"

void RunUserLogic(void) {
    if (EventVector_IsSet(EventVector_ApplyConfig)) {
        UsbCommand_ApplyConfig();
    }
    if (EventVector_IsSet(EventVector_KeymapReloadNeeded)) {
        SwitchKeymapById(CurrentKeymapIndex);
    }
#ifndef __ZEPHYR__
    if (EventVector_IsSet(EventVector_ProtocolChanged)) {
        UsbBasicKeyboard_HandleProtocolChange();
    }
#endif
    if (EventVector_IsSet(EventVector_UsbMacroCommandWaitingForExecution)) {
        UsbMacroCommand_ExecuteSynchronously();
    }

    if (EventVector_IsSet(EventVector_KeyboardLedState)) {
        MacroEvent_ProcessStateKeyEvents();
    }

    if (EventVector_IsSet(EventVector_ReportUpdateMask)) {
        UpdateUsbReports();
    }

    if (EventVector_IsSet(EventVector_LedManagerFullUpdateNeeded)) {
        LedManager_FullUpdate();
    }

    if (EventVector_IsSet(EventVector_LedMapUpdateNeeded)) {
        Ledmap_UpdateBacklightLeds();
    }

    if (EventVector_IsSet(EventVector_SegmentDisplayNeedsUpdate)) {
        SegmentDisplay_Update();
    }

    LOG_SCHEDULE(
        EventVector_ReportMask("=== ", EventScheduler_Vector)
    );
}

void RunUhk80LeftHalfLogic() {
    if (EventVector_IsSet(EventVector_LedMapUpdateNeeded)) {
        Ledmap_UpdateBacklightLeds();
    }
}
