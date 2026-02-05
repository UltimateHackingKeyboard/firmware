#include <string.h>
#include "attributes.h"
#include "command_ids.h"
#include "config_parser/config_globals.h"
#include "config_parser/parse_macro.h"
#include "host_connection.h"
#include "keymap.h"
#include "layer.h"
#include "layer_stack.h"
#include "layer_switcher.h"
#include "ledmap.h"
#include "logger.h"
#include "macro_recorder.h"
#include "macros/commands.h"
#include "macros/debug_commands.h"
#include "macros/core.h"
#include "macros/keyid_parser.h"
#include "macros/scancode_commands.h"
#include "macros/set_command.h"
#include "macros/shortcut_parser.h"
#include "macros/status_buffer.h"
#include "macros/string_reader.h"
#include "macros/typedefs.h"
#include "macros/display.h"
#include "macros/vars.h"
#include "postponer.h"
#include "secondary_role_driver.h"
#include "power_mode.h"
#include "usb_composite_device.h"
#include "host_connection.h"
#include "slave_drivers/uhk_module_driver.h"
#include "str_utils.h"
#include "timer.h"
#include "usb_report_updater.h"
#include "utils.h"
#include "debug.h"
#include "config_manager.h"
#include "usb_commands/usb_command_reenumerate.h"
#include "bt_defs.h"
#include "trace.h"
#include "peripherals/leakage_test.h"
#include "oneshot.h"
#include "test_suite/test_suite.h"

#ifdef __ZEPHYR__
#include "connections.h"
#include "bt_pair.h"
#include "shell.h"
#include "host_connection.h"
#else
#include "segment_display.h"
#endif

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// Include the gperf-generated hash table
#include "command_hash.c"

static uint8_t lastLayerIdx;
static uint8_t lastLayerKeymapIdx;
static uint8_t lastKeymapIdx;


static void jumpToMatchingBrace();
static macro_result_t processCommand(parser_context_t* ctx);
static void consumeAnyJumpTarget(parser_context_t* ctx);

static macro_result_t processDelay(uint32_t time)
{
    if (S->as.actionActive) {
        if (Timer_GetElapsedTime(&S->as.delayData.start) >= time) {
            S->as.actionActive = false;
            S->as.delayData.start = 0;
            return MacroResult_Finished;
        }
        Macros_SleepTillTime(S->as.delayData.start + time, "Macros - delay");
        return MacroResult_Sleeping;
    } else {
        S->as.delayData.start = Timer_GetCurrentTime();
        S->as.actionActive = true;
        return processDelay(time);
    }
}

macro_result_t Macros_ProcessDelay(uint32_t time)
{
    return processDelay(time);
}

macro_result_t Macros_ProcessDelayAction()
{
    return processDelay(S->ms.currentMacroAction.delay.delay);
}

static void postponeNextN(uint8_t count)
{
    S->ms.postponeNextNCommands = count + 1;
    S->ls->as.modifierPostpone = true;
    //PostponerCore_PostponeNCycles(MACRO_CYCLES_TO_POSTPONE);
}

static void postponeCurrentCycle()
{
    //PostponerCore_PostponeNCycles(MACRO_CYCLES_TO_POSTPONE);
    S->ls->as.modifierPostpone = true;
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
        return S->ms.oneShot == 1;
    }
    if (S->ms.postponeNextNCommands > 0 || S->ls->as.modifierPostpone) {
        bool isSameActivation = (S->ms.currentMacroKey->activationTimestamp == S->ms.currentMacroKeyStamp);
        bool keyIsActive = (KeyState_Active(S->ms.currentMacroKey) && !PostponerQuery_IsKeyReleased(S->ms.currentMacroKey));
        return  (isSameActivation && keyIsActive) || S->ms.oneShot == 1;
    } else {
        bool isSameActivation = (S->ms.currentMacroKey->activationTimestamp == S->ms.currentMacroKeyStamp);
        bool keyIsActive = KeyState_Active(S->ms.currentMacroKey);
        return (isSameActivation && keyIsActive) || S->ms.oneShot == 1;
    }
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

    macro_result_t res = Macros_DispatchText(&num[10-len], len, NULL);
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

