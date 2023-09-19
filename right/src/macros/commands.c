#include "config_parser/config_globals.h"
#include "config_parser/parse_macro.h"
#include "keymap.h"
#include "layer.h"
#include "layer_stack.h"
#include "layer_switcher.h"
#include "ledmap.h"
#include "macro_recorder.h"
#include "macros/debug_commands.h"
#include "macros/core.h"
#include "macros/keyid_parser.h"
#include "macros/scancode_commands.h"
#include "macros/set_command.h"
#include "macros/shortcut_parser.h"
#include "macros/status_buffer.h"
#include "macros/string_reader.h"
#include "macros/typedefs.h"
#include "macros/vars.h"
#include "postponer.h"
#include "secondary_role_driver.h"
#include "segment_display.h"
#include "slave_drivers/uhk_module_driver.h"
#include "str_utils.h"
#include "timer.h"
#include "usb_report_updater.h"
#include "utils.h"
#include "debug.h"

static uint8_t lastLayerIdx;
static uint8_t lastLayerKeymapIdx;
static uint8_t lastKeymapIdx;

static int32_t regs[MAX_REG_COUNT];

static macro_result_t processCommand(parser_context_t* ctx);

static macro_result_t processDelay(uint32_t time)
{
    if (S->as.actionActive) {
        if (Timer_GetElapsedTime(&S->as.delayData.start) >= time) {
            S->as.actionActive = false;
            S->as.delayData.start = 0;
            return MacroResult_Finished;
        }
        Macros_SleepTillTime(S->as.delayData.start + time);
        return MacroResult_Sleeping;
    } else {
        S->as.delayData.start = CurrentTime;
        S->as.actionActive = true;
        return processDelay(time);
    }
}

macro_result_t Macros_ProcessDelayAction()
{
    return processDelay(S->ms.currentMacroAction.delay.delay);
}

static void postponeNextN(uint8_t count)
{
    S->ms.postponeNextNCommands = count + 1;
    S->as.modifierPostpone = true;
    PostponerCore_PostponeNCycles(MACRO_CYCLES_TO_POSTPONE);
}

static void postponeCurrentCycle()
{
    PostponerCore_PostponeNCycles(MACRO_CYCLES_TO_POSTPONE);
    S->as.modifierPostpone = true;
}

/**
 * Both key press and release are subject to postponing, therefore we need to ensure
 * that macros which actively initiate postponing and wait until release ignore
 * postponed key releases. The s->postponeNext indicates that the running macro
 * initiates postponing in the current cycle.
 */
bool Macros_CurrentMacroKeyIsActive()
{
    if (S->ms.currentMacroKey == NULL) {
        return S->ms.oneShotState;
    }
    if (S->ms.postponeNextNCommands > 0 || S->as.modifierPostpone) {
        bool keyIsActive = (KeyState_Active(S->ms.currentMacroKey) && !PostponerQuery_IsKeyReleased(S->ms.currentMacroKey));
        return  keyIsActive || S->ms.oneShotState;
    } else {
        bool keyIsActive = KeyState_Active(S->ms.currentMacroKey);
        return keyIsActive || S->ms.oneShotState;
    }
}

static bool validReg(uint8_t idx, const char* pos)
{
    if (idx >= MAX_REG_COUNT) {
        Macros_ReportErrorNum("Invalid register index:", idx, pos);
        return false;
    }
    return true;
}

static macro_result_t writeNum(uint32_t a)
{
    char num[11];
    num[10] = '\0';
    int at = 10;
    while ((a > 0 || at == 10) && at > 0) {
        at--;
        num[at] = a % 10 + 48;
        a = a/10;
    }
    num[at-1] = '0';

    uint8_t len = MAX(2, 10-at);

    macro_result_t res = Macros_DispatchText(&num[10-len], len, true);
    if (res == MacroResult_Finished) {
        PostponerExtended_ConsumePendingKeypresses(1, true);
        return MacroResult_Finished;
    }
    return res;
}

static bool isNUM(parser_context_t* ctx)
{
    switch(*ctx->at) {
    case '0'...'9':
    case '#':
    case '-':
    case '@':
    case '%':
    case '$':
    case '(':
        return true;
    default:
        return false;
    }
}

int32_t Macros_ParseInt(const char *a, const char *aEnd, const char* *parsedTill)
{
    if (*a == '#') {
        if (Macros_DryRun) {
            Macros_ReportWarn("Registers are now deprecated. Please, replace them with named variables. E.g., `setVar foo 1` and `$foo`.", a, a);
        }
        a++;
        if (TokenMatches(a, aEnd, "key")) {
            Macros_ReportWarn("Registers are now deprecated. Please, replace #key with $thisKeyId.", a, a);
            if (parsedTill != NULL) {
                *parsedTill = a+3;
            }
            return Utils_KeyStateToKeyId(S->ms.currentMacroKey);
        }
        uint8_t adr = Macros_ParseInt(a, aEnd, parsedTill);
        if (validReg(adr, a)) {
            return regs[adr];
        } else {
            return 0;
        }
    }
    else if (*a == '%') {
        Macros_ReportWarn("`%` notation is now deprecated. Please, replace it with $queuedKeyId.<index> notation. E.g., `$queuedKeyId.1`.", a, a);
        a++;
        uint8_t idx = Macros_ParseInt(a, aEnd, parsedTill);
        if (idx >= PostponerQuery_PendingKeypressCount()) {
            if (!Macros_DryRun) {
                Macros_ReportError("Not enough pending keys! Note that this is zero-indexed!",  a, a);
            }
            return 0;
        }
        return PostponerExtended_PendingId(idx);
    }
    else if (*a == '@') {
        Macros_ReportWarn("`@` notation is now deprecated. Please, replace it with $currentAddress. E.g., `@3` with `$($currentAddress + 3)`.", a, a);
        a++;
        return S->ms.commandAddress + Macros_ParseInt(a, aEnd, parsedTill);
    }
    else
    {
        parser_context_t ctx = { .macroState = S, .begin = a, .at = a, .end = aEnd };
        int32_t res = Macros_ConsumeInt(&ctx);
        if (parsedTill != NULL) {
            *parsedTill = ctx.at;
        }
        return res;
    }
}

int32_t Macros_LegacyConsumeInt(parser_context_t* ctx)
{
    const char* parsedTill;
    int32_t res = Macros_ParseInt(ctx->at, ctx->end, &parsedTill);
    ctx->at = parsedTill;
    ConsumeWhite(ctx);
    return res;
}

bool Macros_ParseBoolean(const char *a, const char *aEnd)
{
    parser_context_t ctx = { .macroState = S, .begin = a, .at = a, .end = aEnd };
    return Macros_ConsumeBool(&ctx);
}


static int32_t consumeRuntimeMacroSlotId(parser_context_t* ctx)
{
    const char* end = TokEnd(ctx->at, ctx->end);
    static uint16_t lastMacroId = 0;
    if (ConsumeToken(ctx, "last")) {
        return lastMacroId;
    }
    else if (ctx->at == ctx->end) {
        lastMacroId = Utils_KeyStateToKeyId(S->ms.currentMacroKey);
    }
    else if (end == ctx->at+1) {
        lastMacroId = (uint8_t)(*ctx->at);
        ctx->at++;
    }
    else {
        lastMacroId = 128 + Macros_LegacyConsumeInt(ctx);
    }
    return lastMacroId;
}


static macro_result_t processStopAllMacrosCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (&MacroState[i] != S) {
            MacroState[i].ms.macroBroken = true;
            MacroState[i].ms.macroSleeping = false;
        }
    }
    return MacroResult_Finished;
}

static uint8_t consumeKeymapId(parser_context_t* ctx)
{
    if (ConsumeToken(ctx, "last")) {
        return lastKeymapIdx;
    } else {
        uint8_t len = TokLen(ctx->at, ctx->end);
        uint8_t idx = FindKeymapByAbbreviation(len, ctx->at);
        if (idx == 0xFF) {
            Macros_ReportError("Keymap not recognized:", ctx->at, ctx->at);
        }
        ctx->at += len;
        ConsumeWhite(ctx);
        return idx;
    }
}


