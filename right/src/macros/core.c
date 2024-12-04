#include "config_parser/config_globals.h"
#include "config_parser/parse_macro.h"
#include "debug.h"
#include "event_scheduler.h"
#include "layer_stack.h"
#include "macros/commands.h"
#include "macros/debug_commands.h"
#include "macros/core.h"
#include "macros/scancode_commands.h"
#include "macros/status_buffer.h"
#include "macros/typedefs.h"
#include "postponer.h"
#include <string.h>
#include "str_utils.h"
#include "timer.h"
#include "usb_commands/usb_command_exec_macro_command.h"
#include "usb_report_updater.h"
#include "config_manager.h"

macro_reference_t AllMacros[MacroIndex_MaxCount] = {
    // 254 is reserved for USB command execution
    // 255 is reserved as empty value
    [MacroIndex_UsbCmdReserved] = {
        .macroActionsCount = 1,
    }
};
uint8_t AllMacrosCount;

bool Macros_WakedBecauseOfTime = false;
bool Macros_WakedBecauseOfKeystateChange = false;
uint32_t Macros_WakeMeOnTime = 0xFFFFFFFF;
bool Macros_WakeMeOnKeystateChange = false;

bool Macros_ParserError = false;
bool Macros_DryRun = false;
bool Macros_ValidationInProgress = false;
bool SchedulerPostponing = false;

scheduler_state_t Macros_SchedulerState = {
    .previousSlotIdx = 0,
    .currentSlotIdx = 0,
    .lastQueuedSlot = 0,
    .activeSlotCount = 0,
    .remainingCount = 0,
};

macro_scope_state_t MacroScopeState[MACRO_SCOPE_STATE_POOL_SIZE];
macro_state_t MacroState[MACRO_STATE_POOL_SIZE];
macro_state_t *S = NULL;

macro_history_t MacroHistory[MACRO_HISTORY_POOL_SIZE];
uint8_t MacroHistoryPosition = 0;

static void checkSchedulerHealth(const char* tag);
static void wakeMacroInSlot(uint8_t slotIdx);
static void scheduleSlot(uint8_t slotIdx);
static void unscheduleCurrentSlot();
static macro_result_t continueMacro(void);
static bool loadNextCommand();
static bool loadNextAction();
static void resetToAddressZero(uint8_t macroIndex);
static uint8_t currentActionCmdCount();

void Macros_SignalInterrupt()
{
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying) {
            MacroState[i].ms.macroInterrupted = true;
        }
    }
}

void Macros_SignalUsbReportsChange()
{
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying && MacroState[i].ms.macroInterrupted) {
            MacroState[i].ms.oneShotUsbChangeDetected = true;
        }
    }
}


void Macros_WakeBecauseOfKeystateChange() {
    if (Macros_WakeMeOnKeystateChange) {
        Macros_WakedBecauseOfKeystateChange = true;
        EventVector_Set(EventVector_MacroEngine);
    }
}

macro_result_t Macros_GoToAddress(uint8_t address)
{
    if(address == S->ls->ms.commandAddress) {
        memset(&S->as, 0, sizeof S->as);

        return MacroResult_JumpedBackward;
    }

    uint8_t oldAddress = S->ls->ms.commandAddress;

    //if we jump back, we have to reset and go from beginning
    if (address < S->ls->ms.commandAddress) {
        resetToAddressZero(S->ms.currentMacroIndex);
    }

    //if we are in the middle of multicommand action, parse till the end
    if(S->ls->ms.commandAddress < address && S->ls->ms.commandAddress != 0) {
        while (S->ls->ms.commandAddress < address && loadNextCommand());
    }

    //skip across actions without having to read entire action
    uint8_t cmdCount = currentActionCmdCount();
    while (S->ls->ms.commandAddress + cmdCount <= address) {
        loadNextAction();
        S->ls->ms.commandAddress += cmdCount - 1; //loadNextAction already added one
        cmdCount = currentActionCmdCount();
    }

    //now go command by command
    while (S->ls->ms.commandAddress < address && loadNextCommand()) ;

    return address > oldAddress ? MacroResult_JumpedForward: MacroResult_JumpedBackward;
}