bool Macros_IsNUM(parser_context_t* ctx) {
    return isNUM(ctx);
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
        lastMacroId = 128 + Macros_ConsumeInt(ctx);
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
    }
    if (ConsumeToken(ctx, "current")) {
        return CurrentKeymapIndex;
    }
    else {
        uint8_t len = TokLen(ctx->at, ctx->end);
        uint8_t idx = FindKeymapByAbbreviation(len, ctx->at);
        if (idx == 0xFF) {
            Macros_ReportErrorPos(ctx, "Keymap not recognized:");
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

    Macros_ReportErrorTok(ctx, "Unrecognized layer:");
    return LayerId_Base;
}


static uint8_t consumeLayerKeymapId(parser_context_t* ctx)
{
    // Assume: `toggleLayer last`
    // Now since we allow toggling layers of other keymaps, we need to figure out the keymap.
    // This function does that from the layer id, which is parsed twice.
    // This is the first run and doesn't consume the token.
    CTX_COPY(bakCtx, *ctx);
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

        SwitchKeymapById(newKeymapIdx, true);
    }
    lastKeymapIdx = tmpKeymapIdx;
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
    uint8_t keymap = consumeLayerKeymapId(ctx);
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
                Macros_SleepTillTime(S->ms.currentMacroStartTime + timeout, "Macros - holdLayer timeout");
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
    uint8_t keymap = consumeLayerKeymapId(ctx);

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
    uint8_t keymap = consumeLayerKeymapId(ctx);
    uint8_t layer = Macros_ConsumeLayerId(ctx);
    uint16_t timeout = Macros_ConsumeInt(ctx);

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
    uint16_t timeout = Macros_ConsumeInt(ctx);

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
    uint32_t timeout = Macros_ConsumeInt(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    if (Macros_CurrentMacroKeyIsActive() && Timer_GetElapsedTime(&S->ms.currentMacroStartTime) < timeout) {
        Macros_SleepTillKeystateChange();
        Macros_SleepTillTime(S->ms.currentMacroStartTime + timeout, "Macros - delayUntilRelease timeout");
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
    uint32_t time = Macros_ConsumeInt(ctx);

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

    for (uint8_t i = 0; i < MACRO_HISTORY_POOL_SIZE; i++) {
        if (S->ms.currentMacroStartTime - MacroHistory[i].macroStartTime <= Cfg.DoubletapTimeout && S->ms.currentMacroIndex == MacroHistory[i].macroIndex) {
            doubletapFound = true;
        }
    }

    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (
            MacroState[i].ms.macroPlaying &&
            MacroState[i].ms.currentMacroStartTime < S->ms.currentMacroStartTime &&
            S->ms.currentMacroStartTime - MacroState[i].ms.currentMacroStartTime <= Cfg.DoubletapTimeout &&
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
    uint32_t cnt = Macros_ConsumeInt(ctx);
    if (Macros_DryRun) {
        return true;
    }

    return (PostponerQuery_PendingKeypressCount() >= cnt) != negate;
}

static bool processIfPlaytimeCommand(parser_context_t* ctx, bool negate)
{
    uint32_t timeout = Macros_ConsumeInt(ctx);
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
    return (queryLayerIdx == ActiveLayer) != negate;
}

static bool processIfLayerToggledCommand(parser_context_t* ctx, bool negate)
{
    if (Macros_DryRun) {
        return true;
    }
    return (LayerStack_IsLayerToggled()) != negate;
}

static bool processIfCommand(parser_context_t* ctx)
{
    bool res = Macros_ConsumeBool(ctx);

    if (Macros_DryRun) {
        return true;
    }

    return res;
}

static macro_result_t processWhileCommand(parser_context_t* ctx)
{
    bool condition = Macros_ConsumeBool(ctx);

    if (Macros_DryRun) {
        return processCommand(ctx);
    }

    if (!S->ls->as.whileExecuting) {
        if (!condition) {
            return MacroResult_Finished;
        }

        S->ls->as.isWhileScope = true;
        S->ls->as.whileExecuting = true;
        S->ls->as.currentConditionPassed = false;
    }

    macro_result_t res = processCommand(ctx);

    if (res & (MacroResult_ActionFinishedFlag | MacroResult_DoneFlag)) {
        S->ls->as.whileExecuting = false;

        if (res & MacroResult_ActionFinishedFlag) {
            return (res & ~MacroResult_ActionFinishedFlag) | MacroResult_InProgressFlag | MacroResult_YieldFlag;
        } else {
            return res;
        }
    } else {
        return res;
    }
}

static macro_result_t processExitCommand(parser_context_t *ctx)
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    S->ms.macroBroken = true;
    return MacroResult_Finished;
}

static macro_result_t processBreakCommand(parser_context_t *ctx)
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    // Take care of:
    // while (true) break
    if (S->ls->as.isWhileScope) {
        Macros_LoadNextCommand() || Macros_LoadNextAction() || (S->ms.macroBroken = true);
        return MacroResult_DoneFlag;
    }


    // Take care of:
    // while (true) {
    //   break
    // }
    // pop scopes until you reach parent while
    bool popped = false;
    while (S->ls->parentScopeIndex != 255 && !S->ls->as.isWhileScope) {
        popped = true;
        Macros_PopScope(ctx);
    }

    // now skip the scope, if we indeed reached parent while scope and not root scope
    if (popped && S->ls->as.isWhileScope) {
        jumpToMatchingBrace();
        return MacroResult_DoneFlag;
    }

    // Takes care of break in root scope
    S->ms.macroBroken = true;
    return MacroResult_Finished;
}

static macro_result_t processBluetoothCommand(parser_context_t *ctx)
{
    ATTR_UNUSED bool toggle = false;
    ATTR_UNUSED pairing_mode_t mode = PairingMode_Off;
    if (ConsumeToken(ctx, "toggle")) {
        toggle = true;
    }

    if (ConsumeToken(ctx, "pair")) {
        mode = PairingMode_PairHid;
    } else if (ConsumeToken(ctx, "advertise")) {
        mode = PairingMode_Advertise;
    } else if (ConsumeToken(ctx, "noadvertise") || ConsumeToken(ctx, "noAdvertise")) {
        mode = PairingMode_Off;
    } else {
        Macros_ReportErrorTok(ctx, "Unrecognized argument:");
        return MacroResult_Finished;
    }

    if (Macros_ParserError || Macros_DryRun) {
        return MacroResult_Finished;
    }

    if (mode == PairingMode_Off) {
        Cfg.Bt_AlwaysAdvertiseHid = false;
    }
#ifdef __ZEPHYR__
    BtManager_EnterMode(mode, toggle);
#endif

    return MacroResult_Finished;
}

macro_result_t goTo(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        consumeAnyJumpTarget(ctx);
        return MacroResult_Finished;
    }

    if (isNUM(ctx)) {
        return Macros_GoToAddress(Macros_ConsumeInt(ctx));
    } else {
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

static macro_result_t processOpeningBraceCommand(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        return MacroResult_Finished | MacroResult_OpeningBraceFlag;
    }

    if (!S->ls->as.braceExecuting) {
        // When opening a fresh scope, just insolently push scope and load next command.
        S->ls->as.braceExecuting = true;
        Macros_PushScope(ctx);
        S->ls->as.braceExecuting = false;

        Macros_LoadNextCommand() || Macros_LoadNextAction() || (S->ms.macroBroken = true);
        return MacroResult_DoneFlag;
    } else {
        // When closing brace is reached, scope is popped and the command that
        // opened the scope is ran again (still as if in progress). (This is
        // where we are now.)
        //
        // Now we return MacroResult_Finished. This MacroResult_Finished can be
        // intercepted by all modifiers that prefix the scope and e.g., choose
        // to rerun the command (as a while or autoRepeat do).

        S->ls->as.braceExecuting = false;
        while (*ctx->at != '{') {
            ctx->at--;
        }
        return MacroResult_Finished;
    }
}

static macro_result_t processClosingBraceCommand(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        return MacroResult_Finished | MacroResult_ClosingBraceFlag;
    }

    Macros_PopScope(ctx);

    // After this, the command that opened the scope will be rerun, and opening
    // brace will take care of scope epilogue.

    return MacroResult_InProgressFlag | MacroResult_ClosingBraceFlag;
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
        Macros_ReportErrorTok(ctx, "Unrecognized argument:");
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
            Macros_ReportErrorTok(ctx, "Unrecognized argument:");
            return MacroResult_Finished;
        }
    }

    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    if (baseAction != SerializedMouseAction_LeftClick) {
        MouseKeys_SetState(baseAction + dirOffset, true, enable);
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
    bool res = MacroRecorder_PlayRuntimeMacroSmart(id, &S->ms.reports.macroBasicKeyboardReport);
    return res ? MacroResult_Blocking : MacroResult_Finished;
}

static macro_result_t processMacroArgCommand(parser_context_t* ctx)
{
    uint16_t stringOffset = 0;
    uint16_t textIndex = 0;
    uint16_t textSubIndex = 0;

    if (S->ms.macroHeadersProcessed) {
        Macros_ReportErrorPos(ctx, "macroArg commands must be placed before any other commands in the macro");
        return MacroResult_Finished;
    }

    if (Macros_DryRun) {
        // parse macroArg command but ignore it for now

        while (Macros_ConsumeCharOfString(ctx, &stringOffset, &textIndex, &textSubIndex) != '\0') {};

        return MacroResult_Finished;
    }
    // parse the argument name (identifier)
    const char *idStart = ctx->at;
    const char *idEnd = IdentifierEnd(ctx);

    if (idEnd == idStart) {
        Macros_ReportErrorPos(ctx, "Expected identifier");
        return MacroResult_Finished;
    }
    ctx->at = idEnd;

    // see if the argument has a type
    macro_arg_type_t argType;

    if (*ctx->at == ':') {
        ctx->at++;
        const char *typeStart = ctx->at;

        if (ConsumeToken(ctx, "int")) {
            argType = MacroArgType_Int;
        }
        else if (ConsumeToken(ctx, "float")) {
            argType = MacroArgType_Float;
        }
        else if (ConsumeToken(ctx, "string")) {
            argType = MacroArgType_String;
        }
        else if (ConsumeToken(ctx, "keyid")) {
            argType = MacroArgType_KeyId;
        }
        else if (ConsumeToken(ctx, "scancode")) {
            argType = MacroArgType_ScanCode;
        }
        else if (ConsumeToken(ctx, "any")) {
            argType = MacroArgType_Any;
        }
        else {
            Macros_ReportErrorTok(ctx, "Unrecognized macroArg argument type:");
            return MacroResult_Finished;
        }
    }
    else {
        argType = MacroArgType_Any;
        ConsumeWhite(ctx);
    }

    // rest of command is descriptive label, ignored by firmware
    while (Macros_ConsumeCharOfString(ctx, &stringOffset, &textIndex, &textSubIndex) != '\0') {};

    return MacroResult_Finished;

//    Macros_ReportErrorPrintf(ctx->at, "Parsing failed at '%s'?", OneWord(ctx));
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

    macro_result_t res = Macros_DispatchText(ctx->at, ctx->end - ctx->at, ctx);

    if (res & MacroResult_ActionFinishedFlag) {
        ctx->at = ctx->end;
    }

    return res;
}

static void processSuppressModsCommand()
{
    if (Macros_DryRun) {
        return;
    }
    SuppressMods = true;
    S->ls->as.modifierSuppressMods = true;
}

