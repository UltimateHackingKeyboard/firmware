#include "macros/core.h"
#include "macros/status_buffer.h"
#include "macros/debug_commands.h"
#include "macros/key_timing.h"
#include "utils.h"
#include "layer_stack.h"
#include "postponer.h"
#include "config_parser/parse_macro.h"
#include "timer.h"

#ifdef __ZEPHYR__
#include "device.h"
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
    Macros_SetStatusNum(MacroPlaying);
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

macro_result_t Macros_ProcessDiagnoseCommand()
{
#if DEVICE_IS_UHK_DONGLE
    return 0;
#else
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
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
    return MacroResult_Finished;
#endif
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
