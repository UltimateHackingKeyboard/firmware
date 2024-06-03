#include "user_logic.h"
#include "keymap.h"
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

void RunUserLogic(void) {
    if (KeymapReloadNeeded) {
        SwitchKeymapById(CurrentKeymapIndex);
    }
    if (LedManager_FullUpdateNeeded) {
        LedManager_FullUpdate();
    }
#ifndef __ZEPHYR__
    if (UsbBasicKeyboard_ProtocolChanged) {
        UsbBasicKeyboard_HandleProtocolChange();
    }
#endif
    if (UsbMacroCommandWaitingForExecution) {
        UsbMacroCommand_ExecuteSynchronously();
    }
    if (MacroEvent_ScrollLockStateChanged || MacroEvent_NumLockStateChanged || MacroEvent_CapsLockStateChanged) {
        MacroEvent_ProcessStateKeyEvents();
    }

    UpdateUsbReports();

    if (EventScheduler_IsActive) {
        EventScheduler_Process();
    }
    if (SegmentDisplay_NeedsUpdate) {
        SegmentDisplay_Update();
    }
}
