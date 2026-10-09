#include "logger.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "macros/debug_commands.h"
#include "macros/key_timing.h"
#include "str_utils.h"
#include "utils.h"
#include "layer_stack.h"
#include "postponer.h"
#include "config_parser/parse_macro.h"
#include "timer.h"
#include "event_scheduler.h"
#include "wormhole.h"
#include "usb_commands/usb_command_reenumerate.h"
#include "hid/transport.h"

#ifdef __ZEPHYR__
#include "device.h"
#endif

#if DEVICE_IS_KEYBOARD && defined(__ZEPHYR__)
#include "keyboard/charger.h"
#include "keyboard/battery_manager.h"
#include "keyboard/battery_percent_calculator.h"
#include "state_sync.h"
#include "keyboard/uart_bridge.h"
#endif

macro_result_t Macros_ProcessStatsLayerStackCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    Macros_SetStatusString("kmp/layer/held/removed; size is ", NULL);
    Macros_SetStatusNum(LayerStack_Size);
    Macros_SetStatusString("\n", NULL);
    for (int i = 0; i < LayerStack_Size; i++) {
        uint8_t pos = (LayerStack_TopIdx + LAYER_STACK_SIZE - i) % LAYER_STACK_SIZE;
        Macros_SetStatusNum(LayerStack[pos].keymap);
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNum(LayerStack[pos].layer);
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNum(LayerStack[pos].held);
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNum(LayerStack[pos].removed);
        Macros_SetStatusString("\n", NULL);
    }
    return MacroResult_Finished;
}

macro_result_t Macros_ProcessStatsActiveKeysCommand()
{
#if DEVICE_IS_UHK_DONGLE
    return 0;
#else
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    Macros_SetStatusString("keyid/previous/current/debouncing\n", NULL);
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];
            if (keyState->current || keyState->previous) {
                Macros_SetStatusNum(Utils_KeyStateToKeyId(keyState));
                Macros_SetStatusString("/", NULL);
                Macros_SetStatusNum(keyState->previous);
                Macros_SetStatusString("/", NULL);
                Macros_SetStatusNum(keyState->current);
                Macros_SetStatusString("/", NULL);
                Macros_SetStatusNum(keyState->debouncing);
                Macros_SetStatusString("/", NULL);
            }
        }
    }
    return MacroResult_Finished;
#endif
}

macro_result_t Macros_ProcessStatsPostponerStackCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    PostponerExtended_PrintContent();
    return MacroResult_Finished;
}

static void describeSchedulerState()
{
    Macros_SetStatusString("s:", NULL);
    Macros_SetStatusNum(Macros_SchedulerState.currentSlotIdx);
    Macros_SetStatusNum(Macros_SchedulerState.previousSlotIdx);
    Macros_SetStatusNum(Macros_SchedulerState.lastQueuedSlot);
    Macros_SetStatusNum(Macros_SchedulerState.activeSlotCount);

    Macros_SetStatusString(":", NULL);
    uint8_t slot = Macros_SchedulerState.currentSlotIdx;
    for (int i = 0; i < Macros_SchedulerState.activeSlotCount; i++) {
        Macros_SetStatusNum(slot);
        Macros_SetStatusString(" ", NULL);
        slot = MacroState[slot].ms.nextSlot;
    }
    Macros_SetStatusString("\n", NULL);
}

macro_result_t Macros_ProcessStatsActiveMacrosCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    Macros_SetStatusString("Macro playing: ", NULL);
    Macros_SetStatusNum(EventVector_IsSet(EventVector_MacroEngine));
    Macros_SetStatusString("\n", NULL);
    Macros_SetStatusString("macro/slot/adr/properties\n", NULL);
    for (int i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying) {
            const char *name, *nameEnd;
            FindMacroName(&AllMacros[MacroState[i].ms.currentMacroIndex], &name, &nameEnd);
            Macros_SetStatusString(" ", NULL);
            Macros_SetStatusString(name, nameEnd);
            Macros_SetStatusString("/", NULL);
            Macros_SetStatusNum((&MacroState[i]) - MacroState);
            Macros_SetStatusString("/", NULL);
            Macros_SetStatusNum(MacroState[i].ms.currentMacroActionIndex);
            Macros_SetStatusString("/", NULL);
            if (MacroState[i].ls->as.modifierPostpone) {
                Macros_SetStatusString("mp ", NULL);
            }
            if (MacroState[i].ls->as.modifierSuppressMods) {
                Macros_SetStatusString("ms ", NULL);
            }
            if (MacroState[i].ms.macroSleeping) {
                Macros_SetStatusString("s ", NULL);
            }
            if (MacroState[i].ms.wakeMeOnKeystateChange) {
                Macros_SetStatusString("ws ", NULL);
            }
            if (MacroState[i].ms.wakeMeOnTime) {
                Macros_SetStatusString("wt ", NULL);
            }
            Macros_SetStatusString("\n", NULL);

        }
    }
    describeSchedulerState();
    return MacroResult_Finished;
}

