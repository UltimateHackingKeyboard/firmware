#include "config_parser/config_globals.h"
#include "config_parser/parse_macro.h"
#include "debug.h"
#include "eeprom.h"
#include "event_scheduler.h"
#include "fsl_rtos_abstraction.h"
#include "layer_stack.h"
#include "macro_recorder.h"
#include "macros_commands.h"
#include "macros_debug_commands.h"
#include "macros.h"
#include "macros_keyid_parser.h"
#include "macros_scancode_commands.h"
#include "macros_set_command.h"
#include "macros_shortcut_parser.h"
#include "macros_status_buffer.h"
#include "macros_string_reader.h"
#include "macros_typedefs.h"
#include "macros_vars.h"
#include "macros_vars.h"
#include "postponer.h"
#include <string.h>
#include "str_utils.h"
#include "timer.h"
#include "usb_commands/usb_command_exec_macro_command.h"
#include "usb_report_updater.h"
#include "utils.h"

macro_reference_t AllMacros[MacroIndex_MaxCount] = {
    // 254 is reserved for USB command execution
    // 255 is reserved as empty value
    [MacroIndex_UsbCmdReserved] = {
        .macroActionsCount = 1,
    }
};
uint8_t AllMacrosCount;

bool MacroPlaying = false;
bool Macros_WakedBecauseOfTime = false;
bool Macros_WakedBecauseOfKeystateChange = false;
uint32_t Macros_WakeMeOnTime = 0xFFFFFFFF;
bool Macros_WakeMeOnKeystateChange = false;

bool Macros_ParserError = false;
bool Macros_DryRun = false;

macro_scheduler_t Macros_Scheduler = Scheduler_Blocking;
uint8_t Macros_MaxBatchSize = 20;
scheduler_state_t Macros_SchedulerState = {
    .previousSlotIdx = 0,
    .currentSlotIdx = 0,
    .lastQueuedSlot = 0,
    .activeSlotCount = 0,
    .remainingCount = 0,
};

macro_state_t MacroState[MACRO_STATE_POOL_SIZE];
macro_state_t *s = NULL;

uint16_t DoubletapConditionTimeout = 400;
uint16_t AutoRepeatInitialDelay = 500;
uint16_t AutoRepeatDelayRate = 50;

bool RecordKeyTiming = false;

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

macro_result_t Macros_GoToAddress(uint8_t address)
{
    if(address == s->ms.commandAddress) {
        memset(&s->as, 0, sizeof s->as);

        return MacroResult_JumpedBackward;
    }

    uint8_t oldAddress = s->ms.commandAddress;

    //if we jump back, we have to reset and go from beginning
    if (address < s->ms.commandAddress) {
        resetToAddressZero(s->ms.currentMacroIndex);
    }

    //if we are in the middle of multicommand action, parse till the end
    if(s->ms.commandAddress < address && s->ms.commandAddress != 0) {
        while (s->ms.commandAddress < address && loadNextCommand());
    }

    //skip across actions without having to read entire action
    uint8_t cmdCount = currentActionCmdCount();
    while (s->ms.commandAddress + cmdCount <= address) {
        loadNextAction();
        s->ms.commandAddress += cmdCount - 1; //loadNextAction already added one
        cmdCount = currentActionCmdCount();
    }

    //now go command by command
    while (s->ms.commandAddress < address && loadNextCommand()) ;

    return address > oldAddress ? MacroResult_JumpedForward: MacroResult_JumpedBackward;
}

macro_result_t Macros_GoToLabel(parser_context_t* ctx)
{
    uint8_t startedAtAdr = s->ms.commandAddress;
    bool secondPass = false;
    bool reachedEnd = false;

    while ( !secondPass || s->ms.commandAddress < startedAtAdr ) {
        while ( !reachedEnd && (!secondPass || s->ms.commandAddress < startedAtAdr) ) {
            if(s->ms.currentMacroAction.type == MacroActionType_Command) {
                const char* cmd = s->ms.currentMacroAction.cmd.text + s->ms.commandBegin;
                const char* cmdEnd = s->ms.currentMacroAction.cmd.text + s->ms.commandEnd;
                const char* cmdTokEnd = TokEnd(cmd, cmdEnd);

                if(cmdTokEnd[-1] == ':' && TokenMatches2(cmd, cmdTokEnd-1, ctx->at, ctx->end)) {
                    return s->ms.commandAddress > startedAtAdr ? MacroResult_JumpedForward : MacroResult_JumpedBackward;
                }
            }

            reachedEnd = !loadNextCommand() && !loadNextAction();
        }

        if (!secondPass) {
            resetToAddressZero(s->ms.currentMacroIndex);
            secondPass = true;
            reachedEnd = false;
        }
    }

    Macros_ReportError("Label not found:", ctx->at, ctx->end);
    s->ms.macroBroken = true;

    return MacroResult_Finished;
}