macro_result_t Macros_GoToLabel(parser_context_t* ctx)
{
    uint8_t startedAtAdr = S->ls->ms.commandAddress;
    bool secondPass = false;
    bool reachedEnd = false;

    while ( !secondPass || S->ls->ms.commandAddress < startedAtAdr ) {
        while ( !reachedEnd && (!secondPass || S->ls->ms.commandAddress < startedAtAdr) ) {
            if(S->ms.currentMacroAction.type == MacroActionType_Command) {
                const char* cmd = S->ms.currentMacroAction.cmd.text + S->ls->ms.commandBegin;
                const char* cmdEnd = S->ms.currentMacroAction.cmd.text + S->ls->ms.commandEnd;
                const char* cmdTokEnd = TokEnd(cmd, cmdEnd);

                if(cmdTokEnd[-1] == ':' && TokenMatches2(cmd, cmdTokEnd-1, ctx->at, ctx->end)) {
                    return S->ls->ms.commandAddress > startedAtAdr ? MacroResult_JumpedForward : MacroResult_JumpedBackward;
                }
            }

            reachedEnd = !loadNextCommand() && !loadNextAction();
        }

        if (!secondPass) {
            resetToAddressZero(S->ms.currentMacroIndex);
            secondPass = true;
            reachedEnd = false;
        }
    }

    Macros_ReportError("Label not found:", ctx->at, ctx->end);
    S->ms.macroBroken = true;

    return MacroResult_Finished;
}


static macro_result_t processCurrentMacroAction(void)
{
    switch (S->ms.currentMacroAction.type) {
        case MacroActionType_Delay:
            return Macros_ProcessDelayAction();
        case MacroActionType_Key:
            return Macros_ProcessKeyAction();
        case MacroActionType_MouseButton:
            return Macros_ProcessMouseButtonAction();
        case MacroActionType_MoveMouse:
            return Macros_ProcessMoveMouseAction();
        case MacroActionType_ScrollMouse:
            return Macros_ProcessScrollMouseAction();
        case MacroActionType_Text:
            return Macros_ProcessTextAction();
        case MacroActionType_Command:
            return Macros_ProcessCommandAction();
    }
    return MacroResult_Finished;
}

bool Macros_PushScope(parser_context_t* ctx)
{
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (!MacroScopeState[i].slotUsed) {
            macro_scope_state_t* oldLS = S->ls;
            macro_scope_state_t* newLS = &MacroScopeState[i];
            memcpy(newLS, oldLS, sizeof *newLS);
            S->ls = newLS;
            newLS->parentScopeIndex = oldLS - MacroScopeState;
            newLS->ms.lastIfSucceeded = false;
            return true;
        }
    }
    S->ms.macroBroken = true;
    Macros_ReportError("Out of scope states. This means too many too deeply nested macros are running.", ctx->at, ctx->at);
    return false;
}

bool Macros_PopScope(parser_context_t* ctx)
{
    if (S->ls->parentScopeIndex != 255) {
        S->ls->slotUsed = false;
        S->ls = &MacroScopeState[S->ls->parentScopeIndex];
        S->ls->as.whileExecuting = false;
        return true;
    } else {
        Macros_ReportError("Encountered unmatched brace", ctx->at, ctx->at);
        S->ms.macroBroken = true;
        return false;
    }
}

static bool findFreeScopeStateSlot()
{
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (!MacroScopeState[i].slotUsed) {
            S->ls = &MacroScopeState[i];
            memset(S->ls, 0, sizeof *(S->ls));
            S->ls->slotUsed = true;
            S->ls->parentScopeIndex = 255;
            return true;
        }
    }
    S->ms.macroBroken = true;
    Macros_ReportError("Out of scope states. This means too many too deeply nested macros are running.", NULL, NULL);
    return false;
}


static bool findFreeStateSlot()
{
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (!MacroState[i].ms.macroPlaying) {
            S = &MacroState[i];
            return true;
        }
    }
    Macros_ReportError("Too many macros running at one time!", NULL, NULL);
    return false;
}


