#include "macro_set_command.h"
#include "timer.h"
#include "keymap.h"
#include "key_matrix.h"
#include "usb_report_updater.h"
#include "led_display.h"
#include "postponer.h"
#include "macro_recorder.h"
#include "macro_shortcut_parser.h"
#include "str_utils.h"
#include "utils.h"
#include "layer_switcher.h"
#include "mouse_controller.h"
#include "debug.h"
#include "caret_config.h"
#include "config_parser/parse_macro.h"

static const char* proceedByDot(const char* cmd, const char *cmdEnd)
{
    while(*cmd > 32 && *cmd != '.' && cmd < cmdEnd)    {
        cmd++;
    }
    if (*cmd != '.') {
        Macros_ReportError("'.' expected", NULL, NULL);
    }
    return cmd+1;
}

static void moduleNavigationMode(const char* arg1, const char *textEnd, module_configuration_t* module)
{
    layer_id_t layerId = Macros_ParseLayerId(arg1, textEnd);
    navigation_mode_t modeId = ParseNavigationModeId(NextTok(arg1, textEnd), textEnd);

    module->navigationModes[layerId] = modeId;
}

static void moduleSpeed(const char* arg1, const char *textEnd, module_configuration_t* module)
{
    const char* arg2 = NextTok(arg1, textEnd);

    if (TokenMatches(arg1, textEnd, "baseSpeed")) {
        module->baseSpeed = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "speed")) {
        module->speed = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "acceleration")) {
        module->acceleration = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "caretSpeedDivisor")) {
        module->caretSpeedDivisor = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "scrollSpeedDivisor")) {
        module->scrollSpeedDivisor = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "zoomSpeedDivisor")) {
        module->zoomSpeedDivisor = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "axisLockStrength")) {
        module->axisLockSkew = 1.0f - ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "axisLockStrengthFirstTick")) {
        module->axisLockSkewFirstTick = 1.0f - ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "cursorAxisLockEnabled")) {
        module->cursorAxisLock = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else if (TokenMatches(arg1, textEnd, "scrollAxisLockEnabled")) {
        module->scrollAxisLock = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else if (TokenMatches(arg1, textEnd, "swapAxes")) {
        module->swapAxes = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else if (TokenMatches(arg1, textEnd, "invertScrollDirection")) {
        module->invertScrollDirection = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
}

static void module(const char* arg1, const char *textEnd)
{
    layer_id_t moduleId = ParseModuleId(arg1, textEnd);
    module_configuration_t* module = GetModuleConfiguration(moduleId);

    const char* arg2 = proceedByDot(arg1, textEnd);

    if (TokenMatches(arg2, textEnd, "navigationMode")) {
        const char* arg3 = proceedByDot(arg2, textEnd);
        moduleNavigationMode(arg3, textEnd, module);
    }
    else {
        moduleSpeed(arg2, textEnd, module);
    }
}

static void secondaryRoles(const char* arg1, const char *textEnd)
{
    //Todo when they are merged
}

static void mouseKeys(const char* arg1, const char *textEnd)
{
    mouse_kinetic_state_t* state = &MouseMoveState;

    if (TokenMatches(arg1, textEnd, "move")) {
        state = &MouseMoveState;
    } else if (TokenMatches(arg1, textEnd, "scroll")) {
        state = &MouseScrollState;
    } else {
        Macros_ReportError("scroll or move expected", NULL, NULL);
    }

    const char* arg2 = proceedByDot(arg1, textEnd);
    const char* arg3 = NextTok(arg2, textEnd);

    if (TokenMatches(arg2, textEnd, "initialSpeed")) {
        state->initialSpeed = Macros_ParseInt(arg3, textEnd, NULL);
    }
    else if (TokenMatches(arg2, textEnd, "baseSpeed")) {
        state->baseSpeed = Macros_ParseInt(arg3, textEnd, NULL);
    }
    else if (TokenMatches(arg2, textEnd, "initialAcceleration")) {
        state->acceleration = Macros_ParseInt(arg3, textEnd, NULL);
    }
    else if (TokenMatches(arg2, textEnd, "deceleratedSpeed")) {
        state->deceleratedSpeed = Macros_ParseInt(arg3, textEnd, NULL);
    }
    else if (TokenMatches(arg2, textEnd, "acceleratedSpeed")) {
        state->acceleratedSpeed = Macros_ParseInt(arg3, textEnd, NULL);
    }
    else if (TokenMatches(arg2, textEnd, "axisSkew")) {
        state->axisSkew = ParseFloat(arg3, textEnd);
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
}

static void stickyMods(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "0") || TokenMatches(arg1, textEnd, "never")) {
        StickyModifierStrategy = Stick_Never;
    }
    else if (TokenMatches(arg1, textEnd, "smart")) {
        StickyModifierStrategy = Stick_Smart;
    }
    else if (TokenMatches(arg1, textEnd, "1") || TokenMatches(arg1, textEnd, "always")) {
        StickyModifierStrategy = Stick_Always;
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
}

static void macroEngineScheduler(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "preemptive")) {
        Macros_Scheduler = Scheduler_Preemptive;
    }
    else if (TokenMatches(arg1, textEnd, "blocking")) {
        Macros_Scheduler = Scheduler_Blocking;
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
}

static void macroEngine(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "scheduler")) {
        macroEngineScheduler(NextTok(arg1,  textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "batchSize")) {
        Macros_MaxBatchSize = Macros_ParseInt(NextTok(arg1,  textEnd), textEnd, NULL);
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
}

static void navigationModeAction(const char* arg1, const char *textEnd)
{
    navigation_mode_t navigationMode = NavigationMode_Caret;
    bool positive = true;
    caret_axis_t axis = CaretAxis_Horizontal;

    const char* arg2 = proceedByDot(arg1, textEnd);
    const char* arg3 = NextTok(arg2, textEnd);

    navigationMode = ParseNavigationModeId(arg1, textEnd);

    if(navigationMode != NavigationMode_Caret && navigationMode != NavigationMode_Media && navigationMode != NavigationMode_Zoom) {
        Macros_ReportError("Invalid or non-remapable navigation mode", arg1, textEnd);
    }

    if (TokenMatches(arg2, textEnd, "left")) {
        axis = CaretAxis_Horizontal;
        positive = false;
    }
    else if (TokenMatches(arg2, textEnd, "up")) {
        axis = CaretAxis_Vertical;
        positive = false;
    }
    else if (TokenMatches(arg2, textEnd, "right")) {
        axis = CaretAxis_Horizontal;
        positive = true;
    }
    else if (TokenMatches(arg2, textEnd, "down")) {
        axis = CaretAxis_Vertical;
        positive = true;
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }

    uint8_t macroIndex;
    if (TokenMatches(arg3, textEnd, "none")) {
        macroIndex = 255;
    } else {
        macroIndex = FindMacroIndexByName(arg3, TokEnd(arg3, textEnd), true);
    }

    SetModuleCaretConfiguration(navigationMode, axis, positive, macroIndex);
}

static void keymapAction(const char* arg1, const char *textEnd)
{
    const char* arg2 = proceedByDot(arg1, textEnd);
    const char* arg3 = NextTok(arg2, textEnd);

    uint8_t layerId = Macros_ParseLayerId(arg1, textEnd);
    uint16_t keyId = Macros_ParseInt(arg2, textEnd, NULL);

    uint8_t macroIndex = FindMacroIndexByName(arg3, TokEnd(arg3, textEnd), true);

    uint8_t slotIdx = keyId/64;
    uint8_t inSlotIdx = keyId%64;

    if(slotIdx > SLOT_COUNT || inSlotIdx > MAX_KEY_COUNT_PER_MODULE) {
        Macros_ReportError("invalid key id:", arg2, textEnd);
    }

    key_action_t* action = &CurrentKeymap[layerId][slotIdx][inSlotIdx];

    if(macroIndex == 255) {
        action->type = KeyActionType_None;
    } else {
        action->type = KeyActionType_PlayMacro;
        action->playMacro.macroId = macroIndex;
    }
}

macro_result_t MacroSetCommand(const char* arg1, const char *textEnd)
{
    const char* arg2 = NextTok(arg1, textEnd);

    if (TokenMatches(arg1, textEnd, "module")) {
        module(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "secondaryRoles")) {
        secondaryRoles(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "mouseKeys")) {
        mouseKeys(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "keymapAction")) {
        keymapAction(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "navigationModeAction")) {
        navigationModeAction(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "macroEngine")) {
        macroEngine(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "compensateDiagonalSpeed")) {
        CompensateDiagonalSpeed = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else if (TokenMatches(arg1, textEnd, "stickyMods")) {
        stickyMods(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "debounceDelay")) {
        uint16_t time = Macros_ParseInt(arg2, textEnd, NULL);

        DebounceTimePress = time;
        DebounceTimeRelease = time;
    }
    else if (TokenMatches(arg1, textEnd, "keystrokeDelay")) {
        KeystrokeDelay = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else if (TokenMatches(arg1, textEnd, "chording")) {
        Chording = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else if (TokenMatches(arg1, textEnd, "emergencyKey")) {
        uint16_t key = Macros_ParseInt(arg2, textEnd, NULL);
        EmergencyKey = Utils_KeyIdToKeyState(key);
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
    return MacroResult_Finished;
}