static void processPostponeKeysCommand()
{
    if (Macros_DryRun) {
        return;
    }
    postponeCurrentCycle();
    S->ls->as.modifierPostpone = true;
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

static macro_result_t processIfSecondaryCommand(parser_context_t* ctx, bool negate)
{
    secondary_role_strategy_t strategy = Cfg.SecondaryRoles_Strategy;
    bool originalPostponing = S->ls->as.modifierPostpone;
    secondary_role_same_half_t fromSameHalf = SecondaryRole_DefaultFromSameHalf;

    while(true) {
        if (ConsumeToken(ctx, "simpleStrategy")) {
            strategy = SecondaryRoleStrategy_Simple;
        }
        else if (ConsumeToken(ctx, "advancedStrategy")) {
            strategy = SecondaryRoleStrategy_Advanced;
        }

        if (ConsumeToken(ctx, "ignoreTriggersFromSameHalf")) {
            fromSameHalf = SecondaryRole_IgnoreTriggersFromSameHalf;
        }
        else if (ConsumeToken(ctx, "acceptTriggersFromSameHalf")) {
            fromSameHalf = SecondaryRole_AcceptTriggersFromSameHalf;
        }
        else {
            break;
        }
    }

    if (Macros_DryRun) {
        goto conditionPassed;
    }

    if (S->ls->as.currentIfSecondaryConditionPassed) {
        if (S->ls->as.currentConditionPassed) {
            goto conditionPassed;
        } else {
            S->ls->as.currentIfSecondaryConditionPassed = false;
        }
    }

    postponeCurrentCycle();
    secondary_role_state_t res = SecondaryRoles_ResolveState(S->ms.currentMacroKey, strategy, true, fromSameHalf);

    S->as.actionActive = res == SecondaryRoleState_DontKnowYet;
    switch(res) {
    case SecondaryRoleState_DontKnowYet:
        // secondary role driver has its own scheduler hook to wake us up
        return MacroResult_Sleeping;
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
    case SecondaryRoleState_NoOp:
        return MacroResult_Finished | MacroResult_ConditionFailedFlag;
    }

conditionPassed:
    S->ls->as.currentIfSecondaryConditionPassed = true;
    S->ls->as.currentConditionPassed = false; //otherwise following conditions would be skipped
    S->ls->as.modifierPostpone = originalPostponing;
    return processCommand(ctx);
}


static macro_result_t processResolveNextKeyIdCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    postponeCurrentCycle();
    if (PostponerQuery_PendingKeypressCount() == 0) {
        return Macros_SleepTillKeystateChange();
    }
    macro_result_t res = writeNum(PostponerExtended_PendingId(0));
    if (res == MacroResult_Finished) {
        PostponerExtended_ConsumePendingKeypresses(1, true);
        return MacroResult_Finished;
    }
    return res;
}

static macro_result_t processIfHoldCommand(parser_context_t* ctx, bool negate)
{
    if (Macros_DryRun) {
        goto conditionPassed;
    }

    if (S->ls->as.currentIfHoldConditionPassed) {
        if (S->ls->as.currentConditionPassed) {
            goto conditionPassed;
        } else {
            S->ls->as.currentIfHoldConditionPassed = false;
        }
    }

    postponer_buffer_record_type_t *dummy;
    postponer_buffer_record_type_t *keyReleased;
    PostponerQuery_InfoByKeystate(S->ms.currentMacroKey, &dummy, &keyReleased);

    if (keyReleased != NULL) {
        bool releasedAfterTimeout = keyReleased->time - S->ms.currentMacroStartTime >= Cfg.HoldTimeout;
        if (releasedAfterTimeout != negate) {
            goto conditionPassed;
        } else {
            return MacroResult_Finished | MacroResult_ConditionFailedFlag;
        }
    }

    if (Timer_GetCurrentTime() - S->ms.currentMacroStartTime >= Cfg.HoldTimeout) {
        if (negate) {
            return MacroResult_Finished | MacroResult_ConditionFailedFlag;
        } else {
            goto conditionPassed;
        }
    }

    postponeCurrentCycle();
    Macros_SleepTillKeystateChange();
    return Macros_SleepTillTime(S->ms.currentMacroStartTime + Cfg.HoldTimeout, "ifHold timeout");

conditionPassed:
    S->ls->as.currentIfHoldConditionPassed = true;
    S->ls->as.currentConditionPassed = false; //otherwise following conditions would be skipped
    return processCommand(ctx);
}