void Macros_Initialize() {
    LayerStack_Reset();
}

static uint8_t currentActionCmdCount() {
    if(S->ms.currentMacroAction.type == MacroActionType_Command) {
        return S->ms.currentMacroAction.cmd.cmdCount;
    } else {
        return 1;
    }
}

static void freeLocalScopes()
{
    macro_scope_state_t* ls = S->ls;
    while(true) {
        ls->slotUsed = false;
        if (ls->parentScopeIndex == 255) {
            break;
        }
        ls = &MacroScopeState[ls->parentScopeIndex];
    }
}

static macro_result_t endMacro(void)
{
    freeLocalScopes();

    S->ms.macroSleeping = false;
    S->ms.macroPlaying = false;
    S->ms.macroBroken = false;
    MacroHistoryPosition = (MacroHistoryPosition + 1) % MACRO_HISTORY_POOL_SIZE;
    MacroHistory[MacroHistoryPosition].macroIndex = S->ms.currentMacroIndex;
    MacroHistory[MacroHistoryPosition].macroStartTime = S->ms.currentMacroStartTime;
    unscheduleCurrentSlot();
    if (S->ms.parentMacroSlot != 255) {
        //resume our calee, if this macro was called by another macro
        wakeMacroInSlot(S->ms.parentMacroSlot);
        return MacroResult_YieldFlag;
    }
    return MacroResult_YieldFlag;
}

static void loadAction()
{
    if (S->ms.currentMacroIndex == MacroIndex_UsbCmdReserved) {
        // fill in action from memory
        S->ms.currentMacroAction = (macro_action_t){
            .type = MacroActionType_Command,
            .cmd = {
                .text = UsbMacroCommand,
                .textLen = UsbMacroCommandLength,
                .cmdCount = CountCommands(UsbMacroCommand, UsbMacroCommandLength)
            }
        };
    } else {
        // parse one macro action
        ValidatedUserConfigBuffer.offset = S->ms.bufferOffset;
        ParseMacroAction(&ValidatedUserConfigBuffer, &S->ms.currentMacroAction);
        S->ms.bufferOffset = ValidatedUserConfigBuffer.offset;
    }

    memset(&S->as, 0, sizeof S->as);

    if (S->ms.currentMacroAction.type == MacroActionType_Command) {
        const char* cmd = S->ms.currentMacroAction.cmd.text;
        const char* actionEnd = S->ms.currentMacroAction.cmd.text + S->ms.currentMacroAction.cmd.textLen;
        while ( *cmd <= 32 && cmd < actionEnd) {
            cmd++;
        }
        S->ls->ms.commandBegin = cmd - S->ms.currentMacroAction.cmd.text;
        S->ls->ms.commandEnd = CmdEnd(cmd, actionEnd) - S->ms.currentMacroAction.cmd.text;
    }
}

static bool loadNextAction()
{
    if (S->ms.currentMacroActionIndex + 1 >= AllMacros[S->ms.currentMacroIndex].macroActionsCount) {
        return false;
    } else {
        loadAction();
        S->ms.currentMacroActionIndex++;
        S->ls->ms.commandAddress++;
        return true;
    }
}

static bool loadNextCommand()
{
    if (S->ms.currentMacroAction.type != MacroActionType_Command) {
        return false;
    }

    if (!Macros_DryRun) {
        memset(&S->as, 0, sizeof S->as);
        memset(&S->ls->as, 0, sizeof S->ls->as);
    }

    const char* actionEnd = S->ms.currentMacroAction.cmd.text + S->ms.currentMacroAction.cmd.textLen;
    const char* nextCommand = NextCmd(S->ms.currentMacroAction.cmd.text + S->ls->ms.commandEnd, actionEnd);

    if (nextCommand == actionEnd) {
        return false;
    } else {
        S->ls->ms.commandAddress++;
        S->ls->ms.commandBegin = nextCommand - S->ms.currentMacroAction.cmd.text;
        S->ls->ms.commandEnd = CmdEnd(nextCommand, actionEnd) - S->ms.currentMacroAction.cmd.text;
        return true;
    }
}