static macro_result_t processCurrentMacroAction(void)
{
    switch (s->ms.currentMacroAction.type) {
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


static bool findFreeStateSlot()
{
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (!MacroState[i].ms.macroPlaying) {
            s = &MacroState[i];
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
    if(s->ms.currentMacroAction.type == MacroActionType_Command) {
        return s->ms.currentMacroAction.cmd.cmdCount;
    } else {
        return 1;
    }
}

static macro_result_t endMacro(void)
{
    s->ms.macroSleeping = false;
    s->ms.macroPlaying = false;
    s->ms.macroBroken = false;
    s->ps.previousMacroIndex = s->ms.currentMacroIndex;
    s->ps.previousMacroStartTime = s->ms.currentMacroStartTime;
    unscheduleCurrentSlot();
    if (s->ms.parentMacroSlot != 255) {
        //resume our calee, if this macro was called by another macro
        wakeMacroInSlot(s->ms.parentMacroSlot);
        return MacroResult_YieldFlag;
    }
    return MacroResult_YieldFlag;
}

static void loadAction()
{
    if (s->ms.currentMacroIndex == MacroIndex_UsbCmdReserved) {
        // fill in action from memory
        s->ms.currentMacroAction = (macro_action_t){
            .type = MacroActionType_Command,
            .cmd = {
                .text = UsbMacroCommand,
                .textLen = UsbMacroCommandLength,
                .cmdCount = CountCommands(UsbMacroCommand, UsbMacroCommandLength)
            }
        };
    } else {
        // parse one macro action
        ValidatedUserConfigBuffer.offset = s->ms.bufferOffset;
        ParseMacroAction(&ValidatedUserConfigBuffer, &s->ms.currentMacroAction);
        s->ms.bufferOffset = ValidatedUserConfigBuffer.offset;
    }

    memset(&s->as, 0, sizeof s->as);

    if (s->ms.currentMacroAction.type == MacroActionType_Command) {
        const char* cmd = s->ms.currentMacroAction.cmd.text;
        const char* actionEnd = s->ms.currentMacroAction.cmd.text + s->ms.currentMacroAction.cmd.textLen;
        while ( *cmd <= 32 && cmd < actionEnd) {
            cmd++;
        }
        s->ms.commandBegin = cmd - s->ms.currentMacroAction.cmd.text;
        s->ms.commandEnd = CmdEnd(cmd, actionEnd) - s->ms.currentMacroAction.cmd.text;
    }
}

static bool loadNextAction()
{
    if (s->ms.currentMacroActionIndex + 1 >= AllMacros[s->ms.currentMacroIndex].macroActionsCount) {
        return false;
    } else {
        loadAction();
        s->ms.currentMacroActionIndex++;
        s->ms.commandAddress++;
        return true;
    }
}

static bool loadNextCommand()
{
    if (s->ms.currentMacroAction.type != MacroActionType_Command) {
        return false;
    }

    memset(&s->as, 0, sizeof s->as);

    const char* actionEnd = s->ms.currentMacroAction.cmd.text + s->ms.currentMacroAction.cmd.textLen;
    const char* nextCommand = NextCmd(s->ms.currentMacroAction.cmd.text + s->ms.commandEnd, actionEnd);

    if (nextCommand == actionEnd) {
        return false;
    } else {
        s->ms.commandAddress++;
        s->ms.commandBegin = nextCommand - s->ms.currentMacroAction.cmd.text;
        s->ms.commandEnd = CmdEnd(nextCommand, actionEnd) - s->ms.currentMacroAction.cmd.text;
        return true;
    }
}

static void resetToAddressZero(uint8_t macroIndex)
{
    s->ms.currentMacroIndex = macroIndex;
    s->ms.currentMacroActionIndex = 0;
    s->ms.commandAddress = 0;
    s->ms.bufferOffset = AllMacros[macroIndex].firstMacroActionOffset; //set offset to first action
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
       s->ms.macroBroken = true;
       return MacroResult_Finished;
    }

    //reset to address zero and load first address
    resetToAddressZero(macroIndex);

    if (Macros_Scheduler == Scheduler_Preemptive) {
        continueMacro();
    }

    return MacroResult_JumpedForward;
}

macro_result_t Macros_CallMacro(uint8_t macroIndex)
{
    uint32_t parentSlotIndex = s - MacroState;
    uint8_t childSlotIndex = Macros_StartMacro(macroIndex, s->ms.currentMacroKey, parentSlotIndex, true);

    if (childSlotIndex != 255) {
        unscheduleCurrentSlot();
        s->ms.macroSleeping = true;
        s->ms.wakeMeOnKeystateChange = false;
        s->ms.wakeMeOnTime = false;
    }

    return MacroResult_Finished | MacroResult_YieldFlag;
}

macro_result_t Macros_ForkMacro(uint8_t macroIndex)
{
    Macros_StartMacro(macroIndex, s->ms.currentMacroKey, 255, true);
    return MacroResult_Finished;
}

uint8_t initMacro(uint8_t index, key_state_t *keyState, uint8_t parentMacroSlot)
{
    if (!macroIsValid(index) || !findFreeStateSlot())  {
       return 255;
    }

    MacroPlaying = true;

    memset(&s->ms, 0, sizeof s->ms);

    s->ms.macroPlaying = true;
    s->ms.currentMacroIndex = index;
    s->ms.currentMacroKey = keyState;
    s->ms.currentMacroStartTime = CurrentTime;
    s->ms.parentMacroSlot = parentMacroSlot;

    //this loads the first action and resets all adresses
    resetToAddressZero(index);

    return s - MacroState;
}


//partentMacroSlot == 255 means no parent
uint8_t Macros_StartMacro(uint8_t index, key_state_t *keyState, uint8_t parentMacroSlot, bool runFirstAction)
{
    macro_state_t* oldState = s;

    uint8_t slotIndex = initMacro(index, keyState, parentMacroSlot);

    if (slotIndex == 255) {
        s = oldState;
        return slotIndex;
    }

    if (Macros_Scheduler == Scheduler_Preemptive && runFirstAction && (parentMacroSlot == 255 || s < &MacroState[parentMacroSlot])) {
        //execute first action if macro has no caller Or is being called and its caller has higher slot index.
        //The condition ensures that a called macro executes exactly one action in the same eventloop cycle.
        continueMacro();
    }

    scheduleSlot(slotIndex);
    if (Macros_Scheduler == Scheduler_Blocking) {
        // We don't care. Let it execute in regular macro execution loop, irrespectively whether this cycle or next.
        PostponerCore_PostponeNCycles(0);
    }

    s = oldState;
    return slotIndex;
}

void Macros_ValidateAllMacros()
{
    macro_state_t* oldS = s;
    scheduler_state_t schedulerState = Macros_SchedulerState;
    memset(&Macros_SchedulerState, 0, sizeof Macros_SchedulerState);
    Macros_DryRun = true;
    for (uint8_t macroIndex = 0; macroIndex < AllMacrosCount; macroIndex++) {
        uint8_t slotIndex = initMacro(macroIndex, NULL, 255);

        if (slotIndex == 255) {
            s = NULL;
            continue;
        }

        scheduleSlot(slotIndex);

        bool macroHasNotEnded = AllMacros[macroIndex].macroActionsCount;
        while (macroHasNotEnded) {
            if (s->ms.currentMacroAction.type == MacroActionType_Command) {
                processCurrentMacroAction();
                Macros_ParserError = false;
            }

            macroHasNotEnded = loadNextCommand() || loadNextAction();
        }
        endMacro();
        s = NULL;
    }
    Macros_DryRun = false;
    Macros_SchedulerState = schedulerState;
    s = oldS;
}

uint8_t Macros_QueueMacro(uint8_t index, key_state_t *keyState, uint8_t queueAfterSlot)
{
    macro_state_t* oldState = s;

    uint8_t slotIndex = initMacro(index, keyState, 255);

    if (slotIndex == 255) {
        return slotIndex;
    }

    uint8_t childSlotIndex = queueAfterSlot;

    while (MacroState[childSlotIndex].ms.parentMacroSlot != 255) {
        childSlotIndex = MacroState[childSlotIndex].ms.parentMacroSlot;
    }

    MacroState[childSlotIndex].ms.parentMacroSlot = slotIndex;
    s->ms.macroSleeping = true;

    s = oldState;
    return slotIndex;
}

macro_result_t continueMacro(void)
{
    Macros_ParserError = false;
    s->as.modifierPostpone = false;
    s->as.modifierSuppressMods = false;

    if (s->ms.postponeNextNCommands > 0) {
        s->as.modifierPostpone = true;
        PostponerCore_PostponeNCycles(1);
    }

    macro_result_t res = MacroResult_YieldFlag;

    if (!s->ms.macroBroken && ((res = processCurrentMacroAction()) & (MacroResult_InProgressFlag | MacroResult_DoneFlag)) && !s->ms.macroBroken) {
        // InProgressFlag means that action is still in progress
        // DoneFlag means that someone has already done epilogue and therefore we should just return the value.
        return res;
    }

    //at this point, current action/command has finished
    s->ms.postponeNextNCommands = s->ms.postponeNextNCommands > 0 ? s->ms.postponeNextNCommands - 1 : 0;

    if ((s->ms.macroBroken) || (!loadNextCommand() && !loadNextAction())) {
        //macro was ended either because it was broken or because we are out of actions to perform.
        return endMacro() | res;
    } else {
        //we are still running - return last action's return value
        return res;
    }
}

macro_result_t Macros_SleepTillKeystateChange()
{
    if(s->ms.oneShotState > 1) {
        return MacroResult_Blocking;
    }
    if (!s->ms.macroSleeping) {
        unscheduleCurrentSlot();
    }
    Macros_WakeMeOnKeystateChange = true;
    s->ms.wakeMeOnKeystateChange = true;
    s->ms.macroSleeping = true;
    return MacroResult_Sleeping;
}

macro_result_t Macros_SleepTillTime(uint32_t time)
{
    if(s->ms.oneShotState > 1) {
        return MacroResult_Blocking;
    }
    if (!s->ms.macroSleeping) {
        unscheduleCurrentSlot();
    }
    EventScheduler_Schedule(time, EventSchedulerEvent_MacroWakeOnTime);
    s->ms.wakeMeOnTime = true;
    s->ms.macroSleeping = true;
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
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying && !MacroState[i].ms.macroSleeping) {
            s = &MacroState[i];

            macro_result_t res = MacroResult_Finished;
            uint8_t remainingExecution = Macros_MaxBatchSize;
            while (MacroState[i].ms.macroPlaying && !MacroState[i].ms.macroSleeping && res == MacroResult_Finished && remainingExecution > 0) {
                res = continueMacro();
                remainingExecution --;
            }
        }
    }
    s = NULL;
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
    if (s != NULL && s->ms.currentMacroAction.type == MacroActionType_Command) {
        thisCmd = s->ms.currentMacroAction.cmd.text + s->ms.commandBegin;
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
    if (Macros_SchedulerState.currentSlotIdx == (s - MacroState) && MacroState[Macros_SchedulerState.currentSlotIdx].ms.macroScheduled) {
        MacroState[Macros_SchedulerState.previousSlotIdx].ms.nextSlot = MacroState[Macros_SchedulerState.currentSlotIdx].ms.nextSlot;
        MacroState[Macros_SchedulerState.currentSlotIdx].ms.macroScheduled = false;
        Macros_SchedulerState.lastQueuedSlot = Macros_SchedulerState.lastQueuedSlot == Macros_SchedulerState.currentSlotIdx ? Macros_SchedulerState.previousSlotIdx : Macros_SchedulerState.lastQueuedSlot;
        Macros_SchedulerState.currentSlotIdx = Macros_SchedulerState.previousSlotIdx;
        Macros_SchedulerState.activeSlotCount--;
    } else if (Macros_SchedulerState.currentSlotIdx != (s - MacroState) && !s->ms.macroScheduled) {
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
    bool someoneBlocking = false;
    uint8_t remainingExecution = Macros_MaxBatchSize;
    Macros_SchedulerState.remainingCount = Macros_SchedulerState.activeSlotCount;

    while (Macros_SchedulerState.remainingCount > 0 && remainingExecution > 0) {
        macro_result_t res = MacroResult_YieldFlag;
        s = &MacroState[Macros_SchedulerState.currentSlotIdx];

        if (s->ms.macroPlaying && !s->ms.macroSleeping) {
            IF_DEBUG(checkSchedulerHealth("co1"));
            res = continueMacro();
            IF_DEBUG(checkSchedulerHealth("co2"));
        }

        if ((someoneBlocking = (res & MacroResult_BlockingFlag))) {
            break;
        }

        if ((res & MacroResult_YieldFlag) || !s->ms.macroPlaying || s->ms.macroSleeping) {
            getNextScheduledSlot();
        }

        remainingExecution--;
    }

    if(someoneBlocking || remainingExecution == 0) {
        PostponerCore_PostponeNCycles(0);
    }

    s = NULL;
}

static void applySleepingMods()
{
    bool someoneAlive = false;
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying) {
            s = &MacroState[i];
            someoneAlive = true;
            if ( s->as.modifierPostpone ) {
                PostponerCore_PostponeNCycles(0);
            }
            if ( s->as.modifierSuppressMods ) {
                SuppressMods = true;
            }
        }
    }
    MacroPlaying = someoneAlive;
    s = NULL;
}

void Macros_ContinueMacro(void)
{
    wakeSleepers();

    switch (Macros_Scheduler) {
    case Scheduler_Preemptive:
        executePreemptive();
        applySleepingMods();
        break;
    case Scheduler_Blocking:
        executeBlocking();
        applySleepingMods();
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
