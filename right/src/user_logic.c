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
#include "trace.h"

#ifdef __ZEPHYR__
#else
#include "stubs.h"
#endif

uint32_t UserLogic_LastEventloopTime = 0;

void RunUserLogic(void) {
    if (EventVector_IsSet(EventVector_ApplyConfig)) {
        Trace_Printc("l1");
        UsbCommand_ApplyConfigSync(NULL, NULL);
    }
    if (EventVector_IsSet(EventVector_KeymapReloadNeeded)) {
        Trace_Printc("l2");
        SwitchKeymapById(CurrentKeymapIndex, true);
    }
#ifndef __ZEPHYR__
    if (EventVector_IsSet(EventVector_ProtocolChanged)) {
        Trace_Printc("l3");
        UsbBasicKeyboard_HandleProtocolChange();
    }
#endif
    if (EventVector_IsSet(EventVector_UsbMacroCommandWaitingForExecution)) {
        Trace_Printc("l4");
        UsbMacroCommand_ExecuteSynchronously();
    }

    if (EventVector_IsSet(EventVector_KeyboardLedState)) {
        Trace_Printc("l5");
        MacroEvent_ProcessStateKeyEvents();
    }

    if (EventVector_IsSet(EventVector_ReportUpdateMask)) {
        Trace_Printc("l6");
        UpdateUsbReports();
    }

    if (EventVector_IsSet(EventVector_LedManagerFullUpdateNeeded)) {
        Trace_Printc("l7");
        LedManager_FullUpdate();
    }

    if (EventVector_IsSet(EventVector_LedMapUpdateNeeded)) {
        Trace_Printc("l8");
        Ledmap_UpdateBacklightLeds();
    }

    if (EventVector_IsSet(EventVector_SegmentDisplayNeedsUpdate)) {
        Trace_Printc("l9");
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