bool Macros_LoadNextCommand()
{
    return loadNextCommand();
}

bool Macros_LoadNextAction()
{
    return loadNextAction();
}

static void resetToAddressZero(uint8_t macroIndex)
{
    S->ms.currentMacroIndex = macroIndex;
    S->ms.currentMacroActionIndex = 0;
    S->ls->ms.commandAddress = 0;
    S->ms.bufferOffset = AllMacros[macroIndex].firstMacroActionOffset; //set offset to first action
    loadAction();  //loads first action, sets offset to second action
}

static bool macroIsValid(uint8_t index)
{
    bool isNonEmpty = AllMacros[index].macroActionsCount != 0;
    bool exists = index != MacroIndex_None;
    return exists && isNonEmpty;
}

macro_result_t Macros_ExecMacro(uint8_t macroIndex)
{
    if (!macroIsValid(macroIndex))  {
       S->ms.macroBroken = true;
       return MacroResult_Finished;
    }

    //reset to address zero and load first address
    resetToAddressZero(macroIndex);

    if (Cfg.Macros_Scheduler == Scheduler_Preemptive) {
        continueMacro();
    }

    return MacroResult_JumpedForward;
}

macro_result_t Macros_CallMacro(uint8_t macroIndex)
{
    uint32_t parentSlotIndex = S - MacroState;
    uint8_t childSlotIndex = Macros_StartMacro(macroIndex, S->ms.currentMacroKey, parentSlotIndex, true);

    if (childSlotIndex != 255) {
        unscheduleCurrentSlot();
        S->ms.macroSleeping = true;
        S->ms.wakeMeOnKeystateChange = false;
        S->ms.wakeMeOnTime = false;
    }

    return MacroResult_Finished | MacroResult_YieldFlag;
}

macro_result_t Macros_ForkMacro(uint8_t macroIndex)
{
    Macros_StartMacro(macroIndex, S->ms.currentMacroKey, 255, true);
    return MacroResult_Finished;
}

uint8_t initMacro(uint8_t index, key_state_t *keyState, uint8_t parentMacroSlot)
{
    if (!macroIsValid(index) || !findFreeStateSlot() || !findFreeScopeStateSlot())  {
       return 255;
    }

    EventVector_Set(EventVector_MacroEngine);

    memset(&S->ms, 0, sizeof S->ms);

    S->ms.macroPlaying = true;
    S->ms.currentMacroIndex = index;
    S->ms.currentMacroKey = keyState;
    S->ms.currentMacroStartTime = CurrentPostponedTime;
    S->ms.parentMacroSlot = parentMacroSlot;

    //this loads the first action and resets all adresses
    resetToAddressZero(index);

    return S - MacroState;
}


//partentMacroSlot == 255 means no parent
uint8_t Macros_StartMacro(uint8_t index, key_state_t *keyState, uint8_t parentMacroSlot, bool runFirstAction)
{
    macro_state_t* oldState = S;

    uint8_t slotIndex = initMacro(index, keyState, parentMacroSlot);

    if (slotIndex == 255) {
        S = oldState;
        return slotIndex;
    }

    if (Cfg.Macros_Scheduler == Scheduler_Preemptive && runFirstAction && (parentMacroSlot == 255 || S < &MacroState[parentMacroSlot])) {
        //execute first action if macro has no caller Or is being called and its caller has higher slot index.
        //The condition ensures that a called macro executes exactly one action in the same eventloop cycle.
        continueMacro();
    }

    scheduleSlot(slotIndex);
    if (Cfg.Macros_Scheduler == Scheduler_Blocking) {
        // We don't care. Let it execute in regular macro execution loop, irrespectively whether this cycle or next.
        // PostponerCore_PostponeNCycles(0);
        EventVector_Set(EventVector_MacroEnginePostponing);
    }

    S = oldState;
    return slotIndex;
}