static macro_result_t processIfShortcutCommand(parser_context_t* ctx, bool negate, bool untilRelease)
{
    bool originalPostponing = S->ls->as.modifierPostpone;

    //parse optional flags
    bool consume = true;
    bool transitive = false;
    bool fixedOrder = true;
    bool orGate = false;
    uint16_t cancelIn = 0;
    uint16_t timeoutIn= 0;
    uint16_t defaultTimeoutIn = 500;

    bool parsingOptions = true;
    while(parsingOptions) {
        if (ConsumeToken(ctx, "noConsume")) {
            consume = false;
        } else if (ConsumeToken(ctx, "transitive")) {
            transitive = true;
        } else if (ConsumeToken(ctx, "timeoutIn")) {
            timeoutIn = Macros_ConsumeInt(ctx);
        } else if (ConsumeToken(ctx, "cancelIn")) {
            cancelIn = Macros_ConsumeInt(ctx);
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

    if (S->ls->as.currentIfShortcutConditionPassed) {
        if (S->ls->as.currentConditionPassed) {
            goto conditionPassed;
        } else {
            S->ls->as.currentIfShortcutConditionPassed = false;
        }
    }

    uint8_t pendingCount = PostponerQuery_PendingKeypressCount();

    bool insufficientNumberForAnyOrder = false;
    if (!fixedOrder) {
        CTX_COPY(ctx2, *ctx);
        uint8_t totalArgs = 0;
        uint8_t argKeyId = 255;
        while((argKeyId = Macros_TryConsumeKeyId(&ctx2)) != 255 && ctx2.at < ctx2.end) {
            totalArgs++;
        }
        if (totalArgs > PostponerQuery_PendingKeypressCount()) {
            insufficientNumberForAnyOrder = true;
        }
    }

    //parse and check KEYIDs
    postponeCurrentCycle();
    uint8_t numArgs = 0;
    bool someoneNotReleased = false;
    uint8_t argKeyId = 255;
    while((argKeyId = Macros_TryConsumeKeyId(ctx)) != 255 && ctx->at < ctx->end) {
        numArgs++;
        if (pendingCount < numArgs || insufficientNumberForAnyOrder) {
            uint32_t referenceTime = transitive && pendingCount > 0 ? PostponerExtended_LastPressTime() : S->ms.currentMacroStartTime;
            uint16_t elapsedSinceReference = Timer_GetElapsedTime(&referenceTime);

            bool shortcutTimedOut = untilRelease && !Macros_CurrentMacroKeyIsActive() && (!transitive || !someoneNotReleased);
            bool gestureDefaultTimedOut = !untilRelease && cancelIn == 0 && timeoutIn == 0 && elapsedSinceReference > defaultTimeoutIn;
            bool cancelInTimedOut = cancelIn != 0 && elapsedSinceReference > cancelIn;
            bool timeoutInTimedOut = timeoutIn != 0 && elapsedSinceReference > timeoutIn;
            if (!shortcutTimedOut && !gestureDefaultTimedOut && !cancelInTimedOut && !timeoutInTimedOut) {
                if (!untilRelease && cancelIn == 0 && timeoutIn == 0) {
                    Macros_SleepTillTime(referenceTime+defaultTimeoutIn, "Macros - shortcut timeout");
                }
                if (cancelIn != 0) {
                    Macros_SleepTillTime(referenceTime+cancelIn, "Macros - shortcut cancelIn");
                }
                if (timeoutIn != 0) {
                    Macros_SleepTillTime(referenceTime+timeoutIn, "Macros - shortcut timeoutIn");
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
                goto notMatched;
            }
        }
        else if (orGate) {
            // go through all canidates all at once
            while (true) {
                // first keyid had already been processed.
                if (PostponerQuery_ContainsKeyId(argKeyId)) {
                    numArgs = 1;
                    goto matched;
                }
                if ((argKeyId = Macros_TryConsumeKeyId(ctx)) == 255 || ctx->at == ctx->end) {
                    break;
                }
            }
            goto notMatched;
        }
        else if (fixedOrder && PostponerExtended_PendingId(numArgs - 1) != argKeyId) {
            goto notMatched;
        }
        else if (!fixedOrder && !PostponerQuery_ContainsKeyId(argKeyId)) {
            goto notMatched;
        }
        else {
            someoneNotReleased |= !PostponerQuery_IsKeyReleased(Utils_KeyIdToKeyState(argKeyId));
        }
    }
matched:
    //all keys match
    if (consume) {
        PostponerExtended_ConsumePendingKeypresses(numArgs, true);
    }
    if (negate) {
        goto conditionFailed;
    } else {
        goto conditionPassed;
    }
notMatched:
    if (negate) {
        goto conditionPassed;
    } else {
        goto conditionFailed;
    }
conditionFailed:
    while(Macros_TryConsumeKeyId(ctx) != 255) { };
    return MacroResult_Finished | MacroResult_ConditionFailedFlag;
conditionPassed:
    while(Macros_TryConsumeKeyId(ctx) != 255) { };
    S->ls->as.currentIfShortcutConditionPassed = true;
    S->ls->as.currentConditionPassed = false; //otherwise following conditions would be skipped
    S->ls->as.modifierPostpone = originalPostponing;
    return processCommand(ctx);
}

uint8_t Macros_TryConsumeKeyId(parser_context_t* ctx)
{
    uint8_t keyId = MacroKeyIdParser_TryConsumeKeyId(ctx);

    if (keyId == 255 && isNUM(ctx)) {
        uint8_t num = Macros_ConsumeInt(ctx);
        if (Macros_ParserError) {
            return 255;
        } else {
            return num;
        }
    } else {
        return keyId;
    }
}

static macro_result_t processAutoRepeatCommand(parser_context_t* ctx) {
    if (Macros_DryRun) {
        return processCommand(ctx);
    }

    switch (S->ms.autoRepeatPhase) {
    case AutoRepeatState_Waiting:
        goto process_delay;
    case AutoRepeatState_Executing:
    default:
        goto run_command;
    }

process_delay:;
    uint16_t delay = S->ms.autoRepeatInitialDelayPassed ? Cfg.AutoRepeatDelayRate : Cfg.AutoRepeatInitialDelay;
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
    if (S->ms.oneShot == 0 && OneShot_State != OneShotState_Unwinding) {
        S->ms.oneShot = 1;
        OneShot_Activate(CurrentPostponedTime);
    } else if (OneShot_State == OneShotState_Unwinding) {
        S->ms.oneShot = 2;
    }

    return processCommand(ctx);
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

static macro_result_t processReplaceKeymapCommand(parser_context_t* ctx)
{
    uint8_t srcKeymapId = consumeKeymapId(ctx);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    ReplaceKeymap(srcKeymapId);
    return MacroResult_Finished;
}

static bool processIfKeyPendingAtCommand(parser_context_t* ctx, bool negate)
{
    uint16_t idx = Macros_ConsumeInt(ctx);
    uint16_t key = Macros_TryConsumeKeyId(ctx);

    if (key == 255) {
        Macros_ReportErrorPos(ctx, "Failed to decode keyId.");
        return false;
    }

    if (Macros_DryRun) {
        return true;
    }

    return (PostponerExtended_PendingId(idx) == key) != negate;
}

static bool processIfKeyActiveCommand(parser_context_t* ctx, bool negate)
{
    uint16_t keyid = Macros_TryConsumeKeyId(ctx);

    if (keyid == 255) {
        Macros_ReportErrorPos(ctx, "Failed to decode keyId.");
        return false;
    }

    if (Macros_DryRun) {
        return true;
    }

    key_state_t* key = Utils_KeyIdToKeyState(keyid);

    return KeyState_Active(key) != negate;
}

static bool processIfPendingKeyReleasedCommand(parser_context_t* ctx, bool negate)
{
    uint16_t idx = Macros_ConsumeInt(ctx);
    if (Macros_DryRun) {
        return true;
    }
    return PostponerExtended_IsPendingKeyReleased(idx) != negate;
}

static bool processIfKeyDefinedCommand(parser_context_t* ctx, bool negate)
{
    uint16_t keyid = Macros_TryConsumeKeyId(ctx);

    if (keyid == 255) {
        Macros_ReportErrorPos(ctx, "Failed to decode keyId.");
        return false;
    }

    if (Macros_DryRun) {
        return true;
    }
    uint8_t slot;
    uint8_t slotIdx;
    Utils_DecodeId(keyid, &slot, &slotIdx);
    key_action_t* action = &CurrentKeymap[ActiveLayer][slot][slotIdx];
    return (action->type != KeyActionType_None) != negate;
}

static bool processIfModuleConnected(parser_context_t* ctx, bool negate)
{
    uint8_t moduleId = ConsumeModuleId(ctx);
    if (Macros_DryRun) {
        return true;
    }

    bool moduleConnected = false;

    for (uint8_t moduleSlotId=0; moduleSlotId<UHK_MODULE_MAX_SLOT_COUNT; moduleSlotId++) {
        uhk_module_state_t *moduleState = UhkModuleStates + moduleSlotId;
        moduleConnected |= moduleState->moduleId == moduleId;
    }

    return moduleConnected != negate;
}

static macro_result_t processPanicCommand(parser_context_t* ctx) {
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    Trace_Printc("PretendedPanic");

#ifdef __ZEPHYR__
    k_panic();
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
    static volatile int a = 0;
    static volatile int* b = NULL;
    a = 3 / a;
    a = *b;

    LogS("Failed to crash deliberately.");

    NVIC_SystemReset();
#pragma GCC diagnostic pop
#endif
    return MacroResult_Finished;
}

static macro_result_t processPowerModeCommand(parser_context_t* ctx) {
    bool toggle = false;

    if (ConsumeToken(ctx, "toggle")) {
        toggle = true;
    }

    power_mode_t mode = PowerMode_Awake;

    if (false) { }
    else if (ConsumeToken(ctx, "wake")) { mode = PowerMode_Awake; }
    else if (ConsumeToken(ctx, "lock")) { mode = PowerMode_Lock; }
    else if (ConsumeToken(ctx, "sleep")) { mode = PowerMode_SfjlSleep; }
    else if (ConsumeToken(ctx, "shutdown")) { mode = PowerMode_ManualShutDown; }
    else if (ConsumeToken(ctx, "shutDown")) { mode = PowerMode_ManualShutDown; }
    else if (ConsumeToken(ctx, "autoShutdown")) { mode = PowerMode_AutoShutDown; }
    else {
        Macros_ReportErrorTok(ctx, "This mode is not available in this release:");
    }

    if (Macros_DryRun || Macros_ParserError) {
        return MacroResult_Finished;
    }

    /* wait until the key is released to prevent backlight flashing */
    if (Macros_CurrentMacroKeyIsActive()) {
        return Macros_SleepTillKeystateChange();
    }

    PowerMode_ActivateMode(mode, toggle, false, "triggered by macro command");

    return MacroResult_Finished;
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

    uint16_t keyid = Macros_TryConsumeKeyId(ctx);
    key_state_t* key = Utils_KeyIdToKeyState(keyid);

    if (keyid == 255) {
        Macros_ReportErrorPos(ctx, "Failed to decode keyId.");
        return MacroResult_Finished;
    }

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }
    if (Macros_DryRun) {
        return true;
    }

    if (append) {
        if (PostponerQuery_IsActiveEventually(key)) {
            PostponerCore_TrackKeyEvent(key, false, layer, Timer_GetCurrentTime());
            PostponerCore_TrackKeyEvent(key, true, layer, Timer_GetCurrentTime());
        } else {
            PostponerCore_TrackKeyEvent(key, true, layer, Timer_GetCurrentTime());
            PostponerCore_TrackKeyEvent(key, false, layer, Timer_GetCurrentTime());
        }
    } else {
        if (KeyState_Active(key)) {
            //reverse order when prepending
            PostponerCore_PrependKeyEvent(key, true, layer, Timer_GetCurrentTime());
            PostponerCore_PrependKeyEvent(key, false, layer, Timer_GetCurrentTime());
        } else {
            PostponerCore_PrependKeyEvent(key, false, layer, Timer_GetCurrentTime());
            PostponerCore_PrependKeyEvent(key, true, layer, Timer_GetCurrentTime());
        }
    }
    return MacroResult_Finished;
}

static macro_result_t processConsumePendingCommand(parser_context_t* ctx)
{
    uint16_t cnt = Macros_ConsumeInt(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    PostponerExtended_ConsumePendingKeypresses(cnt, true);
    return MacroResult_Finished;
}

static macro_result_t processPostponeNextNCommand(parser_context_t* ctx)
{
    uint16_t cnt = Macros_ConsumeInt(ctx);
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    //PostponerCore_PostponeNCycles(MACRO_CYCLES_TO_POSTPONE);
    postponeNextN(cnt);
    return MacroResult_Finished;
}

static void consumeAnyJumpTarget(parser_context_t* ctx)
{
    if (isNUM(ctx)) {
        Macros_ConsumeInt(ctx);
    } else {
        ConsumeAnyIdentifier(ctx);
    }
}

static macro_result_t processRepeatForCommand(parser_context_t* ctx)
{
    macro_variable_t* v = Macros_ConsumeExistingWritableVariable(ctx);

    if (Macros_DryRun) {
        consumeAnyJumpTarget(ctx);
    }

    if (v != NULL) {
        if (v->asInt > 0) {
            v->asInt--;
            if (v->asInt > 0) {
                return goTo(ctx);
            } else {
                consumeAnyJumpTarget(ctx);
            }
        } else {
            consumeAnyJumpTarget(ctx);
        }
    }

    return MacroResult_Finished;
}

static macro_result_t processTrackpointCommand(parser_context_t* ctx)
{
    module_specific_command_t command = 0;
    if (ConsumeToken(ctx, "run")) {
        command = ModuleSpecificCommand_RunTrackpoint;
    }
    else if (ConsumeToken(ctx, "signalData")) {
        command = ModuleSpecificCommand_TrackpointSignalData;
    }
    else if (ConsumeToken(ctx, "signalClock")) {
        command = ModuleSpecificCommand_TrackpointSignalClock;
    }
    else {
        Macros_ReportError("Unrecognized trackpoint command:", ctx->at, ctx->end);
    }

    if (Macros_DryRun || Macros_ParserError) {
        return MacroResult_Finished;
    }

    UhkModuleSlaveDriver_SendTrackpointCommand(command);

    return MacroResult_Finished;
}

static macro_result_t processTestLeakageCommand(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

#ifndef __ZEPHYR__
    TestLeakage(1000);
#endif

    return MacroResult_Finished;
}

static macro_result_t processTestSuiteCommand(parser_context_t* ctx)
{
    // Check for optional parameters
    if (!IsEnd(ctx)) {
        // Check for "all" keyword
        if (ConsumeToken(ctx, "all")) {
            if (!Macros_DryRun) {
                TestSuite_RunAll();
            }
        } else {
            // Parse module name
            const char* moduleStart = ctx->at;
            const char* moduleEnd = TokEnd(ctx->at, ctx->end);
            ConsumeAnyToken(ctx);

            // Parse test name
            if (!IsEnd(ctx)) {
                const char* testStart = ctx->at;
                const char* testEnd = TokEnd(ctx->at, ctx->end);
                ConsumeAnyToken(ctx);

                if (!Macros_DryRun) {
                    TestSuite_RunSingle(moduleStart, moduleEnd, testStart, testEnd);
                }
            } else {
                Macros_ReportError("testSuite requires both module and test name", NULL, NULL);
            }
        }
    } else {
        if (!Macros_DryRun) {
            TestSuite_RunAll();
        }
    }

    return MacroResult_Finished;
}

static macro_result_t processResetTrackpointCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    UhkModuleSlaveDriver_SendTrackpointCommand(ModuleSpecificCommand_ResetTrackpoint);

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
        return res;
    }

    if (res & MacroResult_InProgressFlag || res & MacroResult_DoneFlag) {
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
        &Cfg.LedMap_ConstantRGB.red,
        &Cfg.LedMap_ConstantRGB.green,
        &Cfg.LedMap_ConstantRGB.blue
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
    EventVector_Set(EventVector_LedMapUpdateNeeded);
    return MacroResult_Finished;
#undef C
}

static macro_result_t processValidateMacrosCommand(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    Macros_ValidateAllMacros();
    return MacroResult_Finished;
}

static macro_result_t processResetConfigurationCommand(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    ConfigManager_ResetConfiguration(true);
    return MacroResult_Finished;
}

static macro_result_t processReconnectCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

#ifdef __ZEPHYR__
    HostConnections_Reconnect();
#endif
    return MacroResult_Finished;
}

static macro_result_t processRebootCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    Reboot(true);
    return MacroResult_Finished;
}

static macro_result_t processFreezeCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    while (true) {
        // Hi there!
    }

    return MacroResult_Finished;
}