// provided by the patched c2usb (usb/df/mac_diag.hpp)
extern void c2usb_diag_dump(void);

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <stdio.h>

extern k_tid_t Main_ThreadId;

#define MAIN_DUMP_SCAN_WORDS 160
#define MAIN_DUMP_MAX_ADDRS 10

// Where the main thread sits: its state, the pc/lr of its saved frame, and the thumb return
// addresses found on its stack - resolve them with addr2line against zephyr.elf. A blocked
// main shows up as a wait inside some caller; an idle one as the wait in scheduleNextRun.
static void dumpMainThread(void) {
    const struct k_thread *thread = Main_ThreadId;
    char state[24];
    const uint32_t *sp = (const uint32_t *)thread->callee_saved.psp;
    uint32_t stackStart = thread->stack_info.start;
    uint32_t stackEnd = stackStart + thread->stack_info.size;
    bool spValid = (uint32_t)sp >= stackStart && (uint32_t)sp + 32 <= stackEnd;

    k_thread_state_str(Main_ThreadId, state, sizeof(state));
    LogTo(DEVICE_ID, LogTarget_Uart | LogTarget_ErrorBuffer, "Main thread [%s] pc=%x lr=%x\n",
            state, spValid ? sp[6] : 0, spValid ? sp[5] : 0);

    if (spValid) {
        char line[128];
        int at = snprintf(line, sizeof(line), "  stack:");
        uint8_t found = 0;
        for (uint16_t i = 8; i < MAIN_DUMP_SCAN_WORDS && (uint32_t)&sp[i] < stackEnd && found < MAIN_DUMP_MAX_ADDRS; i++) {
            uint32_t word = sp[i];
            bool looksLikeReturn = (word & 1) && word > 0x1000 && word < 0x100000;
            if (looksLikeReturn) {
                at += snprintf(line + at, sizeof(line) - at, " %x", word);
                found++;
            }
        }
        LogTo(DEVICE_ID, LogTarget_Uart | LogTarget_ErrorBuffer, "%s\n", line);
    }
}
#endif

void Macros_RecoverDiagnostics(void)
{
    Macros_ProcessClearStatusCommand(true);
#ifdef __ZEPHYR__
    dumpMainThread();
#endif
    c2usb_diag_dump();
    Hid_DumpTransportState();
#if DEVICE_IS_KEYBOARD && defined(__ZEPHYR__)
    UartBridge_DumpStats();
#endif
    Trace_Print(LogTarget_Uart | LogTarget_ErrorBuffer, "Diagnostics reboot.");
    StateWormhole.persistStatusBuffer = true;
    Reboot(false);
}

macro_result_t Macros_ProcessDiagnoseCommand(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        ConsumeAnyToken(ctx);

        return MacroResult_Finished;
    }

    if (ConsumeToken(ctx, "usb")) {
        Macros_RecoverDiagnostics();
    } else if (ConsumeToken(ctx, "logic")) {
        Macros_ProcessStatsLayerStackCommand();
        Macros_ProcessStatsActiveKeysCommand();
        Macros_ProcessStatsPostponerStackCommand();
        Macros_ProcessStatsActiveMacrosCommand();
        for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
            for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
                key_state_t *keyState = &KeyStates[slotId][keyId];
                if (keyState != S->ms.currentMacroKey) {
                    keyState->current = 0;
                    keyState->previous = 0;
                }
            }
        }
        PostponerExtended_ResetPostponer();
    }

    return MacroResult_Finished;
}

macro_result_t Macros_ProcessStatsRecordKeyTimingCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    RecordKeyTiming = !RecordKeyTiming;
    return MacroResult_Finished;
}

macro_result_t Macros_ProcessStatsRuntimeCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    int ms = Timer_GetElapsedTime(&S->ms.currentMacroStartTime);
    Macros_SetStatusString("macro runtime is: ", NULL);
    Macros_SetStatusNum(ms);
    Macros_SetStatusString(" ms\n", NULL);
    return MacroResult_Finished;
}

macro_result_t Macros_ProcessStatsBatteryCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

#if defined(__ZEPHYR__) && DEVICE_IS_KEYBOARD
    battery_manager_config_t* cfg = BatteryManager_GetCurrentBatteryConfig();
    uint8_t perc = BatteryCalculator_CalculatePercent(SyncRightHalfState.battery.batteryVoltage);
    NotifyPrintf("%dmV %d%% / %dmV", SyncRightHalfState.battery.batteryVoltage, perc, cfg->maxVoltage);
#endif

    return MacroResult_Finished;
}