void Macros_ValidateAllMacros()
{
    macro_state_t* oldS = S;
    scheduler_state_t schedulerState = Macros_SchedulerState;
    memset(&Macros_SchedulerState, 0, sizeof Macros_SchedulerState);
    Macros_DryRun = true;
    Macros_ValidationInProgress = true;
    for (uint8_t macroIndex = 0; macroIndex < AllMacrosCount; macroIndex++) {
        uint8_t slotIndex = initMacro(macroIndex, NULL, 255);

        if (slotIndex == 255) {
            S = NULL;
            continue;
        }

        scheduleSlot(slotIndex);

        bool macroHasNotEnded = AllMacros[macroIndex].macroActionsCount;
        while (macroHasNotEnded) {
            if (S->ms.currentMacroAction.type == MacroActionType_Command) {
                processCurrentMacroAction();
                Macros_ParserError = false;
            }

            macroHasNotEnded = loadNextCommand() || loadNextAction();
        }
        endMacro();
        S = NULL;
    }
    Macros_ValidationInProgress = false;
    Macros_DryRun = false;
    Macros_SchedulerState = schedulerState;
    S = oldS;
}

uint8_t Macros_QueueMacro(uint8_t index, key_state_t *keyState, uint8_t queueAfterSlot)
{
    macro_state_t* oldState = S;

    uint8_t slotIndex = initMacro(index, keyState, 255);

    if (slotIndex == 255) {
        return slotIndex;
    }

    uint8_t childSlotIndex = queueAfterSlot;

    while (MacroState[childSlotIndex].ms.parentMacroSlot != 255) {
        childSlotIndex = MacroState[childSlotIndex].ms.parentMacroSlot;
    }

    MacroState[childSlotIndex].ms.parentMacroSlot = slotIndex;
    S->ms.macroSleeping = true;

    S = oldState;
    return slotIndex;
}

macro_result_t continueMacro(void)
{
    Macros_ParserError = false;
    S->ls->as.modifierPostpone = false;
    S->ls->as.modifierSuppressMods = false;

    if (S->ms.postponeNextNCommands > 0) {
        S->ls->as.modifierPostpone = true;
        //PostponerCore_PostponeNCycles(1);
    }

    macro_result_t res = MacroResult_YieldFlag;

    if (!S->ms.macroBroken && ((res = processCurrentMacroAction()) & (MacroResult_InProgressFlag | MacroResult_DoneFlag)) && !S->ms.macroBroken) {
        // InProgressFlag means that action is still in progress
        // DoneFlag means that someone has already done epilogue and therefore we should just return the value.
        return res;
    }

    //at this point, current action/command has finished
    S->ms.postponeNextNCommands = S->ms.postponeNextNCommands > 0 ? S->ms.postponeNextNCommands - 1 : 0;

    if ((S->ms.macroBroken) || (!loadNextCommand() && !loadNextAction())) {
        //macro was ended either because it was broken or because we are out of actions to perform.
        return endMacro() | res;
    } else {
        //we are still running - return last action's return value
        return res;
    }
}

macro_result_t Macros_SleepTillKeystateChange()
{
    if(S->ms.oneShotState > 0) {
        if(S->ms.oneShotState > 1) {
            return MacroResult_Blocking;
        } else if (Cfg.Macros_OneShotTimeout != 0) {
            Macros_SleepTillTime(S->ms.currentMacroStartTime + Cfg.Macros_OneShotTimeout, "Macros - OneShot");
        }
    }
    if (!S->ms.macroSleeping) {
        unscheduleCurrentSlot();
    }
    Macros_WakeMeOnKeystateChange = true;
    S->ms.wakeMeOnKeystateChange = true;
    S->ms.macroSleeping = true;
    return MacroResult_Sleeping;
}