static macro_result_t Macros_ProcessUnpairHostCommand(parser_context_t* ctx)
{
    ATTR_UNUSED uint8_t slotId = 0;
    ATTR_UNUSED uint8_t connId = 0;

    if (Macros_IsNUM(ctx)) {
        ATTR_UNUSED uint8_t arg = Macros_ConsumeInt(ctx);
#ifdef __ZEPHYR__
        slotId = arg;
        connId = ConnectionId_HostConnectionFirst + slotId - 1;
#endif
    } else {
#ifdef __ZEPHYR__
        connId = HostConnections_NameToConnId(ctx);
        slotId = connId - ConnectionId_HostConnectionFirst + 1;
#endif
        Macros_ConsumeStringToken(ctx);
    }

    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

#ifdef __ZEPHYR__
    HostConnections_ClearConnectionByConnId(connId);
#endif

    return MacroResult_Finished;
}

static macro_result_t processSwitchHostCommand(parser_context_t* ctx)
{
#define DRY_RUN_FINISH() if (Macros_DryRun) { return MacroResult_Finished; }

#ifdef __ZEPHYR__
    if (Macros_IsNUM(ctx)) {
        uint8_t hostConnectionIndex = Macros_ConsumeInt(ctx) - 1;
        DRY_RUN_FINISH();
        HostConnections_SelectByHostConnIndex(hostConnectionIndex);
    }
    else if (ConsumeToken(ctx, "next")) {
        DRY_RUN_FINISH();
        HostConnections_SelectNextConnection();
    }
    else if (ConsumeToken(ctx, "prev") || ConsumeToken(ctx, "previous")) {
        DRY_RUN_FINISH();
        HostConnections_SelectPreviousConnection();
    }
    else if (ConsumeToken(ctx, "last")) {
        DRY_RUN_FINISH();
        HostConnections_SelectLastConnection();
    }
    else if (ConsumeToken(ctx, "lastSelected")) {
        DRY_RUN_FINISH();
        HostConnections_SelectLastSelectedConnection();
    }
    else {
        if (!Macros_DryRun) {
            HostConnections_SelectByName(ctx);
        }
        Macros_ConsumeStringToken(ctx);
    }
#else
    Macros_ConsumeStringToken(ctx);
#endif

#undef DRY_RUN_FINISH

    return MacroResult_Finished;
}