uint8_t Macros_ConsumeLayerId(parser_context_t* ctx)
{
    switch(*ctx->at) {
        case 'a':
            if (ConsumeToken(ctx, "alt")) {
                return LayerId_Alt;
            }
            break;
        case 'b':
            if (ConsumeToken(ctx, "base")) {
                return LayerId_Base;
            }
            break;
        case 'c':
            if (ConsumeToken(ctx, "current")) {
                return ActiveLayer;
            } else if (ConsumeToken(ctx, "ctrl")) {
                return LayerId_Ctrl;
            } else if (ConsumeToken(ctx, "control")) {
                return LayerId_Ctrl;
            }
            break;
        case 'f':
            if (ConsumeToken(ctx, "fn")) {
                return LayerId_Fn;
            } else if (ConsumeToken(ctx, "fn2")) {
                return LayerId_Fn2;
            } else if (ConsumeToken(ctx, "fn3")) {
                return LayerId_Fn3;
            } else if (ConsumeToken(ctx, "fn4")) {
                return LayerId_Fn4;
            } else if (ConsumeToken(ctx, "fn5")) {
                return LayerId_Fn5;
            }
            break;
        case 'g':
            if (ConsumeToken(ctx, "gui")) {
                return LayerId_Gui;
            }
            break;
        case 'l':
            if (ConsumeToken(ctx, "last")) {
                return lastLayerIdx;
            }
            break;
        case 'm':
            if (ConsumeToken(ctx, "mouse")) {
                return LayerId_Mouse;
            } else if (ConsumeToken(ctx, "mod")) {
                return LayerId_Mod;
            }
            break;
        case 'p':
            if (ConsumeToken(ctx, "previous")) {
                return LayerStack[LayerStack_FindPreviousLayerRecordIdx()].layer;
            }
            break;
        case 's':
            if (ConsumeToken(ctx, "shift")) {
                return LayerId_Shift;
            } else if (ConsumeToken(ctx, "super")) {
                return LayerId_Gui;
            }
            break;
    }

    Macros_ReportError("Unrecognized layer:", ctx->at, ctx->end);
    return LayerId_Base;
}


uint8_t Macros_ParseLayerId(const char* arg1, const char* cmdEnd)
{
    parser_context_t ctx = { .macroState = S, .begin = arg1, .at = arg1, .end = cmdEnd };
    return Macros_ConsumeLayerId(&ctx);
}



static uint8_t parseLayerKeymapId(parser_context_t* ctx)
{
    parser_context_t bakCtx = *ctx;
    if (ConsumeToken(&bakCtx, "last")) {
        return lastLayerKeymapIdx;
    }
    else if (ConsumeToken(&bakCtx, "previous")) {
        return LayerStack[LayerStack_FindPreviousLayerRecordIdx()].keymap;
    }
    else {
        return CurrentKeymapIndex;
    }
}

static macro_result_t processSwitchKeymapCommand(parser_context_t* ctx)
{
    uint8_t tmpKeymapIdx = CurrentKeymapIndex;
    {
        uint8_t newKeymapIdx = consumeKeymapId(ctx);

        if (Macros_ParserError) {
            return MacroResult_Finished;
        }
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }

        SwitchKeymapById(newKeymapIdx);
        LayerStack_Reset();
    }
    lastKeymapIdx = tmpKeymapIdx;
    return MacroResult_Finished;
}

/**DEPRECATED**/
static macro_result_t processSwitchKeymapLayerCommand(parser_context_t* ctx)
{
    Macros_ReportWarn("Command deprecated. Please, replace switchKeymapLayer by toggleKeymapLayer or holdKeymapLayer. Or complain on github that you actually need this command.", ctx->at, ctx->at);
    uint8_t tmpLayerIdx = LayerStack_ActiveLayer;
    uint8_t tmpLayerKeymapIdx = CurrentKeymapIndex;
    uint8_t keymap = consumeKeymapId(ctx);
    uint8_t layer = Macros_ConsumeLayerId(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    LayerStack_Push(layer, keymap, false);
    lastLayerIdx = tmpLayerIdx;
    lastLayerKeymapIdx = tmpLayerKeymapIdx;
    return MacroResult_Finished;
}

/**DEPRECATED**/
static macro_result_t processSwitchLayerCommand(parser_context_t* ctx)
{
    Macros_ReportWarn("Command deprecated. Please, replace switchLayer by toggleLayer or holdLayer. Or complain on github that you actually need this command.", ctx->at, ctx->at);
    uint8_t tmpLayerIdx = LayerStack_ActiveLayer;
    uint8_t tmpLayerKeymapIdx = CurrentKeymapIndex;
    if (ConsumeToken(ctx, "previous")) {
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        LayerStack_Pop(true, false);
    }
    else {
        uint8_t keymap = parseLayerKeymapId(ctx);
        uint8_t layer = Macros_ConsumeLayerId(ctx);

        if (Macros_ParserError) {
            return MacroResult_Finished;
        }
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }

        LayerStack_Push(layer, keymap, false);
    }
    lastLayerIdx = tmpLayerIdx;
    lastLayerKeymapIdx = tmpLayerKeymapIdx;
    return MacroResult_Finished;
}