macro_result_t Macros_SleepTillTime(uint32_t time, const char* reason)
{
    if(S->ms.oneShotState > 0) {
        if(S->ms.oneShotState > 1) {
            return MacroResult_Blocking;
        } else if (Cfg.Macros_OneShotTimeout != 0) {
            EventScheduler_Schedule(S->ms.currentMacroStartTime + Cfg.Macros_OneShotTimeout, EventSchedulerEvent_MacroWakeOnTime, "Macros - OneShot");
        }
    }
    if (!S->ms.macroSleeping) {
        unscheduleCurrentSlot();
    }
    EventScheduler_Schedule(time, EventSchedulerEvent_MacroWakeOnTime, reason);
    S->ms.wakeMeOnTime = true;
    S->ms.macroSleeping = true;
    return MacroResult_Sleeping;
}

static void wakeSleepers()
{
    if (Macros_WakedBecauseOfKeystateChange) {
        Macros_WakedBecauseOfKeystateChange = false;
        Macros_WakeMeOnKeystateChange = false;
        for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
            if (MacroState[i].ms.wakeMeOnKeystateChange) {
                wakeMacroInSlot(i);
            }
        }
    }
    if (Macros_WakedBecauseOfTime) {
        Macros_WakedBecauseOfTime = false;
        Macros_WakeMeOnTime = 0xFFFFFFFF;
        for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
            if (MacroState[i].ms.wakeMeOnTime) {
                wakeMacroInSlot(i);
            }
        }
    }
}

static void executePreemptive(void)
{
    SchedulerPostponing = false;
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying && !MacroState[i].ms.macroSleeping) {
            S = &MacroState[i];

            macro_result_t res = MacroResult_Finished;

            if (res & MacroResult_BlockingFlag) {
                SchedulerPostponing = true;
                EventVector_Set(EventVector_SendUsbReports);
            }

            if (S->ms.macroInterrupted) {
                S->ms.macroInterrupted = false;
                res = endMacro();
        }

            uint8_t remainingExecution = Cfg.Macros_MaxBatchSize;
            while (MacroState[i].ms.macroPlaying && !MacroState[i].ms.macroSleeping && res == MacroResult_Finished && remainingExecution > 0) {
                res = continueMacro();
                remainingExecution --;
            }
        }
    }
    S = NULL;
}

static void wakeMacroInSlot(uint8_t slotIdx)
{
    if (MacroState[slotIdx].ms.macroSleeping) {
        MacroState[slotIdx].ms.macroSleeping = false;
        MacroState[slotIdx].ms.wakeMeOnTime = false;
        MacroState[slotIdx].ms.wakeMeOnKeystateChange = false;
        scheduleSlot(slotIdx);
    }
}

static void __attribute__((__unused__)) checkSchedulerHealth(const char* tag) {
    static const char* lastCmd = NULL;
    uint8_t scheduledCount = 0;
    uint8_t playingCount = 0;

    // count active macros
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying && !MacroState[i].ms.macroSleeping) {
            playingCount++;
        }
    }

    // count length of the scheduling loop
    if (Macros_SchedulerState.activeSlotCount != 0) {
        uint8_t currentSlot = Macros_SchedulerState.currentSlotIdx;
        uint8_t startedAt = currentSlot;
        for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
            scheduledCount++;

            if (!MacroState[currentSlot].ms.macroPlaying) {
                Macros_ReportErrorNum("This slot is not playing, but is scheduled:", currentSlot, NULL);
            }

            if (MacroState[currentSlot].ms.macroSleeping) {
                Macros_ReportErrorNum("This slot is sleeping, but is scheduled:", currentSlot, NULL);
            }

            currentSlot = MacroState[currentSlot].ms.nextSlot;
            if (currentSlot == startedAt) {
                break;
            }
        }
    }

    const char* thisCmd = NULL;

    // retrieve currently running command if possible
    if (S != NULL && S->ms.currentMacroAction.type == MacroActionType_Command) {
        thisCmd = S->ms.currentMacroAction.cmd.text + S->ls->ms.commandBegin;
    }

    // check the results
    if (scheduledCount != playingCount || scheduledCount != Macros_SchedulerState.activeSlotCount) {
        Macros_ReportError("Scheduled counts don't match up!", tag, NULL);
        Macros_ReportErrorNum("Scheduled", scheduledCount, NULL);
        Macros_ReportErrorNum("Playing", playingCount, NULL);
        Macros_ReportErrorNum("Active slot count", Macros_SchedulerState.activeSlotCount, NULL);
        Macros_ReportError("Prev cmd", lastCmd, lastCmd+10);
        Macros_ReportError("This cmd", thisCmd, thisCmd+10);
        Macros_ProcessStatsActiveMacrosCommand();
    }

    lastCmd = thisCmd;
}