static macro_result_t processZephyrCommand(parser_context_t* ctx) {
    if (Macros_DryRun) {
        ctx->at = ctx->end;
        return MacroResult_Finished;
    }
#ifdef __ZEPHYR__
#define LEN 64
    char buffer[LEN];

    size_t len = MIN(LEN-1, (ctx->end - ctx->at));
    strncpy(buffer, ctx->at, len);
    buffer[len] = '\0';

    Shell_Execute(buffer, "macro");

    ctx->at = ctx->end;
    return MacroResult_Finished;
#undef LEN
#else
    Macros_ReportErrorPrintf(ctx->at, "Zephyr commands are not available on uhk60.\n");

    ctx->at = ctx->end;
    return MacroResult_Finished;
#endif
}

// Helper macro for condition commands that follow the pattern:
// if (!processXCommand(...) && !S->ls->as.currentConditionPassed) return MacroResult_Finished | MacroResult_ConditionFailedFlag;
#define CONDITION_FAILED_RESULT (MacroResult_Finished | MacroResult_ConditionFailedFlag)
#define PROCESS_CONDITION(expr) \
    if (!(expr) && !S->ls->as.currentConditionPassed) { \
        return CONDITION_FAILED_RESULT; \
    } \
    break;

static macro_result_t dispatchCommand(parser_context_t* ctx, command_id_t commandId, bool *headersFinished) {
    // Dispatch based on command ID
    switch (commandId) {
    // 'a' commands
    case CommandId_activateKeyPostponed:
        return processActivateKeyPostponedCommand(ctx);
    case CommandId_autoRepeat:
        return processAutoRepeatCommand(ctx);
    case CommandId_addReg:
        Macros_ReportErrorPos(ctx, "Command was removed, please use command similar to `setVar varName ($varName+1)`.");
        return MacroResult_Finished;

    // 'b' commands
    case CommandId_break:
        return processBreakCommand(ctx);
    case CommandId_bluetooth:
        return processBluetoothCommand(ctx);

    // 'c' commands
    case CommandId_consumePending:
        return processConsumePendingCommand(ctx);
    case CommandId_clearStatus:
        return Macros_ProcessClearStatusCommand(true);
    case CommandId_call:
        return processCallCommand(ctx);

    // 'd' commands
    case CommandId_delayUntilRelease:
        return processDelayUntilReleaseCommand();
    case CommandId_delayUntilReleaseMax:
        return processDelayUntilReleaseMaxCommand(ctx);
    case CommandId_delayUntil:
        return processDelayUntilCommand(ctx);
    case CommandId_diagnose:
        return Macros_ProcessDiagnoseCommand();

    // 'e' commands
    case CommandId_exec:
        return processExecCommand(ctx);
    case CommandId_else:
        if (!Macros_DryRun && S->ls->ms.lastIfSucceeded) {
            return MacroResult_Finished;
        }
        break;
    case CommandId_exit:
        return processExitCommand(ctx);

    // 'f' commands
    case CommandId_final:
        return processFinalCommand(ctx);
    case CommandId_fork:
        return processForkCommand(ctx);
    case CommandId_freeze:
        return processFreezeCommand(ctx);

    // 'g' commands
    case CommandId_goTo:
        return processGoToCommand(ctx);

    // 'h' commands
    case CommandId_holdLayer:
        return processHoldLayerCommand(ctx);
    case CommandId_holdLayerMax:
        return processHoldLayerMaxCommand(ctx);
    case CommandId_holdKeymapLayer:
        return processHoldKeymapLayerCommand(ctx);
    case CommandId_holdKeymapLayerMax:
        return processHoldKeymapLayerMaxCommand(ctx);
    case CommandId_holdKey:
        return Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Hold, &S->ms.reports);

    // 'i' commands - conditionals
    case CommandId_if:
        PROCESS_CONDITION(processIfCommand(ctx))
    case CommandId_ifDoubletap:
        PROCESS_CONDITION(processIfDoubletapCommand(false))
    case CommandId_ifNotDoubletap:
        PROCESS_CONDITION(processIfDoubletapCommand(true))
    case CommandId_ifInterrupted:
        PROCESS_CONDITION(processIfInterruptedCommand(false))
    case CommandId_ifNotInterrupted:
        PROCESS_CONDITION(processIfInterruptedCommand(true))
    case CommandId_ifReleased:
        PROCESS_CONDITION(processIfReleasedCommand(false))
    case CommandId_ifNotReleased:
        PROCESS_CONDITION(processIfReleasedCommand(true))
    case CommandId_ifKeymap:
        PROCESS_CONDITION(processIfKeymapCommand(ctx, false))
    case CommandId_ifNotKeymap:
        PROCESS_CONDITION(processIfKeymapCommand(ctx, true))
    case CommandId_ifLayer:
        PROCESS_CONDITION(processIfLayerCommand(ctx, false))
    case CommandId_ifNotLayer:
        PROCESS_CONDITION(processIfLayerCommand(ctx, true))
    case CommandId_ifLayerToggled:
        PROCESS_CONDITION(processIfLayerToggledCommand(ctx, false))
    case CommandId_ifNotLayerToggled:
        PROCESS_CONDITION(processIfLayerToggledCommand(ctx, true))
    case CommandId_ifPlaytime:
        PROCESS_CONDITION(processIfPlaytimeCommand(ctx, false))
    case CommandId_ifNotPlaytime:
        PROCESS_CONDITION(processIfPlaytimeCommand(ctx, true))
    case CommandId_ifAnyMod:
        PROCESS_CONDITION(processIfModifierCommand(false, 0xFF))
    case CommandId_ifNotAnyMod:
        PROCESS_CONDITION(processIfModifierCommand(true, 0xFF))
    case CommandId_ifShift:
        PROCESS_CONDITION(processIfModifierCommand(false, SHIFTMASK))
    case CommandId_ifNotShift:
        PROCESS_CONDITION(processIfModifierCommand(true, SHIFTMASK))
    case CommandId_ifCtrl:
        PROCESS_CONDITION(processIfModifierCommand(false, CTRLMASK))
    case CommandId_ifNotCtrl:
        PROCESS_CONDITION(processIfModifierCommand(true, CTRLMASK))
    case CommandId_ifAlt:
        PROCESS_CONDITION(processIfModifierCommand(false, ALTMASK))
    case CommandId_ifNotAlt:
        PROCESS_CONDITION(processIfModifierCommand(true, ALTMASK))
    case CommandId_ifGui:
        PROCESS_CONDITION(processIfModifierCommand(false, GUIMASK))
    case CommandId_ifNotGui:
        PROCESS_CONDITION(processIfModifierCommand(true, GUIMASK))
    case CommandId_ifCapsLockOn:
        PROCESS_CONDITION(processIfStateKeyCommand(false, &UsbBasicKeyboard_CapsLockOn))
    case CommandId_ifNotCapsLockOn:
        PROCESS_CONDITION(processIfStateKeyCommand(true, &UsbBasicKeyboard_CapsLockOn))
    case CommandId_ifNumLockOn:
        PROCESS_CONDITION(processIfStateKeyCommand(false, &UsbBasicKeyboard_NumLockOn))
    case CommandId_ifNotNumLockOn:
        PROCESS_CONDITION(processIfStateKeyCommand(true, &UsbBasicKeyboard_NumLockOn))
    case CommandId_ifScrollLockOn:
        PROCESS_CONDITION(processIfStateKeyCommand(false, &UsbBasicKeyboard_ScrollLockOn))
    case CommandId_ifNotScrollLockOn:
        PROCESS_CONDITION(processIfStateKeyCommand(true, &UsbBasicKeyboard_ScrollLockOn))
    case CommandId_ifRecording:
        PROCESS_CONDITION(processIfRecordingCommand(false))
    case CommandId_ifNotRecording:
        PROCESS_CONDITION(processIfRecordingCommand(true))
    case CommandId_ifRecordingId:
        PROCESS_CONDITION(processIfRecordingIdCommand(ctx, false))
    case CommandId_ifNotRecordingId:
        PROCESS_CONDITION(processIfRecordingIdCommand(ctx, true))
    case CommandId_ifNotPending:
        PROCESS_CONDITION(processIfPendingCommand(ctx, true))
    case CommandId_ifPending:
        PROCESS_CONDITION(processIfPendingCommand(ctx, false))
    case CommandId_ifKeyPendingAt:
        PROCESS_CONDITION(processIfKeyPendingAtCommand(ctx, false))
    case CommandId_ifNotKeyPendingAt:
        PROCESS_CONDITION(processIfKeyPendingAtCommand(ctx, true))
    case CommandId_ifKeyActive:
        PROCESS_CONDITION(processIfKeyActiveCommand(ctx, false))
    case CommandId_ifNotKeyActive:
        PROCESS_CONDITION(processIfKeyActiveCommand(ctx, true))
    case CommandId_ifPendingKeyReleased:
        PROCESS_CONDITION(processIfPendingKeyReleasedCommand(ctx, false))
    case CommandId_ifNotPendingKeyReleased:
        PROCESS_CONDITION(processIfPendingKeyReleasedCommand(ctx, true))
    case CommandId_ifKeyDefined:
        PROCESS_CONDITION(processIfKeyDefinedCommand(ctx, false))
    case CommandId_ifNotKeyDefined:
        PROCESS_CONDITION(processIfKeyDefinedCommand(ctx, true))
    case CommandId_ifModuleConnected:
        PROCESS_CONDITION(processIfModuleConnected(ctx, false))
    case CommandId_ifNotModuleConnected:
        PROCESS_CONDITION(processIfModuleConnected(ctx, true))
    case CommandId_ifHold:
        return processIfHoldCommand(ctx, false);
    case CommandId_ifTap:
        return processIfHoldCommand(ctx, true);
    case CommandId_ifSecondary:
        return processIfSecondaryCommand(ctx, false);
    case CommandId_ifPrimary:
        return processIfSecondaryCommand(ctx, true);
    case CommandId_ifShortcut:
        return processIfShortcutCommand(ctx, false, true);
    case CommandId_ifNotShortcut:
        return processIfShortcutCommand(ctx, true, true);
    case CommandId_ifGesture:
        return processIfShortcutCommand(ctx, false, false);
    case CommandId_ifNotGesture:
        return processIfShortcutCommand(ctx, true, false);
    case CommandId_ifRegEq:
    case CommandId_ifNotRegEq:
        Macros_ReportErrorPos(ctx, "Command was removed, please use command similar to `if ($varName == 1)`.");
        return MacroResult_Finished;
    case CommandId_ifRegGt:
    case CommandId_ifRegLt:
        Macros_ReportErrorPos(ctx, "Command was removed, please use command similar to `if ($varName >= 1)`.");
        return MacroResult_Finished;

    // 'm' commands
    case CommandId_macroArg:
        *headersFinished = false;   // this is a valid header command, stay in header mode
        return processMacroArgCommand(ctx);
    case CommandId_mulReg:
        Macros_ReportErrorPos(ctx, "Command was removed, please use command similar to `setVar varName ($varName*2)`.");
        return MacroResult_Finished;

    // 'n' commands
    case CommandId_noOp:
        return processNoOpCommand();
    case CommandId_notify:
        return Macros_ProcessNotifyCommand(ctx);

    // 'o' commands
    case CommandId_oneShot:
        return processOneShotCommand(ctx);
    case CommandId_overlayLayer:
        return processOverlayLayerCommand(ctx);
    case CommandId_overlayKeymap:
        return processOverlayKeymapCommand(ctx);

    // 'p' commands
    case CommandId_printStatus:
        return Macros_ProcessPrintStatusCommand();
    case CommandId_playMacro:
        return processPlayMacroCommand(ctx);
    case CommandId_pressKey:
        return Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Press, &S->ms.reports);
    case CommandId_postponeKeys:
        processPostponeKeysCommand();
        break;
    case CommandId_postponeNext:
        return processPostponeNextNCommand(ctx);
    case CommandId_progressHue:
        return processProgressHueCommand();
    case CommandId_powerMode:
        return processPowerModeCommand(ctx);
    case CommandId_panic:
        return processPanicCommand(ctx);

    // 'r' commands
    case CommandId_recordMacro:
        return processRecordMacroCommand(ctx, false);
    case CommandId_recordMacroBlind:
        return processRecordMacroCommand(ctx, true);
    case CommandId_recordMacroDelay:
        return processRecordMacroDelayCommand();
    case CommandId_resolveNextKeyId:
        return processResolveNextKeyIdCommand();
    case CommandId_releaseKey:
        return Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Release, &S->ms.reports);
    case CommandId_repeatFor:
        return processRepeatForCommand(ctx);
    case CommandId_resetTrackpoint:
        return processResetTrackpointCommand();
    case CommandId_replaceLayer:
        return processReplaceLayerCommand(ctx);
    case CommandId_replaceKeymap:
        return processReplaceKeymapCommand(ctx);
    case CommandId_resolveNextKeyEq:
        Macros_ReportErrorPos(ctx, "Command deprecated. Please, replace resolveNextKeyEq by ifShortcut or ifGesture, or complain at github that you actually need this.");
        return MacroResult_Finished;
    case CommandId_resolveSecondary:
        Macros_ReportErrorPos(ctx, "Command deprecated. Please, replace resolveSecondary by `ifPrimary advancedStrategy goTo ...` or `ifSecondary advancedStrategy goTo ...`.");
        return MacroResult_Finished;
    case CommandId_resetConfiguration:
        return processResetConfigurationCommand(ctx);
    case CommandId_reboot:
        return processRebootCommand();
    case CommandId_reconnect:
        return processReconnectCommand();

    // 's' commands
    case CommandId_set:
        return Macro_ProcessSetCommand(ctx);
    case CommandId_setVar:
        return Macros_ProcessSetVarCommand(ctx);
    case CommandId_setStatus:
        return Macros_ProcessSetStatusCommand(ctx, true);
    case CommandId_startRecording:
        return processStartRecordingCommand(ctx, false);
    case CommandId_startRecordingBlind:
        return processStartRecordingCommand(ctx, true);
    case CommandId_setLedTxt:
        return Macros_ProcessSetLedTxtCommand(ctx);
    case CommandId_statsRuntime:
        return Macros_ProcessStatsRuntimeCommand();
    case CommandId_statsRecordKeyTiming:
        return Macros_ProcessStatsRecordKeyTimingCommand();
    case CommandId_statsLayerStack:
        return Macros_ProcessStatsLayerStackCommand();
    case CommandId_statsActiveKeys:
        return Macros_ProcessStatsActiveKeysCommand();
    case CommandId_statsActiveMacros:
        return Macros_ProcessStatsActiveMacrosCommand();
    case CommandId_statsPostponerStack:
        return Macros_ProcessStatsPostponerStackCommand();
    case CommandId_statsVariables:
        return Macros_ProcessStatsVariablesCommand();
    case CommandId_statsBattery:
        return Macros_ProcessStatsBatteryCommand();
    case CommandId_switchKeymap:
        return processSwitchKeymapCommand(ctx);
    case CommandId_startMouse:
        return processMouseCommand(ctx, true);
    case CommandId_stopMouse:
        return processMouseCommand(ctx, false);
    case CommandId_stopRecording:
    case CommandId_stopRecordingBlind:
        return processStopRecordingCommand();
    case CommandId_stopAllMacros:
        return processStopAllMacrosCommand();
    case CommandId_suppressMods:
        processSuppressModsCommand();
        break;
    case CommandId_setReg:
        Macros_ReportErrorPos(ctx, "Command was removed, please use named variables. E.g., `setVar myVar 1` and `write \"$myVar\"`");
        return MacroResult_Finished;
    case CommandId_subReg:
        Macros_ReportErrorPos(ctx, "Command was removed, please use command similar to `setVar varName ($varName+1)`.");
        return MacroResult_Finished;
    case CommandId_setStatusPart:
        Macros_ReportErrorPos(ctx, "Command was removed, please use string interpolated setStatus.");
        return MacroResult_Finished;
    case CommandId_switchKeymapLayer:
    case CommandId_switchLayer:
        Macros_ReportErrorPos(ctx, "Command deprecated. Please, replace switchKeymapLayer by toggleKeymapLayer or holdKeymapLayer. Or complain on github that you actually need this command.");
        return MacroResult_Finished;
    case CommandId_switchHost:
        return processSwitchHostCommand(ctx);

    // 't' commands
    case CommandId_toggleKeymapLayer:
        return processToggleKeymapLayerCommand(ctx);
    case CommandId_toggleLayer:
        return processToggleLayerCommand(ctx);
    case CommandId_tapKey:
        return Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Tap, &S->ms.reports);
    case CommandId_tapKeySeq:
        return Macros_ProcessTapKeySeqCommand(ctx);
    case CommandId_toggleKey:
        return Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Toggle, &S->ms.reports);
    case CommandId_trackpoint:
        return processTrackpointCommand(ctx);
    case CommandId_trace:
        if (!Macros_DryRun) {
            Trace_Print(LogTarget_ErrorBuffer, "Triggered by macro command");
        }
        return MacroResult_Finished;
    case CommandId_testLeakage:
        return processTestLeakageCommand(ctx);
    case CommandId_testSuite:
        return processTestSuiteCommand(ctx);

    // 'u' commands
    case CommandId_unToggleLayer:
    case CommandId_untoggleLayer:
        return processUnToggleLayerCommand();
    case CommandId_unpairHost:
        return Macros_ProcessUnpairHostCommand(ctx);

    // 'v' commands
    case CommandId_validateUserConfig:
    case CommandId_validateMacros:
        return processValidateMacrosCommand(ctx);

    // 'w' commands
    case CommandId_write:
        return processWriteCommand(ctx);
    case CommandId_while:
        return processWhileCommand(ctx);
    case CommandId_writeExpr:
        Macros_ReportErrorPos(ctx, "writeExpr is now deprecated, please migrate to interpolated strings");
        return MacroResult_Finished;

    // 'y' commands
    case CommandId_yield:
        return processYieldCommand(ctx);

    // 'z' commands
    case CommandId_zephyr:
        return processZephyrCommand(ctx);

    // brace commands
    case CommandId_openBrace:
        return processOpeningBraceCommand(ctx);
    case CommandId_closeBrace:
        return processClosingBraceCommand(ctx);

    default:
        Macros_ReportErrorTok(ctx, "Unrecognized command:");
        return MacroResult_Finished;
    }
}
    