static macro_result_t processToggleKeymapLayerCommand(parser_context_t* ctx)
{
    uint8_t tmpLayerIdx = LayerStack_ActiveLayer;
    uint8_t tmpLayerKeymapIdx = CurrentKeymapIndex;
    uint8_t keymap = consumeKeymapId(ctx);
    uint8_t layer = Macros_ConsumeLayerId(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    LayerStack_Push(layer, keymap, false);
    lastLayerIdx = tmpLayerIdx;
    lastLayerKeymapIdx = tmpLayerKeymapIdx;
    return MacroResult_Finished;
}

static macro_result_t processToggleLayerCommand(parser_context_t* ctx)
{
    uint8_t tmpLayerIdx = LayerStack_ActiveLayer;
    uint8_t tmpLayerKeymapIdx = CurrentKeymapIndex;
    uint8_t keymap = parseLayerKeymapId(ctx);
    uint8_t layer = Macros_ConsumeLayerId(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    LayerStack_Push(layer, keymap, false);
    lastLayerIdx = tmpLayerIdx;
    lastLayerKeymapIdx = tmpLayerKeymapIdx;
    return MacroResult_Finished;
}

static macro_result_t processUnToggleLayerCommand()
{
    uint8_t tmpLayerIdx = LayerStack_ActiveLayer;
    uint8_t tmpLayerKeymapIdx = CurrentKeymapIndex;

    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    LayerStack_Pop(true, true);
    lastLayerIdx = tmpLayerIdx;
    lastLayerKeymapIdx = tmpLayerKeymapIdx;
    return MacroResult_Finished;
}

static macro_result_t processHoldLayer(uint8_t layer, uint8_t keymap, uint16_t timeout)
{
    if (!S->as.actionActive) {
        S->as.actionActive = true;
        LayerStack_Push(layer, keymap, true);
        S->as.holdLayerData.layerStackIdx = LayerStack_TopIdx;
        return MacroResult_Waiting;
    }
    else {
        if (Macros_CurrentMacroKeyIsActive() && (Timer_GetElapsedTime(&S->ms.currentMacroStartTime) < timeout || S->ms.macroInterrupted)) {
            if (!S->ms.macroInterrupted) {
                Macros_SleepTillTime(S->ms.currentMacroStartTime + timeout);
            }
            Macros_SleepTillKeystateChange();
            return MacroResult_Sleeping;
        }
        else {
            S->as.actionActive = false;
            LayerStack_RemoveRecord(S->as.holdLayerData.layerStackIdx);
            LayerStack_Pop(false, false);
            return MacroResult_Finished;
        }
    }
}

static macro_result_t processHoldLayerCommand(parser_context_t* ctx)
{
    uint8_t layer = Macros_ConsumeLayerId(ctx);
    uint8_t keymap = parseLayerKeymapId(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    return processHoldLayer(layer, keymap, 0xFFFF);
}

static macro_result_t processHoldLayerMaxCommand(parser_context_t* ctx)
{
    uint8_t keymap = parseLayerKeymapId(ctx);
    uint8_t layer = Macros_ConsumeLayerId(ctx);
    uint16_t timeout = Macros_LegacyConsumeInt(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    return processHoldLayer(layer, keymap, timeout);
}

static macro_result_t processHoldKeymapLayerCommand(parser_context_t* ctx)
{
    uint8_t keymap = consumeKeymapId(ctx);
    uint8_t layer = Macros_ConsumeLayerId(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    return processHoldLayer(layer, keymap, 0xFFFF);
}

static macro_result_t processHoldKeymapLayerMaxCommand(parser_context_t* ctx)
{
    uint8_t keymap = consumeKeymapId(ctx);
    uint8_t layer = Macros_ConsumeLayerId(ctx);
    uint16_t timeout = Macros_LegacyConsumeInt(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    return processHoldLayer(layer, keymap, timeout);
}

static macro_result_t processDelayUntilReleaseMaxCommand(parser_context_t* ctx)
{
    uint32_t timeout = Macros_LegacyConsumeInt(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    if (Macros_CurrentMacroKeyIsActive() && Timer_GetElapsedTime(&S->ms.currentMacroStartTime) < timeout) {
        Macros_SleepTillKeystateChange();
        Macros_SleepTillTime(S->ms.currentMacroStartTime + timeout);
        return MacroResult_Sleeping;
    }
    return MacroResult_Finished;
}

static macro_result_t processDelayUntilReleaseCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    if (Macros_CurrentMacroKeyIsActive()) {
        return Macros_SleepTillKeystateChange();
    }
    return MacroResult_Finished;
}

static macro_result_t processDelayUntilCommand(parser_context_t* ctx)
{
    uint32_t time = Macros_LegacyConsumeInt(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    return processDelay(time);
}

static macro_result_t processRecordMacroDelayCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    if (Macros_CurrentMacroKeyIsActive()) {
        return MacroResult_Waiting;
    }
    uint16_t delay = Timer_GetElapsedTime(&S->ms.currentMacroStartTime);
    MacroRecorder_RecordDelay(delay);
    return MacroResult_Finished;
}

static bool processIfDoubletapCommand(bool negate)
{
    if (Macros_DryRun) {
        return true;
    }
    bool doubletapFound = false;

    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (S->ms.currentMacroStartTime - MacroState[i].ps.previousMacroStartTime <= DoubletapConditionTimeout && S->ms.currentMacroIndex == MacroState[i].ps.previousMacroIndex) {
            doubletapFound = true;
        }
        if (
            MacroState[i].ms.macroPlaying &&
            MacroState[i].ms.currentMacroStartTime < S->ms.currentMacroStartTime &&
            S->ms.currentMacroStartTime - MacroState[i].ms.currentMacroStartTime <= DoubletapConditionTimeout &&
            S->ms.currentMacroIndex == MacroState[i].ms.currentMacroIndex &&
            &MacroState[i] != S
        ) {
            doubletapFound = true;
        }
    }

    return doubletapFound != negate;
}

static bool processIfModifierCommand(bool negate, uint8_t modmask)
{
    if (Macros_DryRun) {
        return true;
    }
    return ((InputModifiersPrevious & modmask) > 0) != negate;
}

static bool processIfStateKeyCommand(bool negate, bool *state)
{
    if (Macros_DryRun) {
        return true;
    }
    return *state != negate;
}

static bool processIfRecordingCommand(bool negate)
{
    if (Macros_DryRun) {
        return true;
    }
    return MacroRecorder_IsRecording() != negate;
}

static bool processIfRecordingIdCommand(parser_context_t* ctx, bool negate)
{
    uint16_t id = consumeRuntimeMacroSlotId(ctx);
    if (Macros_DryRun) {
        return true;
    }
    bool res = MacroRecorder_RecordingId() == id;
    return res != negate;
}

static bool processIfPendingCommand(parser_context_t* ctx, bool negate)
{
    uint32_t cnt = Macros_LegacyConsumeInt(ctx);
    if (Macros_DryRun) {
        return true;
    }

    return (PostponerQuery_PendingKeypressCount() >= cnt) != negate;
}

static bool processIfPlaytimeCommand(parser_context_t* ctx, bool negate)
{
    uint32_t timeout = Macros_LegacyConsumeInt(ctx);
    if (Macros_DryRun) {
        return true;
    }
    uint32_t delay = Timer_GetElapsedTime(&S->ms.currentMacroStartTime);
    return (delay > timeout) != negate;
}

static bool processIfInterruptedCommand(bool negate)
{
    if (Macros_DryRun) {
        return true;
    }
    return S->ms.macroInterrupted != negate;
}

static bool processIfReleasedCommand(bool negate)
{
    if (Macros_DryRun) {
        return true;
    }
   return (!Macros_CurrentMacroKeyIsActive()) != negate;
}

static bool processIfRegEqCommand(parser_context_t* ctx, bool negate)
{
    Macros_ReportWarn("Command is deprecated, please use command similar to `if ($varName == 1)`.", ctx->at, ctx->at);
    uint8_t address = Macros_LegacyConsumeInt(ctx);
    int32_t param = Macros_LegacyConsumeInt(ctx);
    bool regIsValid = validReg(address, ctx->at);
    if (Macros_DryRun) {
        return true;
    }
    if (regIsValid) {
        bool res = regs[address] == param;
        return res != negate;
    } else {
        return false;
    }
}

static bool processIfRegInequalityCommand(parser_context_t* ctx, bool greaterThan)
{
    Macros_ReportWarn("Command is deprecated, please use command similar to `if ($varName >= 1)`.", ctx->at, ctx->at);
    uint8_t address = Macros_LegacyConsumeInt(ctx);
    int32_t param = Macros_LegacyConsumeInt(ctx);
    bool regIsValid = validReg(address, ctx->at);
    if (Macros_DryRun) {
        return true;
    }
    if (regIsValid) {
        if (greaterThan) {
            return regs[address] > param;
        } else {
            return regs[address] < param;
        }
    } else {
        return false;
    }
}

static bool processIfKeymapCommand(parser_context_t* ctx, bool negate)
{
    uint8_t queryKeymapIdx = consumeKeymapId(ctx);
    if (Macros_DryRun) {
        return true;
    }
    return (queryKeymapIdx == CurrentKeymapIndex) != negate;
}

static bool processIfLayerCommand(parser_context_t* ctx, bool negate)
{
    uint8_t queryLayerIdx = Macros_ConsumeLayerId(ctx);
    if (Macros_DryRun) {
        return true;
    }
    return (queryLayerIdx == LayerStack_ActiveLayer) != negate;
}

static bool processIfCommand(parser_context_t* ctx)
{
    bool res = Macros_ConsumeBool(ctx);

    if (Macros_DryRun) {
        return true;
    }

    return res;
}

static macro_result_t processBreakCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    S->ms.macroBroken = true;
    return MacroResult_Finished;
}

static macro_result_t processSetLedTxtCommand(parser_context_t* ctx)
{
    int16_t time = Macros_LegacyConsumeInt(ctx);
    char text[3];
    uint8_t textLen = 0;

    if (isNUM(ctx)) {
        macro_variable_t value = Macros_ConsumeAnyValue(ctx);
        SegmentDisplay_SerializeVar(text, value);
        textLen = 3;
    } else if (ctx->at != ctx->end) {
        uint16_t stringOffset = 0, textIndex = 0, textSubIndex = 0;
        for (uint8_t i = 0; i < 3; i++) {
            text[i] = Macros_ConsumeCharOfString(ctx, &stringOffset, &textIndex, &textSubIndex);
            if (text[i] != '\0') {
                textLen++;
            }
        }
        ctx->at += stringOffset;
        ctx->at += textIndex;
        ConsumeWhite(ctx);
    } else {
        Macros_ReportError("Text argument expected.", ctx->at, ctx->at);
        return MacroResult_Finished;
    }

    if (Macros_DryRun || Macros_ParserError) {
        return MacroResult_Finished;
    }

    macro_result_t res = MacroResult_Finished;
    if (time == 0) {
        SegmentDisplay_SetText(textLen, text, SegmentDisplaySlot_Macro);
        return MacroResult_Finished;
    } else if ((res = processDelay(time)) == MacroResult_Finished) {
        SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Macro);
        return MacroResult_Finished;
    } else {
        SegmentDisplay_SetText(textLen, text, SegmentDisplaySlot_Macro);
        return res;
    }
}

static macro_result_t processSetRegCommand(parser_context_t* ctx)
{
    uint8_t address = Macros_LegacyConsumeInt(ctx);
    int32_t param = Macros_LegacyConsumeInt(ctx);
    bool regIsValid = validReg(address, ctx->at);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    if (regIsValid) {
        regs[address] = param;
    }
    return MacroResult_Finished;
}

static macro_result_t processRegAddCommand(parser_context_t* ctx, bool invert)
{
    Macros_ReportWarn("Command is deprecated, please use command similar to `setVar varName ($varName+1)`.", ctx->at, ctx->at);
    uint8_t address = Macros_LegacyConsumeInt(ctx);
    int32_t param = Macros_LegacyConsumeInt(ctx);
    bool regIsValid = validReg(address, ctx->at);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    if (regIsValid) {
        if (invert) {
            regs[address] = regs[address] - param;
        } else {
            regs[address] = regs[address] + param;
        }
    }
    return MacroResult_Finished;
}

static macro_result_t processRegMulCommand(parser_context_t* ctx)
{
    Macros_ReportWarn("Command is deprecated, please use command similar to `setVar varName ($varName*2)`.", ctx->at, ctx->at);
    uint8_t address = Macros_LegacyConsumeInt(ctx);
    int32_t param = Macros_LegacyConsumeInt(ctx);
    bool regIsValid = validReg(address, ctx->at);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    if (regIsValid) {
        regs[address] = regs[address]*param;
    }
    return MacroResult_Finished;
}

macro_result_t goTo(parser_context_t* ctx)
{
    if (isNUM(ctx)) {
        if (Macros_DryRun) {
            Macros_LegacyConsumeInt(ctx);
            return MacroResult_Finished;
        }
        return Macros_GoToAddress(Macros_LegacyConsumeInt(ctx));
    } else {
        if (Macros_DryRun) {
            ConsumeAnyIdentifier(ctx);
            return MacroResult_Finished;
        }
        return Macros_GoToLabel(ctx);
    }
}


static macro_result_t processGoToCommand(parser_context_t* ctx)
{
    return goTo(ctx);
}

static macro_result_t processYieldCommand(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    return MacroResult_ActionFinishedFlag | MacroResult_YieldFlag;
}

static macro_result_t processStopRecordingCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    MacroRecorder_StopRecording();
    return MacroResult_Finished;
}

static macro_result_t processMouseCommand(parser_context_t* ctx, bool enable)
{
    uint8_t dirOffset = 0;

    serialized_mouse_action_t baseAction = SerializedMouseAction_LeftClick;

    if (ConsumeToken(ctx, "move")) {
        baseAction = SerializedMouseAction_MoveUp;
    }
    else if (ConsumeToken(ctx, "scroll")) {
        baseAction = SerializedMouseAction_ScrollUp;
    }
    else if (ConsumeToken(ctx, "accelerate")) {
        baseAction = SerializedMouseAction_Accelerate;
    }
    else if (ConsumeToken(ctx, "decelerate")) {
        baseAction = SerializedMouseAction_Decelerate;
    }
    else {
        Macros_ReportError("Unrecognized argument:", ctx->at, ctx->end);
        return MacroResult_Finished;
    }

    if (baseAction == SerializedMouseAction_MoveUp || baseAction == SerializedMouseAction_ScrollUp) {
        if (ConsumeToken(ctx, "up")) {
            dirOffset = 0;
        }
        else if (ConsumeToken(ctx, "down")) {
            dirOffset = 1;
        }
        else if (ConsumeToken(ctx, "left")) {
            dirOffset = 2;
        }
        else if (ConsumeToken(ctx, "right")) {
            dirOffset = 3;
        }
        else {
            Macros_ReportError("Unrecognized argument:", ctx->at, ctx->end);
            return MacroResult_Finished;
        }
    }

    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    if (baseAction != SerializedMouseAction_LeftClick) {
        ToggleMouseState(baseAction + dirOffset, enable);
    }
    return MacroResult_Finished;
}

static macro_result_t processRecordMacroCommand(parser_context_t* ctx, bool blind)
{
    uint16_t id = consumeRuntimeMacroSlotId(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    MacroRecorder_RecordRuntimeMacroSmart(id, blind);
    return MacroResult_Finished;
}

static macro_result_t processStartRecordingCommand(parser_context_t* ctx, bool blind)
{
    uint16_t id = consumeRuntimeMacroSlotId(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    MacroRecorder_StartRecording(id, blind);
    return MacroResult_Finished;
}

static macro_result_t processPlayMacroCommand(parser_context_t* ctx)
{
    S->ms.reportsUsed = true;
    uint16_t id = consumeRuntimeMacroSlotId(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    bool res = MacroRecorder_PlayRuntimeMacroSmart(id, &S->ms.macroBasicKeyboardReport);
    return res ? MacroResult_Blocking : MacroResult_Finished;
}

static macro_result_t processWriteCommand(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        uint16_t stringOffset = 0;
        uint16_t textIndex = 0;
        uint16_t textSubIndex = 0;

        while (Macros_ConsumeCharOfString(ctx, &stringOffset, &textIndex, &textSubIndex) != '\0') {};

        return MacroResult_Finished;
    }
    return Macros_DispatchText(ctx->at, ctx->end - ctx->at, false);
}

static macro_result_t processWriteExprCommand(parser_context_t* ctx)
{
    Macros_ReportWarn("writeExpr is now deprecated, please migrate to interpolated strings", ctx->at, ctx->at);
    uint32_t num = Macros_LegacyConsumeInt(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    return writeNum(num);
}

static void processSuppressModsCommand()
{
    if (Macros_DryRun) {
        return;
    }
    SuppressMods = true;
    S->as.modifierSuppressMods = true;
}

static void processPostponeKeysCommand()
{
    if (Macros_DryRun) {
        return;
    }
    postponeCurrentCycle();
    S->as.modifierSuppressMods = true;
}

static macro_result_t processNoOpCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    if (!S->as.actionActive) {
        S->as.actionActive = true;
        return MacroResult_Blocking;
    } else {
        S->as.actionActive = false;
        return MacroResult_Finished;
    }
}

#define RESOLVESEC_RESULT_DONTKNOWYET 0
#define RESOLVESEC_RESULT_PRIMARY 1
#define RESOLVESEC_RESULT_SECONDARY 2


static uint8_t processResolveSecondary(uint16_t timeout1, uint16_t timeout2)
{
    postponeCurrentCycle();
    bool pendingReleased = PostponerExtended_IsPendingKeyReleased(0);
    bool currentKeyIsActive = Macros_CurrentMacroKeyIsActive();

    //phase 1 - wait until some other key is released, then write down its release time
    bool timer1Exceeded = Timer_GetElapsedTime(&S->ms.currentMacroStartTime) >= timeout1;
    if (!timer1Exceeded && currentKeyIsActive && !pendingReleased) {
        S->as.secondaryRoleData.phase2Start = 0;
        return RESOLVESEC_RESULT_DONTKNOWYET;
    }
    if (S->as.secondaryRoleData.phase2Start == 0) {
        S->as.secondaryRoleData.phase2Start = CurrentTime;
    }
    //phase 2 - "safety margin" - wait another `timeout2` ms, and if the switcher is released during this time, still interpret it as a primary action
    bool timer2Exceeded = Timer_GetElapsedTime(&S->as.secondaryRoleData.phase2Start) >= timeout2;
    if (!timer1Exceeded && !timer2Exceeded &&  currentKeyIsActive && pendingReleased && PostponerQuery_PendingKeypressCount() < 3) {
        return RESOLVESEC_RESULT_DONTKNOWYET;
    }
    //phase 3 - resolve the situation - if the switcher is released first or within the "safety margin", interpret it as primary action, otherwise secondary
    if (timer1Exceeded || (pendingReleased && timer2Exceeded)) {
        return RESOLVESEC_RESULT_SECONDARY;
    }
    else {
        return RESOLVESEC_RESULT_PRIMARY;
    }

}

static macro_result_t processResolveSecondaryCommand(parser_context_t* ctx)
{
    const char* argEnd = ctx->end;
    const char* arg1 = ctx->at;
    const char* arg2 = NextTok(arg1, argEnd);
    const char* arg3 = NextTok(arg2, argEnd);
    const char* arg4 = NextTok(arg3, argEnd);

    const char* primaryAdr;
    const char* secondaryAdr;
    uint16_t timeout1;
    uint16_t timeout2;

    if (arg4 == argEnd) {
        timeout1 = Macros_ParseInt(arg1, argEnd, NULL);
        timeout2 = timeout1;
        primaryAdr = arg2;
        secondaryAdr = arg3;
    } else {
        timeout1 = Macros_ParseInt(arg1, argEnd, NULL);
        timeout2 = Macros_ParseInt(arg2, argEnd, NULL);
        primaryAdr = arg3;
        secondaryAdr = arg4;
    }

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        Macros_ReportWarn("Command deprecated. Please, replace resolveSecondary by `ifPrimary advancedStrategy goTo ...` or `ifSecondary advancedStrategy goTo ...`.", NULL, NULL);
        return MacroResult_Finished;
    }

    uint8_t res = processResolveSecondary(timeout1, timeout2);

    switch(res) {
    case RESOLVESEC_RESULT_DONTKNOWYET:
        return MacroResult_Waiting;
    case RESOLVESEC_RESULT_PRIMARY: {
            postponeNextN(1);
            parser_context_t ctx2 = { .macroState = S, .begin = arg1, .at = primaryAdr, .end = argEnd};
            return goTo(&ctx2);
        }
    case RESOLVESEC_RESULT_SECONDARY: {
            parser_context_t ctx2 = { .macroState = S, .begin = arg1, .at = secondaryAdr, .end = argEnd};
            return goTo(&ctx2);
        }
    }
    //this is unreachable, prevents warning
    return MacroResult_Finished;
}



static macro_result_t processIfSecondaryCommand(parser_context_t* ctx, bool negate)
{
    secondary_role_strategy_t strategy = SecondaryRoles_Strategy;

    if (ConsumeToken(ctx, "simpleStrategy")) {
        strategy = SecondaryRoleStrategy_Simple;
    }
    else if (ConsumeToken(ctx, "advancedStrategy")) {
        strategy = SecondaryRoleStrategy_Advanced;
    }

    if (Macros_DryRun) {
        goto conditionPassed;
    }

    if (S->as.currentIfSecondaryConditionPassed) {
        if (S->as.currentConditionPassed) {
            goto conditionPassed;
        } else {
            S->as.currentIfSecondaryConditionPassed = false;
        }
    }

    postponeCurrentCycle();
    secondary_role_result_t res = SecondaryRoles_ResolveState(S->ms.currentMacroKey, 0, strategy, !S->as.actionActive);

    S->as.actionActive = res.state == SecondaryRoleState_DontKnowYet;

    switch(res.state) {
    case SecondaryRoleState_DontKnowYet:
        return MacroResult_Waiting;
    case SecondaryRoleState_Primary:
        if (negate) {
            goto conditionPassed;
        } else {
            postponeNextN(0);
            return MacroResult_Finished | MacroResult_ConditionFailedFlag;
        }
    case SecondaryRoleState_Secondary:
        if (negate) {
            return MacroResult_Finished | MacroResult_ConditionFailedFlag;
        } else {
            goto conditionPassed;
        }
    }
conditionPassed:
    S->as.currentIfSecondaryConditionPassed = true;
    S->as.currentConditionPassed = false; //otherwise following conditions would be skipped
    return processCommand(ctx);
}


static macro_result_t processResolveNextKeyIdCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    postponeCurrentCycle();
    if (PostponerQuery_PendingKeypressCount() == 0) {
        return MacroResult_Waiting;
    }
    macro_result_t res = writeNum(PostponerExtended_PendingId(0));
    if (res == MacroResult_Finished) {
        PostponerExtended_ConsumePendingKeypresses(1, true);
        return MacroResult_Finished;
    }
    return res;
}

static macro_result_t processResolveNextKeyEqCommand(parser_context_t* ctx)
{
    Macros_ReportWarn("Command deprecated. Please, replace resolveNextKeyEq by ifShortcut or ifGesture, or complain at github that you actually need this.", NULL, NULL);
    const char* argEnd = ctx->end;
    const char* arg1 = ctx->at;
    const char* arg2 = NextTok(arg1, argEnd);
    const char* arg3 = NextTok(arg2, argEnd);
    const char* arg4 = NextTok(arg3, argEnd);
    const char* arg5 = NextTok(arg4, argEnd);
    uint16_t idx = Macros_ParseInt(arg1, argEnd, NULL);
    uint16_t key = Macros_ParseInt(arg2, argEnd, NULL);
    uint16_t timeout = 0;
    bool untilRelease = false;
    if (TokenMatches(arg3, argEnd, "untilRelease")) {
       untilRelease = true;
    } else {
       timeout = Macros_ParseInt(arg3, argEnd, NULL);
    }
    const char* adr1 = arg4;
    const char* adr2 = arg5;


    if (idx > POSTPONER_BUFFER_MAX_FILL) {
        Macros_ReportErrorNum("Invalid argument 1, allowed at most:", idx, arg1);
    }

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    postponeCurrentCycle();

    if (untilRelease ? !Macros_CurrentMacroKeyIsActive() : Timer_GetElapsedTime(&S->ms.currentMacroStartTime) >= timeout) {
        ctx->at = adr2;
        return goTo(ctx);
    }
    if (PostponerQuery_PendingKeypressCount() < idx + 1) {
        return MacroResult_Waiting;
    }

    if (PostponerExtended_PendingId(idx) == key) {
        ctx->at = adr1;
        return goTo(ctx);
    } else {
        ctx->at = adr2;
        return goTo(ctx);
    }
}

static macro_result_t processIfShortcutCommand(parser_context_t* ctx, bool negate, bool untilRelease)
{
    //parse optional flags
    bool consume = true;
    bool transitive = false;
    bool fixedOrder = true;
    bool orGate = false;
    uint16_t cancelIn = 0;
    uint16_t timeoutIn= 0;

    bool parsingOptions = true;
    while(parsingOptions) {
        if (ConsumeToken(ctx, "noConsume")) {
            consume = false;
        } else if (ConsumeToken(ctx, "transitive")) {
            transitive = true;
        } else if (ConsumeToken(ctx, "timeoutIn")) {
            timeoutIn = Macros_LegacyConsumeInt(ctx);
        } else if (ConsumeToken(ctx, "cancelIn")) {
            cancelIn = Macros_LegacyConsumeInt(ctx);
        } else if (ConsumeToken(ctx, "anyOrder")) {
            fixedOrder = false;
        } else if (ConsumeToken(ctx, "orGate")) {
            orGate = true;
        } else {
            parsingOptions = false;
        }
    }

    if (Macros_DryRun) {
        goto conditionPassed;
    }

    if (S->as.currentIfShortcutConditionPassed) {
        if (S->as.currentConditionPassed) {
            goto conditionPassed;
        } else {
            S->as.currentIfShortcutConditionPassed = false;
        }
    }

    //parse and check KEYIDs
    postponeCurrentCycle();
    uint8_t pendingCount = PostponerQuery_PendingKeypressCount();
    uint8_t numArgs = 0;
    bool someoneNotReleased = false;
    uint8_t argKeyId = 255;
    while((argKeyId = Macros_TryConsumeKeyId(ctx)) != 255 && ctx->at < ctx->end) {
        numArgs++;
        if (pendingCount < numArgs) {
            uint32_t referenceTime = transitive && pendingCount > 0 ? PostponerExtended_LastPressTime() : S->ms.currentMacroStartTime;
            uint16_t elapsedSinceReference = Timer_GetElapsedTime(&referenceTime);

            bool shortcutTimedOut = untilRelease && !Macros_CurrentMacroKeyIsActive() && (!transitive || !someoneNotReleased);
            bool gestureDefaultTimedOut = !untilRelease && cancelIn == 0 && timeoutIn == 0 && elapsedSinceReference > 1000;
            bool cancelInTimedOut = cancelIn != 0 && elapsedSinceReference > cancelIn;
            bool timeoutInTimedOut = timeoutIn != 0 && elapsedSinceReference > timeoutIn;
            if (!shortcutTimedOut && !gestureDefaultTimedOut && !cancelInTimedOut && !timeoutInTimedOut) {
                if (!untilRelease && cancelIn == 0 && timeoutIn == 0) {
                    Macros_SleepTillTime(referenceTime+1000);
                }
                if (cancelIn != 0) {
                    Macros_SleepTillTime(referenceTime+cancelIn);
                }
                if (timeoutIn != 0) {
                    Macros_SleepTillTime(referenceTime+timeoutIn);
                }
                Macros_SleepTillKeystateChange();
                return MacroResult_Sleeping;
            }
            else if (cancelInTimedOut) {
                PostponerExtended_ConsumePendingKeypresses(numArgs, true);
                S->ms.macroBroken = true;
                goto conditionFailed;
            }
            else {
                if (negate) {
                    goto conditionPassed;
                } else {
                    goto conditionFailed;
                }
            }
        }
        else if (orGate) {
            // go through all canidates all at once
            while (true) {
                // first keyid had already been processed.
                if (PostponerQuery_ContainsKeyId(argKeyId)) {
                    if (negate) {
                        goto conditionFailed;
                    } else {
                        goto conditionPassed;
                    }
                }
                if ((argKeyId = Macros_TryConsumeKeyId(ctx)) == 255 || ctx->at == ctx->end) {
                    break;
                }
            }
            // none is matched
            if (negate) {
                goto conditionPassed;
            } else {
                goto conditionFailed;
            }
        }
        else if (fixedOrder && PostponerExtended_PendingId(numArgs - 1) != argKeyId) {
            if (negate) {
                goto conditionPassed;
            } else {
                goto conditionFailed;
            }
        }
        else if (!fixedOrder && !PostponerQuery_ContainsKeyId(argKeyId)) {
            if (negate) {
                goto conditionPassed;
            } else {
                goto conditionFailed;
            }
        }
        else {
            someoneNotReleased |= !PostponerQuery_IsKeyReleased(Utils_KeyIdToKeyState(argKeyId));
        }
    }
    //all keys match
    if (consume) {
        PostponerExtended_ConsumePendingKeypresses(numArgs, true);
    }
    if (negate) {
        goto conditionFailed;
    } else {
        goto conditionPassed;
    }
conditionFailed:
    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
conditionPassed:
    while(Macros_TryConsumeKeyId(ctx) != 255) { };
    S->as.currentIfShortcutConditionPassed = true;
    S->as.currentConditionPassed = false; //otherwise following conditions would be skipped
    return processCommand(ctx);
}

uint8_t Macros_TryConsumeKeyId(parser_context_t* ctx)
{
    uint8_t keyId = MacroKeyIdParser_TryConsumeKeyId(ctx);

    if (keyId == 255 && isNUM(ctx)) {
        return Macros_LegacyConsumeInt(ctx);
    } else {
        return keyId;
    }
}

static macro_result_t processAutoRepeatCommand(parser_context_t* ctx) {
    if (Macros_DryRun) {
        return processCommand(ctx);;
    }

    switch (S->ms.autoRepeatPhase) {
    case AutoRepeatState_Waiting:
        goto process_delay;
    case AutoRepeatState_Executing:
    default:
        goto run_command;
    }

process_delay:;
    uint16_t delay = S->ms.autoRepeatInitialDelayPassed ? AutoRepeatDelayRate : AutoRepeatInitialDelay;
    bool pendingReleased = PostponerQuery_IsKeyReleased(S->ms.currentMacroKey);
    bool currentKeyIsActive = Macros_CurrentMacroKeyIsActive();

    if (!currentKeyIsActive || pendingReleased) {
        // reset delay state in case it was interrupted by key release
        memset(&S->as.delayData, 0, sizeof S->as.delayData);
        S->as.actionActive = 0;
        S->ms.autoRepeatPhase = AutoRepeatState_Executing;

        return MacroResult_Finished;
    }

    if (processDelay(delay) == MacroResult_Finished) {
        S->ms.autoRepeatInitialDelayPassed = true;
        S->ms.autoRepeatPhase = AutoRepeatState_Executing;
        goto run_command;
    } else {
        Macros_SleepTillKeystateChange();
        return MacroResult_Sleeping;
    }


run_command:;
    macro_result_t res = processCommand(ctx);
    if (res & MacroResult_ActionFinishedFlag) {
        S->ms.autoRepeatPhase = AutoRepeatState_Waiting;
        //tidy the state in case someone left it dirty
        memset(&S->as.delayData, 0, sizeof S->as.delayData);
        S->as.actionActive = false;
        S->as.actionPhase = 0;
        return (res & ~MacroResult_ActionFinishedFlag) | MacroResult_InProgressFlag;
    } else if (res & MacroResult_DoneFlag) {
        S->ms.autoRepeatPhase = AutoRepeatState_Waiting;
        return res;
    } else {
        return res;
    }
}


static macro_result_t processOneShotCommand(parser_context_t* ctx) {
    if (Macros_DryRun) {
        return processCommand(ctx);
    }

    /* In order for modifiers of a scancode action to be applied properly, we need to
     * make the action live at least one cycle after macroInterrupted becomes true.
     *
     * Also, we need to prevent the command to go sleeping after this because
     * we would not be woken up.
     * */
    if (!S->ms.macroInterrupted) {
        S->ms.oneShotState = 1;
    } else if (S->ms.oneShotState < 3) {
        S->ms.oneShotState++;
    } else {
        S->ms.oneShotState = 0;
    }
    macro_result_t res = processCommand(ctx);
    if (res & MacroResult_ActionFinishedFlag || res & MacroResult_DoneFlag) {
        S->ms.oneShotState = 0;
    }
    if (S->ms.oneShotState > 1) {
        return res | MacroResult_Blocking;
    } else {
        return res;
    }
}

static macro_result_t processOverlayKeymapCommand(parser_context_t* ctx)
{
    uint8_t srcKeymapId = consumeKeymapId(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    OverlayKeymap(srcKeymapId);
    return MacroResult_Finished;
}

static macro_result_t processReplaceLayerCommand(parser_context_t* ctx)
{
    layer_id_t dstLayerId = Macros_ConsumeLayerId(ctx);
    uint8_t srcKeymapId = consumeKeymapId(ctx);
    layer_id_t srcLayerId = Macros_ConsumeLayerId(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    ReplaceLayer(dstLayerId, srcKeymapId, srcLayerId);
    return MacroResult_Finished;
}

static macro_result_t processOverlayLayerCommand(parser_context_t* ctx)
{
    layer_id_t dstLayerId = Macros_ConsumeLayerId(ctx);
    uint8_t srcKeymapId = consumeKeymapId(ctx);
    layer_id_t srcLayerId = Macros_ConsumeLayerId(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    OverlayLayer(dstLayerId, srcKeymapId, srcLayerId);
    return MacroResult_Finished;
}

static bool processIfKeyPendingAtCommand(parser_context_t* ctx, bool negate)
{
    uint16_t idx = Macros_LegacyConsumeInt(ctx);
    uint16_t key = Macros_LegacyConsumeInt(ctx);

    if (Macros_DryRun) {
        return true;
    }

    return (PostponerExtended_PendingId(idx) == key) != negate;
}

static bool processIfKeyActiveCommand(parser_context_t* ctx, bool negate)
{
    uint16_t keyid = Macros_LegacyConsumeInt(ctx);
    key_state_t* key = Utils_KeyIdToKeyState(keyid);
    if (Macros_DryRun) {
        return true;
    }
    return KeyState_Active(key) != negate;
}

static bool processIfPendingKeyReleasedCommand(parser_context_t* ctx, bool negate)
{
    uint16_t idx = Macros_LegacyConsumeInt(ctx);
    if (Macros_DryRun) {
        return true;
    }
    return PostponerExtended_IsPendingKeyReleased(idx) != negate;
}

static bool processIfKeyDefinedCommand(parser_context_t* ctx, bool negate)
{
    uint16_t keyid = Macros_LegacyConsumeInt(ctx);
    if (Macros_DryRun) {
        return true;
    }
    uint8_t slot;
    uint8_t slotIdx;
    Utils_DecodeId(keyid, &slot, &slotIdx);
    key_action_t* action = &CurrentKeymap[ActiveLayer][slot][slotIdx];
    return (action->type != KeyActionType_None) != negate;
}

static macro_result_t processActivateKeyPostponedCommand(parser_context_t* ctx)
{
    uint8_t layer = 255;
    bool options = true;
    bool append = true;

    while (options) {
        options = false;
        if (ConsumeToken(ctx, "atLayer")) {
            layer = Macros_ConsumeLayerId(ctx);
            options = true;
        }
        if (ConsumeToken(ctx, "append")) {
            append = true;
            options = true;
        }
        if (ConsumeToken(ctx, "prepend")) {
            append = false;
            options = true;
        }
    }

    uint16_t keyid = Macros_LegacyConsumeInt(ctx);
    key_state_t* key = Utils_KeyIdToKeyState(keyid);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return true;
    }

    if (append) {
        if (PostponerQuery_IsActiveEventually(key)) {
            PostponerCore_TrackKeyEvent(key, false, layer);
            PostponerCore_TrackKeyEvent(key, true, layer);
        } else {
            PostponerCore_TrackKeyEvent(key, true, layer);
            PostponerCore_TrackKeyEvent(key, false, layer);
        }
    } else {
        if (KeyState_Active(key)) {
            //reverse order when prepending
            PostponerCore_PrependKeyEvent(key, true, layer);
            PostponerCore_PrependKeyEvent(key, false, layer);
        } else {
            PostponerCore_PrependKeyEvent(key, false, layer);
            PostponerCore_PrependKeyEvent(key, true, layer);
        }
    }
    return MacroResult_Finished;
}

static macro_result_t processConsumePendingCommand(parser_context_t* ctx)
{
    uint16_t cnt = Macros_LegacyConsumeInt(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    PostponerExtended_ConsumePendingKeypresses(cnt, true);
    return MacroResult_Finished;
}

static macro_result_t processPostponeNextNCommand(parser_context_t* ctx)
{
    uint16_t cnt = Macros_LegacyConsumeInt(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    PostponerCore_PostponeNCycles(MACRO_CYCLES_TO_POSTPONE);
    postponeNextN(cnt);
    return MacroResult_Finished;
}


static macro_result_t processRepeatForCommand(parser_context_t* ctx)
{
    if (isNUM(ctx)) {
        uint8_t idx = Macros_LegacyConsumeInt(ctx);
        bool regIsValid = validReg(idx, ctx->at);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        if (regIsValid) {
            if (regs[idx] > 0) {
                regs[idx]--;
                if (regs[idx] > 0) {
                    return goTo(ctx);
                }
            }
        }
    } else {
        macro_variable_t* v = Macros_ConsumeExistingWritableVariable(ctx);
        if (v != NULL) {
            if (v->asInt > 0) {
                v->asInt--;
                if (v->asInt > 0) {
                    return goTo(ctx);
                }
            }
        }
    }
    return MacroResult_Finished;
}

static macro_result_t processResetTrackpointCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    UhkModuleSlaveDriver_ResetTrackpoint();
    return MacroResult_Finished;
}

static uint8_t consumeMacroIndexByName(parser_context_t* ctx)
{
    const char* end = TokEnd(ctx->at, ctx->end);
    uint8_t macroIndex = FindMacroIndexByName(ctx->at, end, true);
    ctx->at = end;
    ConsumeWhite(ctx);
    return macroIndex;
}

static macro_result_t processExecCommand(parser_context_t* ctx)
{
    uint8_t macroIndex = consumeMacroIndexByName(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    return Macros_ExecMacro(macroIndex);
}

static macro_result_t processCallCommand(parser_context_t* ctx)
{
    uint8_t macroIndex = consumeMacroIndexByName(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    return Macros_CallMacro(macroIndex);
}

static macro_result_t processForkCommand(parser_context_t* ctx)
{
    uint8_t macroIndex = consumeMacroIndexByName(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    return Macros_ForkMacro(macroIndex);
}

static macro_result_t processFinalCommand(parser_context_t* ctx)
{
    macro_result_t res = processCommand(ctx);

    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    if (res & MacroResult_InProgressFlag) {
        return res;
    } else {
        S->ms.macroBroken = true;
        return MacroResult_Finished;
    }
}

static macro_result_t processProgressHueCommand()
{
#define C(I) (*cols[((phase + I + 3) % 3)])

    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    const uint8_t step = 10;
    static uint8_t phase = 0;

    uint8_t* cols[] = {
        &LedMap_ConstantRGB.red,
        &LedMap_ConstantRGB.green,
        &LedMap_ConstantRGB.blue
    };

    bool progressed = false;
    if (C(-1) > step) {
        C(-1) -= step;
        progressed = true;
    }
    if (C(0) < 255-step+1) {
        C(0) += step;
        progressed = true;
    }
    if (!progressed) {
        C(-1) = 0;
        C(0) = 255;
        phase++;
    }

    Ledmap_SetLedBacklightingMode(BacklightingMode_ConstantRGB);
    Ledmap_UpdateBacklightLeds();
    return MacroResult_Finished;
#undef C
}

static macro_result_t processValidateUserConfigCommand(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    Macros_ValidateAllMacros();
    return MacroResult_Finished;
}

static macro_result_t processCommand(parser_context_t* ctx)
{
    if (*ctx->at == '$') {
        ctx->at++;
    }

    const char* cmdTokEnd = TokEnd(ctx->at, ctx->end);
    if (cmdTokEnd > ctx->at && cmdTokEnd[-1] == ':') {
        //skip labels
        ctx->at = NextTok(ctx->at, ctx->end);
        if (ctx->at == ctx->end) {
            return MacroResult_Finished;
        }
    }
    while(ctx->at < ctx->end) {
        switch(*ctx->at) {
        case 'a':
            if (ConsumeToken(ctx, "addReg")) {
                return processRegAddCommand(ctx, false);
            }
            else if (ConsumeToken(ctx, "activateKeyPostponed")) {
                return processActivateKeyPostponedCommand(ctx);
            }
            else if (ConsumeToken(ctx, "autoRepeat")) {
                return processAutoRepeatCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        case 'b':
            if (ConsumeToken(ctx, "break")) {
                return processBreakCommand();
            }
            else {
                goto failed;
            }
            break;
        case 'c':
            if (ConsumeToken(ctx, "consumePending")) {
                return processConsumePendingCommand(ctx);
            }
            else if (ConsumeToken(ctx, "clearStatus")) {
                return Macros_ProcessClearStatusCommand();
            }
            else if (ConsumeToken(ctx, "call")) {
                return processCallCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        case 'd':
            if (ConsumeToken(ctx, "delayUntilRelease")) {
                return processDelayUntilReleaseCommand();
            }
            else if (ConsumeToken(ctx, "delayUntilReleaseMax")) {
                return processDelayUntilReleaseMaxCommand(ctx);
            }
            else if (ConsumeToken(ctx, "delayUntil")) {
                return processDelayUntilCommand(ctx);
            }
            else if (ConsumeToken(ctx, "diagnose")) {
                return Macros_ProcessDiagnoseCommand();
            }
            else {
                goto failed;
            }
            break;
        case 'e':
            if (ConsumeToken(ctx, "exec")) {
                return processExecCommand(ctx);
            }
            else if (ConsumeToken(ctx, "else")) {
                if (S->ms.lastIfSucceeded) {
                    return MacroResult_Finished;
                }
            }
            else {
                goto failed;
            }
            break;
        case 'f':
            if (ConsumeToken(ctx, "final")) {
                return processFinalCommand(ctx);
            }
            else if (ConsumeToken(ctx, "fork")) {
                return processForkCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        case 'g':
            if (ConsumeToken(ctx, "goTo")) {
                return processGoToCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        case 'h':
            if (ConsumeToken(ctx, "holdLayer")) {
                return processHoldLayerCommand(ctx);
            }
            else if (ConsumeToken(ctx, "holdLayerMax")) {
                return processHoldLayerMaxCommand(ctx);
            }
            else if (ConsumeToken(ctx, "holdKeymapLayer")) {
                return processHoldKeymapLayerCommand(ctx);
            }
            else if (ConsumeToken(ctx, "holdKeymapLayerMax")) {
                return processHoldKeymapLayerMaxCommand(ctx);
            }
            else if (ConsumeToken(ctx, "holdKey")) {
                return Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Hold);
            }
            else {
                goto failed;
            }
            break;
        case 'i':
            if (ConsumeToken(ctx, "if")) {
                if (!processIfCommand(ctx) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifDoubletap")) {
                if (!processIfDoubletapCommand(false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotDoubletap")) {
                if (!processIfDoubletapCommand(true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifInterrupted")) {
                if (!processIfInterruptedCommand(false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotInterrupted")) {
                if (!processIfInterruptedCommand(true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifReleased")) {
                if (!processIfReleasedCommand(false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotReleased")) {
                if (!processIfReleasedCommand(true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifRegEq")) {
                if (!processIfRegEqCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotRegEq")) {
                if (!processIfRegEqCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifRegGt")) {
                if (!processIfRegInequalityCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifRegLt")) {
                if (!processIfRegInequalityCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifKeymap")) {
                if (!processIfKeymapCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotKeymap")) {
                if (!processIfKeymapCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifLayer")) {
                if (!processIfLayerCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotLayer")) {
                if (!processIfLayerCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifPlaytime")) {
                if (!processIfPlaytimeCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotPlaytime")) {
                if (!processIfPlaytimeCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifAnyMod")) {
                if (!processIfModifierCommand(false, 0xFF)  && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotAnyMod")) {
                if (!processIfModifierCommand(true, 0xFF)  && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifShift")) {
                if (!processIfModifierCommand(false, SHIFTMASK)  && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotShift")) {
                if (!processIfModifierCommand(true, SHIFTMASK) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifCtrl")) {
                if (!processIfModifierCommand(false, CTRLMASK) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotCtrl")) {
                if (!processIfModifierCommand(true, CTRLMASK) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifAlt")) {
                if (!processIfModifierCommand(false, ALTMASK) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotAlt")) {
                if (!processIfModifierCommand(true, ALTMASK) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifGui")) {
                if (!processIfModifierCommand(false, GUIMASK)  && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotGui")) {
                if (!processIfModifierCommand(true, GUIMASK) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifCapsLockOn")) {
                if (!processIfStateKeyCommand(false, &UsbBasicKeyboard_CapsLockOn) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotCapsLockOn")) {
                if (!processIfStateKeyCommand(true, &UsbBasicKeyboard_CapsLockOn) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNumLockOn")) {
                if (!processIfStateKeyCommand(false, &UsbBasicKeyboard_NumLockOn) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotNumLockOn")) {
                if (!processIfStateKeyCommand(true, &UsbBasicKeyboard_NumLockOn) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifScrollLockOn")) {
                if (!processIfStateKeyCommand(false, &UsbBasicKeyboard_ScrollLockOn) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotScrollLockOn")) {
                if (!processIfStateKeyCommand(true, &UsbBasicKeyboard_ScrollLockOn) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifRecording")) {
                if (!processIfRecordingCommand(false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotRecording")) {
                if (!processIfRecordingCommand(true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifRecordingId")) {
                if (!processIfRecordingIdCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotRecordingId")) {
                if (!processIfRecordingIdCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotPending")) {
                if (!processIfPendingCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifPending")) {
                if (!processIfPendingCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifKeyPendingAt")) {
                if (!processIfKeyPendingAtCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotKeyPendingAt")) {
                if (!processIfKeyPendingAtCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifKeyActive")) {
                if (!processIfKeyActiveCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotKeyActive")) {
                if (!processIfKeyActiveCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifPendingKeyReleased")) {
                if (!processIfPendingKeyReleasedCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotPendingKeyReleased")) {
                if (!processIfPendingKeyReleasedCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifKeyDefined")) {
                if (!processIfKeyDefinedCommand(ctx, false) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifNotKeyDefined")) {
                if (!processIfKeyDefinedCommand(ctx, true) && !S->as.currentConditionPassed) {
                    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
                }
            }
            else if (ConsumeToken(ctx, "ifSecondary")) {
                return processIfSecondaryCommand(ctx, false);
            }
            else if (ConsumeToken(ctx, "ifPrimary")) {
                return processIfSecondaryCommand(ctx, true);
            }
            else if (ConsumeToken(ctx, "ifShortcut")) {
                return processIfShortcutCommand(ctx, false, true);
            }
            else if (ConsumeToken(ctx, "ifNotShortcut")) {
                return processIfShortcutCommand(ctx, true, true);
            }
            else if (ConsumeToken(ctx, "ifGesture")) {
                return processIfShortcutCommand(ctx, false, false);
            }
            else if (ConsumeToken(ctx, "ifNotGesture")) {
                return processIfShortcutCommand(ctx, true, false);
            }
            else {
                goto failed;
            }
            break;
        case 'm':
            if (ConsumeToken(ctx, "mulReg")) {
                return processRegMulCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        case 'n':
            if (ConsumeToken(ctx, "noOp")) {
                return processNoOpCommand();
            }
            else {
                goto failed;
            }
            break;
        case 'o':
            if (ConsumeToken(ctx, "oneShot")) {
                return processOneShotCommand(ctx);
            }
            if (ConsumeToken(ctx, "overlayLayer")) {
                return processOverlayLayerCommand(ctx);
            }
            if (ConsumeToken(ctx, "overlayKeymap")) {
                return processOverlayKeymapCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        case 'p':
            if (ConsumeToken(ctx, "printStatus")) {
                return Macros_ProcessPrintStatusCommand();
            }
            else if (ConsumeToken(ctx, "playMacro")) {
                return processPlayMacroCommand(ctx);
            }
            else if (ConsumeToken(ctx, "pressKey")) {
                return Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Press);
            }
            else if (ConsumeToken(ctx, "postponeKeys")) {
                processPostponeKeysCommand();
            }
            else if (ConsumeToken(ctx, "postponeNext")) {
                return processPostponeNextNCommand(ctx);
            }
            else if (ConsumeToken(ctx, "progressHue")) {
                return processProgressHueCommand();
            }
            else {
                goto failed;
            }
            break;
        case 'r':
            if (ConsumeToken(ctx, "recordMacro")) {
                return processRecordMacroCommand(ctx, false);
            }
            else if (ConsumeToken(ctx, "recordMacroBlind")) {
                return processRecordMacroCommand(ctx, true);
            }
            else if (ConsumeToken(ctx, "recordMacroDelay")) {
                return processRecordMacroDelayCommand();
            }
            else if (ConsumeToken(ctx, "resolveSecondary")) {
                return processResolveSecondaryCommand(ctx);
            }
            else if (ConsumeToken(ctx, "resolveNextKeyId")) {
                return processResolveNextKeyIdCommand();
            }
            else if (ConsumeToken(ctx, "resolveNextKeyEq")) {
                return processResolveNextKeyEqCommand(ctx);
            }
            else if (ConsumeToken(ctx, "releaseKey")) {
                return Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Release);
            }
            else if (ConsumeToken(ctx, "repeatFor")) {
                return processRepeatForCommand(ctx);
            }
            else if (ConsumeToken(ctx, "resetTrackpoint")) {
                return processResetTrackpointCommand();
            }
            else if (ConsumeToken(ctx, "replaceLayer")) {
                return processReplaceLayerCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        case 's':
            if (ConsumeToken(ctx, "setStatusPart")) {
                return Macros_ProcessSetStatusCommand(ctx, false);
            }
            else if (ConsumeToken(ctx, "set")) {
                return Macro_ProcessSetCommand(ctx);
            }
            else if (ConsumeToken(ctx, "setVar")) {
                return Macros_ProcessSetVarCommand(ctx);
            }
            else if (ConsumeToken(ctx, "setStatus")) {
                return Macros_ProcessSetStatusCommand(ctx, true);
            }
            else if (ConsumeToken(ctx, "startRecording")) {
                return processStartRecordingCommand(ctx, false);
            }
            else if (ConsumeToken(ctx, "startRecordingBlind")) {
                return processStartRecordingCommand(ctx, true);
            }
            else if (ConsumeToken(ctx, "setLedTxt")) {
                return processSetLedTxtCommand(ctx);
            }
            else if (ConsumeToken(ctx, "setReg")) {
                return processSetRegCommand(ctx);
            }
            else if (ConsumeToken(ctx, "statsRuntime")) {
                return Macros_ProcessStatsRuntimeCommand();
            }
            else if (ConsumeToken(ctx, "statsRecordKeyTiming")) {
                return Macros_ProcessStatsRecordKeyTimingCommand();
            }
            else if (ConsumeToken(ctx, "statsLayerStack")) {
                return Macros_ProcessStatsLayerStackCommand();
            }
            else if (ConsumeToken(ctx, "statsActiveKeys")) {
                return Macros_ProcessStatsActiveKeysCommand();
            }
            else if (ConsumeToken(ctx, "statsActiveMacros")) {
                return Macros_ProcessStatsActiveMacrosCommand();
            }
            else if (ConsumeToken(ctx, "statsPostponerStack")) {
                return Macros_ProcessStatsPostponerStackCommand();
            }
            else if (ConsumeToken(ctx, "subReg")) {
                return processRegAddCommand(ctx, true);
            }
            else if (ConsumeToken(ctx, "switchKeymap")) {
                return processSwitchKeymapCommand(ctx);
            }
            else if (ConsumeToken(ctx, "switchKeymapLayer")) {
                return processSwitchKeymapLayerCommand(ctx);
            }
            else if (ConsumeToken(ctx, "switchLayer")) {
                return processSwitchLayerCommand(ctx);
            }
            else if (ConsumeToken(ctx, "startMouse")) {
                return processMouseCommand(ctx, true);
            }
            else if (ConsumeToken(ctx, "stopMouse")) {
                return processMouseCommand(ctx, false);
            }
            else if (ConsumeToken(ctx, "stopRecording")) {
                return processStopRecordingCommand();
            }
            else if (ConsumeToken(ctx, "stopAllMacros")) {
                return processStopAllMacrosCommand();
            }
            else if (ConsumeToken(ctx, "stopRecordingBlind")) {
                return processStopRecordingCommand();
            }
            else if (ConsumeToken(ctx, "suppressMods")) {
                processSuppressModsCommand();
            }
            else {
                goto failed;
            }
            break;
        case 't':
            if (ConsumeToken(ctx, "toggleKeymapLayer")) {
                return processToggleKeymapLayerCommand(ctx);
            }
            else if (ConsumeToken(ctx, "toggleLayer")) {
                return processToggleLayerCommand(ctx);
            }
            else if (ConsumeToken(ctx, "tapKey")) {
                return Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Tap);
            }
            else if (ConsumeToken(ctx, "tapKeySeq")) {
                return Macros_ProcessTapKeySeqCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        case 'u':
            if (ConsumeToken(ctx, "unToggleLayer")) {
                return processUnToggleLayerCommand();
            }
            else if (ConsumeToken(ctx, "untoggleLayer")) {
                return processUnToggleLayerCommand();
            }
            else {
                goto failed;
            }
            break;
        case 'v':
            if (ConsumeToken(ctx, "validateUserConfig")) {
                return processValidateUserConfigCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        case 'w':
            if (ConsumeToken(ctx, "write")) {
                return processWriteCommand(ctx);
            }
            else if (ConsumeToken(ctx, "writeExpr")) {
                return processWriteExprCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        case 'y':
            if (ConsumeToken(ctx, "yield")) {
                return processYieldCommand(ctx);
            }
            else {
                goto failed;
            }
            break;
        default:
        failed:
            Macros_ReportError("Unrecognized command:", ctx->at, ctx->end);
            return MacroResult_Finished;
            break;
        }
    }
    //this is reachable if there is a train of conditions/modifiers/labels without any command
    return MacroResult_Finished;
}

macro_result_t Macros_ProcessCommandAction(void)
{
    const char* cmd = S->ms.currentMacroAction.cmd.text + S->ms.commandBegin;
    const char* cmdEnd = S->ms.currentMacroAction.cmd.text + S->ms.commandEnd;


    parser_context_t ctx = { .macroState = S, .begin = cmd, .at = cmd, .end = cmdEnd };

    ConsumeWhite(&ctx);

    if (ctx.at[0] == '#') {
        Macros_ReportWarn("# comments are deprecated, please switch to //", ctx.at, ctx.at);
        return MacroResult_Finished;
    }
    if (ctx.at[0] == '/' && ctx.at[1] == '/') {
        return MacroResult_Finished;
    }

    macro_result_t macroResult;

    macroResult = processCommand(&ctx);

    if (*ctx.at == '#') {
        Macros_ReportWarn("# comments are deprecated, please switch to //", ctx.at, ctx.at);
    } else if (ctx.at != ctx.end && !Macros_ParserError) {
        Macros_ReportWarn("Unprocessed input encountered.", ctx.at, ctx.at);
    }

    S->as.currentConditionPassed = macroResult & MacroResult_InProgressFlag;
    if (macroResult & (MacroResult_ActionFinishedFlag | MacroResult_DoneFlag)) {
        S->ms.lastIfSucceeded = !(macroResult & MacroResult_ConditionFailedFlag);
    }
    return macroResult;
}
