#include "event_scheduler.h"
#include "timer.h"
#include "macros/core.h"
#include "macro_recorder.h"
#include "utils.h"
#include "stubs.h"
#include "postponer.h"
#include "debug.h"
#include "led_manager.h"
#include "slave_drivers/uhk_module_driver.h"
#include "peripherals/merge_sensor.h"

#ifdef __ZEPHYR__
#include "keyboard/oled/screens/screen_manager.h"
#include "keyboard/oled/oled.h"
#include "keyboard/charger.h"
#include "keyboard/uart.h"
#include "main.h"
#else
#include "segment_display.h"
#endif

static uint32_t times[EventSchedulerEvent_Count] = {};
static const char* labels[EventSchedulerEvent_Count] = {};
static event_scheduler_event_t nextEvent;
static uint32_t nextEventAt;

uint32_t EventScheduler_Vector = 0;

static void scheduleNext()
{
    bool gotAny = false;
    for (uint8_t i = 0; i < EventSchedulerEvent_Count; i++) {
        if (times[i] != 0 && (!gotAny || times[i] < nextEventAt)) {
            gotAny = true;
            nextEvent = i;
            nextEventAt = times[i];
        }
    }
    EventVector_SetValue(EventVector_EventScheduler, gotAny);
}

static void processEvt(event_scheduler_event_t evt)
{
    switch (evt) {
        case EventSchedulerEvent_UpdateBattery:
            Charger_UpdateBatteryState();
            break;
        case EventSchedulerEvent_ShiftScreen:
            Oled_ShiftScreen();
            break;
        case EventSchedulerEvent_SwitchScreen:
            ScreenManager_SwitchScreenEvent();
            break;
        case EventSchedulerEvent_SegmentDisplayUpdate:
            EventVector_Set(EventVector_SegmentDisplayNeedsUpdate);
            break;
        case EventSchedulerEvent_MacroWakeOnTime:
            Macros_WakedBecauseOfTime = true;
            EventVector_Set(EventVector_MacroEngine);
            break;
        case EventSchedulerEvent_MacroRecorderFlashing:
            MacroRecorder_UpdateRecordingLed();
            break;
        case EventSchedulerEvent_Postponer:
            EventVector_Set(EventVector_Postponer);
            break;
        case EventSchedulerEvent_NativeActions:
            EventVector_Set(EventVector_NativeActions);
            break;
        case EventSchedulerEvent_AgentLed:
            LedManager_UpdateAgentLed();
            break;
        case EventSchedulerEvent_UpdateLedSleepModes:
            LedManager_UpdateSleepModes();
            break;
        case EventSchedulerEvent_MouseController:
            EventVector_Set(EventVector_MouseController);
            break;
        case EventSchedulerEvent_ReenableUart:
            Uart_Enable();
            break;
        case EventSchedulerEvent_ModuleConnectionStatusUpdate:
            UhkModuleSlaveDriver_UpdateConnectionStatus();
            break;
        case EventSchedulerEvent_UpdateMergeSensor:
            MergeSensor_Update();
            break;
        default:
            return;
    }
}


void EventScheduler_Reschedule(uint32_t at, event_scheduler_event_t evt, const char* label)
{
    LOG_SCHEDULE(
        printk("%c", PostponerCore_EventsShouldBeQueued() ? 'P' : ' ');
        printk("    !!! Replan: %s\n", label);
    )
    times[evt] = at;
    labels[evt] = label;
    if (nextEvent == evt) {
        scheduleNext();
    }
    if (at < nextEventAt || !EventVector_IsSet(EventVector_EventScheduler) || nextEvent == evt) {
        nextEventAt = at;
        nextEvent = evt;
        EventVector_Set(EventVector_EventScheduler);
#ifdef __ZEPHYR__
        k_wakeup(Main_ThreadId);
#endif
    }
}

void EventScheduler_Schedule(uint32_t at, event_scheduler_event_t evt, const char* label)
{
    LOG_SCHEDULE(
        printk("%c", PostponerCore_EventsShouldBeQueued() ? 'P' : ' ');
        printk("    !!! Plan: %s\n", label);
    );
    if (times[evt] == 0 || at < times[evt]) {
        times[evt] = at;
        labels[evt] = label;
    }
    if (at < nextEventAt || !EventVector_IsSet(EventVector_EventScheduler)) {
        nextEventAt = at;
        nextEvent = evt;
        EventVector_Set(EventVector_EventScheduler);
#ifdef __ZEPHYR__
        k_wakeup(Main_ThreadId);
#endif
    }
}

void EventScheduler_Unschedule(event_scheduler_event_t evt)
{
    times[evt] = 0;
    labels[evt] = NULL;

    if (nextEvent == evt) {
        scheduleNext();
    }
}

uint32_t EventScheduler_Process()
{
    while (EventVector_IsSet(EventVector_EventScheduler) && nextEventAt <= CurrentTime) {
        event_scheduler_event_t evt = nextEvent;
        LOG_SCHEDULE(
            if (labels[evt] != NULL) {
                printk("%c", PostponerCore_EventsShouldBeQueued() ? 'P' : ' ');
                printk("!!! Scheduled event: %s\n", labels[evt]);
            }
        );
        times[nextEvent] = 0;
        labels[nextEvent] = NULL;
        scheduleNext();
        processEvt(evt);
    }
    return nextEventAt;
}

void EventVector_ReportMask(const char* prefix, uint32_t mask) {
#ifdef __ZEPHYR__
#define REPORT_MASK(NAME) if (mask & EventVector_##NAME) {  \
    printk("%s ", #NAME);  \
    mask &= ~EventVector_##NAME;  \
}

    printk("%c", PostponerCore_EventsShouldBeQueued() ? 'P' : ' ');
    printk("%s", prefix);

    REPORT_MASK(MacroEngine);
    REPORT_MASK(StateMatrix);
    REPORT_MASK(NativeActions);
    REPORT_MASK(MouseKeys);
    REPORT_MASK(MouseController);
    REPORT_MASK(Postponer);
    REPORT_MASK(LayerHolds);
    REPORT_MASK(EventScheduler);

    REPORT_MASK(KeyboardLedState);
    REPORT_MASK(UsbMacroCommandWaitingForExecution);
    REPORT_MASK(ProtocolChanged);
    REPORT_MASK(LedManagerFullUpdateNeeded);
    REPORT_MASK(KeymapReloadNeeded);
    REPORT_MASK(SegmentDisplayNeedsUpdate);
    REPORT_MASK(LedMapUpdateNeeded);

    REPORT_MASK(NativeActionReportsUsed);
    REPORT_MASK(MacroReportsUsed);
    REPORT_MASK(MouseKeysReportsUsed);
    REPORT_MASK(MouseControllerMouseReportsUsed);
    REPORT_MASK(MouseControllerKeyboardReportsUsed);
    REPORT_MASK(ReportsChanged);
    REPORT_MASK(NativeActionsPostponing);
    REPORT_MASK(MacroEnginePostponing);

    printk("\n");

    if (mask != 0) {
        printk("Unknown event vector bits: %u\n", mask);
    }
#undef REPORT_MASK
#endif
}

void EventVector_Init() {
    if (DEVICE_IS_UHK60) {
        EventScheduler_Schedule(1000, EventSchedulerEvent_AgentLed, "Agent led");
    }
    EventScheduler_Schedule(1000, EventSchedulerEvent_UpdateLedSleepModes, "Update led sleep modes");
}
