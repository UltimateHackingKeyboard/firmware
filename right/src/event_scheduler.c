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
#include "power_mode.h"
#include "oneshot.h"

#ifdef __ZEPHYR__
#include "round_trip_test.h"
#include "keyboard/oled/screens/screen_manager.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/uart_modules.h"
#include "keyboard/oled/oled.h"
#include "keyboard/charger.h"
#include "keyboard/uart_bridge.h"
#include "main.h"
#include "bt_manager.h"
#include "bt_conn.h"
#else
#include "segment_display.h"
#endif

// Binary min-heap implementation for efficient event scheduling
// The heap stores scheduled events ordered by timestamp (minimum at root)

typedef struct {
    event_scheduler_event_t event;
    uint32_t time;
    const char* label;
} heap_entry_t;

#define HEAP_INDEX_NONE 0xFF

static heap_entry_t heap[EventSchedulerEvent_Count];
static uint8_t heapIndex[EventSchedulerEvent_Count] = { [0 ... EventSchedulerEvent_Count-1] = HEAP_INDEX_NONE };
static uint8_t heapSize = 0;

volatile uint32_t EventScheduler_Vector = 0;

// Heap helper functions
static inline uint8_t parent(uint8_t i) { return (i - 1) / 2; }
static inline uint8_t leftChild(uint8_t i) { return 2 * i + 1; }
static inline uint8_t rightChild(uint8_t i) { return 2 * i + 2; }

static inline void heapSwap(uint8_t i, uint8_t j) {
    heap_entry_t tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
    heapIndex[heap[i].event] = i;
    heapIndex[heap[j].event] = j;
}

static void siftUp(uint8_t i) {
    while (i > 0 && heap[parent(i)].time > heap[i].time) {
        heapSwap(i, parent(i));
        i = parent(i);
    }
}

static void siftDown(uint8_t i) {
    uint8_t minIdx = i;
    uint8_t left = leftChild(i);
    uint8_t right = rightChild(i);

    if (left < heapSize && heap[left].time < heap[minIdx].time) {
        minIdx = left;
    }
    if (right < heapSize && heap[right].time < heap[minIdx].time) {
        minIdx = right;
    }
    if (minIdx != i) {
        heapSwap(i, minIdx);
        siftDown(minIdx);
    }
}

static void heapInsert(event_scheduler_event_t evt, uint32_t time, const char* label) {
    heap[heapSize].event = evt;
    heap[heapSize].time = time;
    heap[heapSize].label = label;
    heapIndex[evt] = heapSize;
    heapSize++;
    siftUp(heapSize - 1);
}

static void heapRemove(uint8_t i) {
    heapIndex[heap[i].event] = HEAP_INDEX_NONE;
    heapSize--;
    if (i < heapSize) {
        heap[i] = heap[heapSize];
        heapIndex[heap[i].event] = i;
        // May need to sift up or down depending on the replacement value
        if (i > 0 && heap[parent(i)].time > heap[i].time) {
            siftUp(i);
        } else {
            siftDown(i);
        }
    }
}

static void heapUpdate(uint8_t i, uint32_t newTime, const char* label) {
    uint32_t oldTime = heap[i].time;
    heap[i].time = newTime;
    heap[i].label = label;
    if (newTime < oldTime) {
        siftUp(i);
    } else {
        siftDown(i);
    }
}

static inline bool heapIsEmpty(void) {
    return heapSize == 0;
}

static inline heap_entry_t* heapPeek(void) {
    return heapSize > 0 ? &heap[0] : NULL;
}

static void updateEventVector(void) {
    EventVector_SetValue(EventVector_EventScheduler, heapSize > 0);
}

#define RETURN_IF_SPAM(EVT) if (isSpam(EVT)) { return; }

static bool isSpam(event_scheduler_event_t evt)
{
    switch (evt) {
        case EventSchedulerEvent_UpdateBattery:
        case EventSchedulerEvent_UpdateMergeSensor:
            return DEBUG_EVENTLOOP_SCHEDULE;
        default:
            return false;
    }
}