static void scheduleSlot(uint8_t slotIdx)
{
    if (!MacroState[slotIdx].ms.macroScheduled) {
        if (Macros_SchedulerState.activeSlotCount == 0) {
            MacroState[slotIdx].ms.nextSlot = slotIdx;
            MacroState[slotIdx].ms.macroScheduled = true;
            Macros_SchedulerState.previousSlotIdx = slotIdx;
            Macros_SchedulerState.currentSlotIdx = slotIdx;
            Macros_SchedulerState.lastQueuedSlot = slotIdx;
            Macros_SchedulerState.activeSlotCount++;
            Macros_SchedulerState.remainingCount++;
        } else {
            bool shouldInheritPrevious = Macros_SchedulerState.lastQueuedSlot == Macros_SchedulerState.previousSlotIdx;
            MacroState[slotIdx].ms.nextSlot = MacroState[Macros_SchedulerState.lastQueuedSlot].ms.nextSlot;
            MacroState[Macros_SchedulerState.lastQueuedSlot].ms.nextSlot = slotIdx;
            MacroState[slotIdx].ms.macroScheduled = true;
            Macros_SchedulerState.lastQueuedSlot = slotIdx;
            Macros_SchedulerState.activeSlotCount++;
            Macros_SchedulerState.remainingCount++;
            if(shouldInheritPrevious) {
                Macros_SchedulerState.previousSlotIdx = slotIdx;
            }
        }
    } else {
        ERR("Scheduling an already scheduled slot attempted!");
    }
}

static void unscheduleCurrentSlot()
{
    if (Macros_SchedulerState.currentSlotIdx == (S - MacroState) && MacroState[Macros_SchedulerState.currentSlotIdx].ms.macroScheduled) {
        MacroState[Macros_SchedulerState.previousSlotIdx].ms.nextSlot = MacroState[Macros_SchedulerState.currentSlotIdx].ms.nextSlot;
        MacroState[Macros_SchedulerState.currentSlotIdx].ms.macroScheduled = false;
        Macros_SchedulerState.lastQueuedSlot = Macros_SchedulerState.lastQueuedSlot == Macros_SchedulerState.currentSlotIdx ? Macros_SchedulerState.previousSlotIdx : Macros_SchedulerState.lastQueuedSlot;
        Macros_SchedulerState.currentSlotIdx = Macros_SchedulerState.previousSlotIdx;
        Macros_SchedulerState.activeSlotCount--;
    } else if (Macros_SchedulerState.currentSlotIdx != (S - MacroState) && !S->ms.macroScheduled) {
        // This means that current slot is already unscheduled but scheduler state is fine.
        // This may happen (for instance) if callMacro is the last command of a macro.
        // (We get one unschedule attempt for sleeping the macro, but at the same time for macro end.)
        return;
    } else {
        // this means that the state is inconsistent for some reason
        ERR("Unsechuling non-scheduled slot attempted!");
    }
}

static void getNextScheduledSlot()
{
    if (Macros_SchedulerState.remainingCount > 0) {
        uint8_t nextSlot = MacroState[Macros_SchedulerState.currentSlotIdx].ms.nextSlot;
        Macros_SchedulerState.previousSlotIdx = Macros_SchedulerState.currentSlotIdx;
        Macros_SchedulerState.lastQueuedSlot = Macros_SchedulerState.lastQueuedSlot == Macros_SchedulerState.currentSlotIdx ? nextSlot : Macros_SchedulerState.lastQueuedSlot;
        Macros_SchedulerState.currentSlotIdx = nextSlot;
        Macros_SchedulerState.remainingCount--;
    }
}