static macro_result_t processCommand(parser_context_t* ctx)
{
    const char* cmdTokEnd = TokEnd(ctx->at, ctx->end);
    if (cmdTokEnd > ctx->at && cmdTokEnd[-1] == ':') {
        //skip labels
        ConsumeAnyToken(ctx);
        if (ctx->at == ctx->end && IsEnd(ctx)) {
            return MacroResult_Finished;
        }
    }

    while(ctx->at < ctx->end || !IsEnd(ctx)) {
        // Look up the command in the hash table
        const char* cmdAt = ctx->at;
        const struct command_entry* entry = ConsumeGperfToken(ctx);

        if (entry == NULL) {
            Macros_ReportError("Unrecognized command:", cmdAt, TokEnd(cmdAt, ctx->end));
            return MacroResult_Finished;
        }

        // We assume the next command is a non-header command and will therefore
        // finish the header block.
        bool headersProcessed = true;

        macro_result_t res = dispatchCommand(ctx, entry->id, &headersProcessed);

        if (headersProcessed) {
            S->ms.macroHeadersProcessed = true;
        }
    }

    //this is reachable if there is a train of conditions/modifiers/labels without any command
    return MacroResult_Finished;
}

#undef CONDITION_FAILED_RESULT
#undef PROCESS_CONDITION