static void processEvt(event_scheduler_event_t evt)
{
    switch (evt) {
        case EventSchedulerEvent_UpdateBattery:
            Charger_UpdateBatteryState();
            break;
        case EventSchedulerEvent_UpdateBatteryCharging:
            Charger_UpdateBatteryCharging();
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
            UartBridge_Enable();
            UartModules_Enable();
            break;
        case EventSchedulerEvent_ModuleConnectionStatusUpdate:
            UhkModuleSlaveDriver_UpdateConnectionStatus();
            break;
        case EventSchedulerEvent_UpdateMergeSensor:
            MergeSensor_Update();
            break;
        case EventSchedulerEvent_PowerModeUpdate:
            PowerMode_Update();
            break;
        case EventSchedulerEvent_PowerModeRestart:
            PowerMode_Restart();
            break;
        case EventSchedulerEvent_EndBtPairing:
            BtPair_EndPairing(false, "Pairing timeout");
            break;
        case EventSchedulerEvent_RestartBt:
            BtManager_RestartBt();
            break;
        case EventSchedulerEvent_BtStartScanningAndAdvertising:
            BtManager_StartScanningAndAdvertising();
            break;
        case EventSchedulerEvent_RedrawOled:
            Oled_RequestRedraw();
            break;
        case EventSchedulerEvent_UpdateDebugOledLine:
            WIDGET_REFRESH(&DebugLineWidget);
            if (DEBUG_MODE) {
                EventScheduler_Schedule(Timer_GetCurrentTime()+1000, EventSchedulerEvent_UpdateDebugOledLine, "Event scheduler loop");
            }
            break;

        case EventSchedulerEvent_RoundTripTest:
            RoundTripTest_Run();
            EventScheduler_Schedule(Timer_GetCurrentTime()+10000, EventSchedulerEvent_RoundTripTest, "Event scheduler loop");
            break;
        case EventSchedulerEvent_ResendMessage:
            Resend_RequestResendSync();
            break;
        case EventSchedulerEvent_CheckFwChecksums:
            StateSync_CheckFirmwareVersions();
            break;
        case EventSchedulerEvent_CheckDongleProtocolVersion:
            StateSync_CheckDongleProtocolVersion();
            break;
        case EventSchedulerEvent_PutBackToShutDown:
            PowerMode_PutBackToSleepMaybe();
            break;
        case EventSchedulerEvent_BlinkStatusIcons:
            WIDGET_REFRESH(&StatusWidget);
            break;
        case EventSchedulerEvent_UnselectHostConnection:
            HostConnection_Unselect();
            break;
        case EventSchedulerEvent_OneShotTimeout:
            OneShot_OnTimeout();
            break;
        case EventSchedulerEvent_KickHid:
            BtConn_KickHid();
            break;
        default:
            return;
    }
}


void EventScheduler_Reschedule(uint32_t at, event_scheduler_event_t evt, const char* label)
{
    RETURN_IF_SPAM(evt);
    LOG_SCHEDULE(
        printk("%c", PostponerCore_EventsShouldBeQueued() ? 'P' : ' ');
        printk("    !!! Replan: %s\n", label);
    )

    uint8_t idx = heapIndex[evt];
    if (idx != HEAP_INDEX_NONE) {
        heapUpdate(idx, at, label);
    } else {
        heapInsert(evt, at, label);
    }
    updateEventVector();

#ifdef __ZEPHYR__
    Main_Wake();
#endif
}

void EventScheduler_Schedule(uint32_t at, event_scheduler_event_t evt, const char* label)
{
    RETURN_IF_SPAM(evt);
    LOG_SCHEDULE(
        printk("%c", PostponerCore_EventsShouldBeQueued() ? 'P' : ' ');
        printk("    !!! Plan: %s\n", label);
    );

    uint8_t idx = heapIndex[evt];
    if (idx != HEAP_INDEX_NONE) {
        // Only update if the new time is earlier
        if (at < heap[idx].time) {
            heapUpdate(idx, at, label);
        }
    } else {
        heapInsert(evt, at, label);
    }
    updateEventVector();

#ifdef __ZEPHYR__
    Main_Wake();
#endif
}

void EventScheduler_Unschedule(event_scheduler_event_t evt)
{
    RETURN_IF_SPAM(evt);

    uint8_t idx = heapIndex[evt];
    if (idx != HEAP_INDEX_NONE) {
        heapRemove(idx);
    }
    updateEventVector();
}

uint32_t EventScheduler_Process()
{
    while (!heapIsEmpty() && heap[0].time <= Timer_GetCurrentTime()) {
        heap_entry_t entry = heap[0];
        LOG_SCHEDULE(
            if (entry.label != NULL) {
                printk("%c", PostponerCore_EventsShouldBeQueued() ? 'P' : ' ');
                printk("!!! Scheduled event: %s\n", entry.label);
            }
        );
        heapRemove(0);
        updateEventVector();
        processEvt(entry.event);
    }
    return heapIsEmpty() ? UINT32_MAX : heap[0].time;
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
    REPORT_MASK(LedManagerFullUpdateNeeded);
    REPORT_MASK(KeymapReloadNeeded);
    REPORT_MASK(SegmentDisplayNeedsUpdate);
    REPORT_MASK(LedMapUpdateNeeded);
    REPORT_MASK(ApplyConfig);
    REPORT_MASK(NewMessage);

    REPORT_MASK(NativeActionReportsUsed);
    REPORT_MASK(MacroReportsUsed);
    REPORT_MASK(MouseKeysReportsUsed);
    REPORT_MASK(MouseControllerMouseReportsUsed);
    REPORT_MASK(MouseControllerKeyboardReportsUsed);
    REPORT_MASK(SendUsbReports);
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
    // heapIndex is statically initialized to 0, but we use 0xFF as "not scheduled"
    // Since static initialization is 0, we must initialize here before any scheduling
    for (uint8_t i = 0; i < EventSchedulerEvent_Count; i++) {
        heapIndex[i] = HEAP_INDEX_NONE;
    }

    if (DEVICE_IS_UHK60) {
        EventScheduler_Schedule(1000, EventSchedulerEvent_AgentLed, "Agent led");
    }
    EventScheduler_Schedule(1000, EventSchedulerEvent_UpdateLedSleepModes, "Update led sleep modes");
}