static void executeBlocking(void)
{
    SchedulerPostponing = false;
    bool someoneBlocking = false;
    uint8_t remainingExecution = Cfg.Macros_MaxBatchSize;
    Macros_SchedulerState.remainingCount = Macros_SchedulerState.activeSlotCount;

    while (Macros_SchedulerState.remainingCount > 0 && remainingExecution > 0) {
        macro_result_t res = MacroResult_YieldFlag;
        S = &MacroState[Macros_SchedulerState.currentSlotIdx];

        if (S->ms.macroPlaying && !S->ms.macroSleeping) {
            IF_DEBUG(checkSchedulerHealth("co1"));
            res = continueMacro();
            IF_DEBUG(checkSchedulerHealth("co2"));
        }

        if ((someoneBlocking = (res & MacroResult_BlockingFlag))) {
            SchedulerPostponing = true;
            EventVector_Set(EventVector_SendUsbReports);
            break;
        }

        if ((res & MacroResult_YieldFlag) || !S->ms.macroPlaying || S->ms.macroSleeping) {
            getNextScheduledSlot();
        }

        remainingExecution--;
    }

    if(someoneBlocking || remainingExecution == 0) {
        SchedulerPostponing = true;
    }

    S = NULL;
}


typedef struct {
    bool modifierSuppressMods;
    bool reportsUsed;
    bool someoneAlive;
    bool someoneAwake;
    bool someonePostponing;
} macro_applicable_state_t;

static macro_applicable_state_t applicableState = {
    .modifierSuppressMods = false,
    .reportsUsed = false,
    .someoneAlive = false,
    .someoneAwake = false,
    .someonePostponing = false,
};

static void recalculateSleepingMods()
{
    applicableState.modifierSuppressMods = false;
    applicableState.reportsUsed = false;
    applicableState.someoneAlive = false;
    applicableState.someoneAwake = false;
    applicableState.someonePostponing = SchedulerPostponing;

    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying) {
            applicableState.someoneAlive = true;
            applicableState.reportsUsed |= MacroState[i].ms.reportsUsed;
        }
        if (MacroState[i].ms.macroPlaying && !MacroState[i].ms.macroSleeping) {
            applicableState.someoneAwake = true;
        }
    }
    for (uint8_t i = 0; i < MACRO_SCOPE_STATE_POOL_SIZE; i++) {
        if (MacroScopeState[i].slotUsed) {
            macro_scope_state_t* ls = &MacroScopeState[i];
            if ( ls->as.modifierPostpone ) {
                applicableState.someonePostponing = true;
            }
            if ( ls->as.modifierSuppressMods ) {
                SuppressMods = true;
                applicableState.modifierSuppressMods = true;
            }
        }
    }
    EventVector_SetValue(EventVector_MacroEnginePostponing, applicableState.someonePostponing);
    EventVector_SetValue(EventVector_MacroEngine, applicableState.someoneAwake);
    EventVector_SetValue(EventVector_MacroReportsUsed, applicableState.reportsUsed);
    SuppressMods = applicableState.modifierSuppressMods;
    S = NULL;
}

void Macros_ContinueMacro(void)
{
    wakeSleepers();

    switch (Cfg.Macros_Scheduler) {
    case Scheduler_Preemptive:
        executePreemptive();
        recalculateSleepingMods();
        break;
    case Scheduler_Blocking:
        executeBlocking();
        recalculateSleepingMods();
        break;
    default:
        break;
    }
}


bool Macros_MacroHasActiveInstance(macro_index_t macroIdx)
{
    for(uint8_t j = 0; j < MACRO_STATE_POOL_SIZE; j++) {
        if(MacroState[j].ms.macroPlaying && MacroState[j].ms.currentMacroIndex == macroIdx) {
            return true;
        }
    }
    return false;
}

void Macros_ResetBasicKeyboardReports()
{
    for(uint8_t j = 0; j < MACRO_STATE_POOL_SIZE; j++) {
        memset(&MacroState[j].ms.macroBasicKeyboardReport, 0, sizeof MacroState[j].ms.macroBasicKeyboardReport);
    }
}