static bool isOpeningBrace(parser_context_t* ctx)
{
    Macros_DryRun = true;

    macro_result_t macroResult = processCommand(ctx);

    bool res = macroResult & MacroResult_OpeningBraceFlag;

    Macros_DryRun = false;

    return res;
}

static void jumpToMatchingBrace()
{
    uint8_t nesting = 1;
    Macros_DryRun = true;

    bool reachedEnd = false;
    while (!reachedEnd && nesting != 0) {
        reachedEnd = !Macros_LoadNextCommand();
        macro_result_t macroResult = Macros_ProcessCommandAction();

        if (macroResult & MacroResult_OpeningBraceFlag) {
            nesting++;
        }
        if (macroResult & MacroResult_ClosingBraceFlag) {
            nesting--;
        }
    }

    Macros_DryRun = false;

    // First reset the dry run, so that the following code performs state resets
    Macros_LoadNextCommand() || Macros_LoadNextAction() || (S->ms.macroBroken = true);
}

macro_result_t Macros_ProcessCommandAction(void)
{
    const char* cmd = S->ms.currentMacroAction.cmd.text + S->ls->ms.commandBegin;
    const char* cmdEnd = S->ms.currentMacroAction.cmd.text + S->ls->ms.commandEnd;

    parser_context_t ctx = {
        .macroState = S,
        .begin = cmd,
        .at = cmd,
        .end = cmdEnd,
        .nestingLevel = 0,
        .nestingBound = 0,
    };

    ConsumeWhite(&ctx);

    if (ctx.at[0] == '#') {
        Macros_ReportWarn("# comments are deprecated, please switch to //", ctx.at, ctx.at);
        return MacroResult_Finished;
    }
    if (ctx.at[0] == '/' && ctx.at[1] == '/') {
        return MacroResult_Finished;
    }

    macro_result_t macroResult;

#if (DEBUG_CHECK_MACRO_RUN_TIMES && defined(__ZEPHYR__))
    uint32_t start = Timer_GetCurrentTime();
    macroResult = processCommand(&ctx);
    uint64_t end = Timer_GetCurrentTime();
    uint32_t time = end - start;
    if (time > 20) {
        uint32_t cmdLength = (uint32_t)(cmdEnd - cmd);
        Macros_ReportErrorPrintf(cmd, "Macro command took: %d ms to evaluate.\n", time, cmdLength, cmd);
    }
#else
    macroResult = processCommand(&ctx);
#endif

    if ((ctx.at < ctx.end || !IsEnd(&ctx)) && !Macros_ParserError && Macros_DryRun) {
        Macros_ReportWarn("Unprocessed input encountered.", ctx.at, ctx.at);
    }

    S->ls->as.currentConditionPassed = macroResult & MacroResult_InProgressFlag;
    if (!Macros_DryRun && (macroResult & (MacroResult_ActionFinishedFlag | MacroResult_DoneFlag))) {
        S->ls->ms.lastIfSucceeded = !(macroResult & MacroResult_ConditionFailedFlag);
    }

    // This triggers when an unconsumed '{' is left at the end of line.
    //
    // If '{...}' has been completed, '{' command intentionally leaves itself
    // in unconsumed state and returns MacroResult_Finished, so that any
    // commands along the way can choose to repeat the command or not.
    //
    // Finally, if the finished flag reaches us (contrary to some modifier
    // deciding that it wants to rerun the command), we now skip the entire
    // scope
    if (!Macros_DryRun && (macroResult & MacroResult_ActionFinishedFlag) && isOpeningBrace(&ctx)) {
        jumpToMatchingBrace();
        return MacroResult_DoneFlag;
    }

    return macroResult;
}
