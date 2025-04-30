#include "user_logic.h"
#include "keymap.h"
#include "layer_stack.h"
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
#ifdef __ZEPHYR__
#include "trace.h"
#else
#include "stubs.h"
#endif

void RunUserLogic(void) {
    if (EventVector_IsSet(EventVector_ApplyConfig)) {
        Trace_Printf("l1");
        UsbCommand_ApplyConfig(NULL, NULL);
    }
    if (EventVector_IsSet(EventVector_KeymapReloadNeeded)) {
        Trace_Printf("l2");
        SwitchKeymapById(CurrentKeymapIndex, true);
    }
#ifndef __ZEPHYR__
    if (EventVector_IsSet(EventVector_ProtocolChanged)) {
        Trace_Printf("l3");
        UsbBasicKeyboard_HandleProtocolChange();
    }
#endif
    if (EventVector_IsSet(EventVector_UsbMacroCommandWaitingForExecution)) {
        Trace_Printf("l4");
        UsbMacroCommand_ExecuteSynchronously();
    }

    if (EventVector_IsSet(EventVector_KeyboardLedState)) {
        Trace_Printf("l5");
        MacroEvent_ProcessStateKeyEvents();
    }

    if (EventVector_IsSet(EventVector_ReportUpdateMask)) {
        Trace_Printf("l6");
        UpdateUsbReports();
    }

    if (EventVector_IsSet(EventVector_LedManagerFullUpdateNeeded)) {
        Trace_Printf("l7");
        LedManager_FullUpdate();
    }

    if (EventVector_IsSet(EventVector_LedMapUpdateNeeded)) {
        Trace_Printf("l8");
        Ledmap_UpdateBacklightLeds();
    }

    if (EventVector_IsSet(EventVector_SegmentDisplayNeedsUpdate)) {
        Trace_Printf("l9");
        SegmentDisplay_Update();
    }

    LOG_SCHEDULE(
        EventVector_ReportMask("=== ", EventScheduler_Vector)
    );
}

void RunUhk80LeftHalfLogic() {
    if (EventVector_IsSet(EventVector_LedManagerFullUpdateNeeded)) {
        LedManager_FullUpdate();
    }
    if (EventVector_IsSet(EventVector_LedMapUpdateNeeded)) {
        Ledmap_UpdateBacklightLeds();
    }

    EventVector_Unset(EventVector_KeyboardLedState);

    LOG_SCHEDULE(
        EventVector_ReportMask("=== ", EventScheduler_Vector)
    );
    if (EventScheduler_Vector & EventVector_UserLogicUpdateMask & (~EventVector_NewMessage)) {
        EventVector_ReportMask("Warning: following event hasn't been unset: ", EventScheduler_Vector & EventVector_UserLogicUpdateMask);
    }
}
