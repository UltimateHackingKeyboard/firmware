#include "macro_set_command.h"
#include "config_parser/parse_config.h"
#include "layer.h"
#include "ledmap.h"
#include "macros.h"
#include "module.h"
#include "secondary_role_driver.h"
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
#include "slave_drivers/is31fl3xxx_driver.h"
#include "init_peripherals.h"
#include <stdint.h>

static const char* proceedByDot(const char* cmd, const char *cmdEnd)
{
    while(*cmd > 32 && *cmd != '.' && cmd < cmdEnd)    {
        cmd++;
    }
    if (*cmd != '.') {
        Macros_ReportError("'.' expected", cmd, cmd);
    }
    return cmd+1;
}

static void moduleNavigationMode(const char* arg1, const char *textEnd, module_configuration_t* module)
{
    layer_id_t layerId = Macros_ParseLayerId(arg1, textEnd);
    navigation_mode_t modeId = ParseNavigationModeId(NextTok(arg1, textEnd), textEnd);

    if (IS_MODIFIER_LAYER(layerId)) {
        Macros_ReportError("Navigation mode cannot be changed for modifier layers!", arg1, arg1);
        return;
    }

    if (Macros_ParserError) {
        return;
    }
    if (Macros_DryRun) {
        return ;
    }

    module->navigationModes[layerId] = modeId;
}

static void moduleSpeed(const char* arg1, const char *textEnd, module_configuration_t* module, uint8_t moduleId)
{
    const char* arg2 = NextTok(arg1, textEnd);

    if (TokenMatches(arg1, textEnd, "baseSpeed")) {
        float v = ParseFloat(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->baseSpeed = v;
    }
    else if (TokenMatches(arg1, textEnd, "speed")) {
        float v = ParseFloat(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->speed = v;
    }
    else if (TokenMatches(arg1, textEnd, "xceleration")) {
        float v = ParseFloat(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->xceleration = v;
    }
    else if (TokenMatches(arg1, textEnd, "caretSpeedDivisor")) {
        float v = ParseFloat(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->caretSpeedDivisor = v;
    }
    else if (TokenMatches(arg1, textEnd, "scrollSpeedDivisor")) {
        float v = ParseFloat(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->scrollSpeedDivisor = v;
    }
    else if (TokenMatches(arg1, textEnd, "pinchZoomDivisor") && moduleId == ModuleId_TouchpadRight) {
        float v = ParseFloat(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->pinchZoomSpeedDivisor = v;
    }
    else if (TokenMatches(arg1, textEnd, "pinchZoomMode") && moduleId == ModuleId_TouchpadRight) {
        navigation_mode_t v = ParseNavigationModeId(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        TouchpadPinchZoomMode = v;
    }
    else if (TokenMatches(arg1, textEnd, "axisLockSkew")) {
        float v = ParseFloat(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->axisLockSkew = v;
    }
    else if (TokenMatches(arg1, textEnd, "axisLockFirstTickSkew")) {
        float v = ParseFloat(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->axisLockFirstTickSkew = v;
    }
    else if (TokenMatches(arg1, textEnd, "cursorAxisLock")) {
        bool v = Macros_ParseBoolean(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->cursorAxisLock = v;
    }
    else if (TokenMatches(arg1, textEnd, "scrollAxisLock")) {
        bool v = Macros_ParseBoolean(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->scrollAxisLock = v;
    }
    else if (TokenMatches(arg1, textEnd, "caretAxisLock")) {
        bool v = Macros_ParseBoolean(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->caretAxisLock = v;
    }
    else if (TokenMatches(arg1, textEnd, "swapAxes")) {
        bool v = Macros_ParseBoolean(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->swapAxes = v;
    }
    else if (TokenMatches(arg1, textEnd, "invertScrollDirection")) {
        bool v = Macros_ParseBoolean(arg2, textEnd);
        if (Macros_DryRun) {
            Macros_ReportWarn("Command deprecated. Please, replace invertScrollDirection by invertScrollDirectionY.", arg1, arg1);
            return;
        }
        module->invertScrollDirectionY = v;
    }
    else if (TokenMatches(arg1, textEnd, "invertScrollDirectionY")) {
        bool v = Macros_ParseBoolean(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->invertScrollDirectionY = v;
    }
    else if (TokenMatches(arg1, textEnd, "invertScrollDirectionX")) {
        bool v = Macros_ParseBoolean(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        module->invertScrollDirectionX = v;
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
}

static void module(const char* arg1, const char *textEnd)
{
    module_id_t moduleId = ParseModuleId(arg1, textEnd);
    module_configuration_t* module = GetModuleConfiguration(moduleId);

    const char* arg2 = proceedByDot(arg1, textEnd);

    if (Macros_ParserError) {
        return;
    }

    if (TokenMatches(arg2, textEnd, "navigationMode")) {
        const char* arg3 = proceedByDot(arg2, textEnd);
        moduleNavigationMode(arg3, textEnd, module);
    }
    else {
        moduleSpeed(arg2, textEnd, module, moduleId);
    }
}

static void secondaryRoleAdvanced(const char* arg1, const char *textEnd)
{
    const char* arg2 = NextTok(arg1, textEnd);

    if (TokenMatches(arg1, textEnd, "timeout")) {
        uint32_t v = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return;
        }
        SecondaryRoles_AdvancedStrategyTimeout = v;
    }
    else if (TokenMatches(arg1, textEnd, "timeoutAction")) {
        if (TokenMatches(arg2, textEnd, "primary")) {
            if (Macros_DryRun) {
                return;
            }
            SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Primary;
        }
        else if (TokenMatches(arg2, textEnd, "secondary")) {
            if (Macros_DryRun) {
                return;
            }
            SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Secondary;
        }
        else {
            Macros_ReportError("Parameter not recognized:", arg2, textEnd);
        }
    }
    else if (TokenMatches(arg1, textEnd, "safetyMargin")) {
        uint32_t v = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return;
        }
        SecondaryRoles_AdvancedStrategySafetyMargin = v;
    }
    else if (TokenMatches(arg1, textEnd, "triggerByRelease")) {
        bool v = Macros_ParseBoolean(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        SecondaryRoles_AdvancedStrategyTriggerByRelease = v;
    }
    else if (TokenMatches(arg1, textEnd, "doubletapToPrimary")) {
        bool v = Macros_ParseBoolean(arg2, textEnd);
        if (Macros_DryRun) {
            return;
        }
        SecondaryRoles_AdvancedStrategyDoubletapToPrimary = v;
    }
    else if (TokenMatches(arg1, textEnd, "doubletapTime")) {
        int32_t v = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return;
        }
        SecondaryRoles_AdvancedStrategyDoubletapTime = v;
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
}

static void secondaryRoles(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "defaultStrategy")) {
        const char* arg2 = NextTok(arg1, textEnd);
        if (TokenMatches(arg2, textEnd, "simple")) {
            if (Macros_DryRun) {
                return;
            }
            SecondaryRoles_Strategy = SecondaryRoleStrategy_Simple;
        }
        else if (TokenMatches(arg2, textEnd, "advanced")) {
            if (Macros_DryRun) {
                return;
            }
            SecondaryRoles_Strategy = SecondaryRoleStrategy_Advanced;
        }
        else {
            Macros_ReportError("Parameter not recognized:", arg2, textEnd);
        }
    }
    else if (TokenMatches(arg1, textEnd, "advanced")) {
        secondaryRoleAdvanced(proceedByDot(arg1, textEnd), textEnd);
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
}

static void mouseKeys(const char* arg1, const char *textEnd)
{
    mouse_kinetic_state_t* state = &MouseMoveState;

    if (TokenMatches(arg1, textEnd, "move")) {
        state = &MouseMoveState;
    } else if (TokenMatches(arg1, textEnd, "scroll")) {
        state = &MouseScrollState;
    } else {
        Macros_ReportError("Scroll or move expected", arg1, arg1);
    }

    const char* arg2 = proceedByDot(arg1, textEnd);
    const char* arg3 = NextTok(arg2, textEnd);

    if (TokenMatches(arg2, textEnd, "initialSpeed")) {
        int32_t v = Macros_ParseInt(arg3, textEnd, NULL) / state->intMultiplier;
        if (Macros_DryRun) {
            return;
        }
        state->initialSpeed = v;
    }
    else if (TokenMatches(arg2, textEnd, "baseSpeed")) {
        int32_t v = Macros_ParseInt(arg3, textEnd, NULL) / state->intMultiplier;
        if (Macros_DryRun) {
            return;
        }
        state->baseSpeed = v;
    }
    else if (TokenMatches(arg2, textEnd, "initialAcceleration")) {
        int32_t v = Macros_ParseInt(arg3, textEnd, NULL) / state->intMultiplier;
        if (Macros_DryRun) {
            return;
        }
        state->acceleration = v;
    }
    else if (TokenMatches(arg2, textEnd, "deceleratedSpeed")) {
        int32_t v = Macros_ParseInt(arg3, textEnd, NULL) / state->intMultiplier;
        if (Macros_DryRun) {
            return;
        }
        state->deceleratedSpeed = v;
    }
    else if (TokenMatches(arg2, textEnd, "acceleratedSpeed")) {
        int32_t v = Macros_ParseInt(arg3, textEnd, NULL) / state->intMultiplier;
        if (Macros_DryRun) {
            return;
        }
        state->acceleratedSpeed = v;
    }
    else if (TokenMatches(arg2, textEnd, "axisSkew")) {
        float v = ParseFloat(arg3, textEnd);
        if (Macros_DryRun) {
            return;
        }
        state->axisSkew = v;
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
}

static void stickyModifiers(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "never")) {
        sticky_strategy_t v = Stick_Never;
        if (Macros_DryRun) {
            return;
        }
        StickyModifierStrategy = v;
    }
    else if (TokenMatches(arg1, textEnd, "smart")) {
        sticky_strategy_t v = Stick_Smart;
        if (Macros_DryRun) {
            return;
        }
        StickyModifierStrategy = v;
    }
    else if (TokenMatches(arg1, textEnd, "always")) {
        sticky_strategy_t v = Stick_Always;
        if (Macros_DryRun) {
            return;
        }
        StickyModifierStrategy = v;
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
}

static void macroEngineScheduler(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "preemptive")) {
        macro_scheduler_t v = Scheduler_Preemptive;
        if (Macros_DryRun) {
            return;
        }
        Macros_Scheduler = v;
    }
    else if (TokenMatches(arg1, textEnd, "blocking")) {
        macro_scheduler_t v = Scheduler_Blocking;
        if (Macros_DryRun) {
            return;
        }
        Macros_Scheduler = v;
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
}

static void macroEngine(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "scheduler")) {
        macroEngineScheduler(NextTok(arg1,  textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "batchSize")) {
        int32_t v = Macros_ParseInt(NextTok(arg1,  textEnd), textEnd, NULL);
        if (Macros_DryRun) {
            return;
        }
        Macros_MaxBatchSize = v;
    }
    else if (TokenMatches(arg1, textEnd, "extendedCommands")) {
        /* this option was removed -> accept the command & do nothing */
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
}

static void backlightStrategy(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "functional")) {
        if (Macros_DryRun) {
            return;
        }
        Ledmap_SetLedBacklightingMode(BacklightingMode_Functional);
        Ledmap_UpdateBacklightLeds();
    }
    else if (TokenMatches(arg1, textEnd, "constantRgb")) {
        if (Macros_DryRun) {
            return;
        }
        Ledmap_SetLedBacklightingMode(BacklightingMode_ConstantRGB);
        Ledmap_UpdateBacklightLeds();
    }
    else if (TokenMatches(arg1, textEnd, "perKeyRgb")) {
        if (PerKeyRgbPresent) {
            if (Macros_DryRun) {
                return;
            }
            Ledmap_SetLedBacklightingMode(BacklightingMode_PerKeyRgb);
            Ledmap_UpdateBacklightLeds();
        } else {
            Macros_ReportError("Cannot set perKeyRgb mode when perKeyRgb maps are not available. Please, consult Agent's led section...", arg1, arg1);
        }
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
}

static void keyRgb(const char* arg1, const char *textEnd)
{
    const char* arg2 = proceedByDot(arg1, textEnd);

    layer_id_t layerId = Macros_ParseLayerId(arg1, textEnd);
    uint8_t keyId = Macros_ParseInt(arg2, textEnd, NULL);
    const char* r = NextTok(arg2,  textEnd);
    const char* g = NextTok(r, textEnd);
    const char* b = NextTok(g, textEnd);
    rgb_t rgb;
    rgb.red = Macros_ParseInt(r, textEnd, NULL);
    rgb.green = Macros_ParseInt(g, textEnd, NULL);
    rgb.blue = Macros_ParseInt(b, textEnd, NULL);

    if (Macros_ParserError) {
        return;
    }
    if (Macros_DryRun) {
        return;
    }

    uint8_t slotIdx = keyId/64;
    uint8_t inSlotIdx = keyId%64;

    CurrentKeymap[layerId][slotIdx][inSlotIdx].colorOverridden = true;
    CurrentKeymap[layerId][slotIdx][inSlotIdx].color = rgb;

    Ledmap_UpdateBacklightLeds();
}


static void constantRgb(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "rgb")) {
        const char* r = NextTok(arg1,  textEnd);
        const char* g = NextTok(r, textEnd);
        const char* b = NextTok(g, textEnd);
        rgb_t rgb;
        rgb.red = Macros_ParseInt(r, textEnd, NULL);
        rgb.green = Macros_ParseInt(g, textEnd, NULL);
        rgb.blue = Macros_ParseInt(b, textEnd, NULL);

        if (Macros_DryRun) {
            return;
        }

        LedMap_ConstantRGB = rgb;
        Ledmap_SetLedBacklightingMode(BacklightingMode_ConstantRGB);
        Ledmap_UpdateBacklightLeds();
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
}

static void leds(const char* arg1, const char *textEnd)
{
    const char* value = NextTok(arg1, textEnd);
    if (TokenMatches(arg1, textEnd, "fadeTimeout")) {
        int32_t v = 1000*Macros_ParseInt(value, textEnd, NULL);
        if (Macros_DryRun) {
            return;
        }
        LedsFadeTimeout = v;
    } else if (TokenMatches(arg1, textEnd, "brightness")) {
        float v = ParseFloat(value, textEnd);
        if (Macros_DryRun) {
            return;
        }
        LedBrightnessMultiplier = v;
    } else if (TokenMatches(arg1, textEnd, "enabled")) {
        bool v = Macros_ParseBoolean(value, textEnd);
        if (Macros_DryRun) {
            return;
        }
        LedsEnabled = v;
    } else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }

    LedSlaveDriver_UpdateLeds();
}

static void backlight(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "strategy")) {
        backlightStrategy(NextTok(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "constantRgb")) {
        constantRgb(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "keyRgb")) {
        keyRgb(proceedByDot(arg1, textEnd), textEnd);
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
}

static key_action_t parseKeyAction(const char* arg1, const char *textEnd) {
    const char* arg2 = NextTok(arg1, textEnd);

    key_action_t action = { .type = KeyActionType_None };

    if (TokenMatches(arg1, textEnd, "macro")) {
        uint8_t macroIndex = FindMacroIndexByName(arg2, TokEnd(arg2, textEnd), true);

        action.type = KeyActionType_PlayMacro;
        action.playMacro.macroId = macroIndex;
    }
    else if (TokenMatches(arg1, textEnd, "keystroke")) {
        MacroShortcutParser_Parse(arg2, TokEnd(arg2, textEnd), MacroSubAction_Press, NULL, &action);
    }
    else if (TokenMatches(arg1, textEnd, "none")) {
        action.type = KeyActionType_None;
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }

    return action;
}


static void navigationModeAction(const char* arg1, const char *textEnd)
{
    navigation_mode_t navigationMode = NavigationMode_Caret;
    bool positive = true;
    caret_axis_t axis = CaretAxis_Horizontal;

    const char* arg2 = proceedByDot(arg1, textEnd);
    const char* arg3 = NextTok(arg2, textEnd);

    navigationMode = ParseNavigationModeId(arg1, textEnd);

    if(navigationMode < NavigationMode_RemappableFirst || navigationMode > NavigationMode_RemappableLast) {
        Macros_ReportError("Invalid or non-remapable navigation mode:", arg1, textEnd);
    }

    if (Macros_ParserError) {
        return;
    }

    if (TokenMatches(arg2, textEnd, "left")) {
        axis = CaretAxis_Horizontal;
        positive = false;
    }
    else if (TokenMatches(arg2, textEnd, "up")) {
        axis = CaretAxis_Vertical;
        positive = true;
    }
    else if (TokenMatches(arg2, textEnd, "right")) {
        axis = CaretAxis_Horizontal;
        positive = true;
    }
    else if (TokenMatches(arg2, textEnd, "down")) {
        axis = CaretAxis_Vertical;
        positive = false;
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }

    key_action_t action = parseKeyAction(arg3, textEnd);

    if (Macros_ParserError) {
        return;
    }
    if (Macros_DryRun) {
        return;
    }

    SetModuleCaretConfiguration(navigationMode, axis, positive, action);
}

static void keymapAction(const char* arg1, const char *textEnd)
{
    const char* arg2 = proceedByDot(arg1, textEnd);
    const char* arg3 = NextTok(arg2, textEnd);

    uint8_t layerId = Macros_ParseLayerId(arg1, textEnd);
    uint16_t keyId = Macros_ParseInt(arg2, textEnd, NULL);

    key_action_t action = parseKeyAction(arg3, textEnd);

    uint8_t slotIdx = keyId/64;
    uint8_t inSlotIdx = keyId%64;

    if(slotIdx > SLOT_COUNT || inSlotIdx > MAX_KEY_COUNT_PER_MODULE) {
        Macros_ReportError("Invalid key id:", arg2, textEnd);
    }

    if (Macros_ParserError) {
        return;
    }
    if (Macros_DryRun) {
        return;
    }

    key_action_t* actionSlot = &CurrentKeymap[layerId][slotIdx][inSlotIdx];

    *actionSlot = action;
}

static void modLayerTriggers(const char* arg1, const char *textEnd)
{
    const char* specifier = NextTok(arg1, textEnd);
    uint8_t layerId = Macros_ParseLayerId(arg1, textEnd);
    uint8_t left = 0;
    uint8_t right = 0;

    switch (layerId) {
    case LayerId_Shift:
        left = HID_KEYBOARD_MODIFIER_LEFTSHIFT ;
        right = HID_KEYBOARD_MODIFIER_RIGHTSHIFT ;
        break;
    case LayerId_Ctrl:
        left = HID_KEYBOARD_MODIFIER_LEFTCTRL ;
        right = HID_KEYBOARD_MODIFIER_RIGHTCTRL ;
        break;
    case LayerId_Alt:
        left = HID_KEYBOARD_MODIFIER_LEFTALT ;
        right = HID_KEYBOARD_MODIFIER_RIGHTALT ;
        break;
    case LayerId_Gui:
        left = HID_KEYBOARD_MODIFIER_LEFTGUI ;
        right = HID_KEYBOARD_MODIFIER_RIGHTGUI ;
        break;
    default:
        Macros_ReportError("This layer does not allow modifier triggers:", arg1, specifier);
        return;
    }

    if (Macros_ParserError) {
        return;
    }
    if (Macros_DryRun) {
        return;
    }

    if (TokenMatches(specifier, textEnd, "left")) {
        LayerConfig[layerId].modifierLayerMask = left;
    }
    else if (TokenMatches(specifier, textEnd, "right")) {
        LayerConfig[layerId].modifierLayerMask = right;
    }
    else if (TokenMatches(specifier, textEnd, "both")) {
        LayerConfig[layerId].modifierLayerMask = left | right;
    }
    else {
        Macros_ReportError("Specifier not recognized:", specifier, textEnd);
    }
}


macro_result_t Macro_ProcessSetCommand(const char* arg1, const char *textEnd)
{
    const char* arg2 = NextTok(arg1, textEnd);

    if (TokenMatches(arg1, textEnd, "module")) {
        module(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "secondaryRole")) {
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
    else if (TokenMatches(arg1, textEnd, "backlight")) {
        backlight(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "leds")) {
        leds(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "modifierLayerTriggers")) {
        modLayerTriggers(proceedByDot(arg1, textEnd), textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "diagonalSpeedCompensation")) {
        bool v = Macros_ParseBoolean(arg2, textEnd);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        DiagonalSpeedCompensation = v;
    }
    else if (TokenMatches(arg1, textEnd, "stickyModifiers")) {
        stickyModifiers(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "debounceDelay")) {
        uint16_t time = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        DebounceTimePress = time;
        DebounceTimeRelease = time;
    }
    else if (TokenMatches(arg1, textEnd, "keystrokeDelay")) {
        uint32_t d = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        KeystrokeDelay = d;
    }
    else if (
            TokenMatches(arg1, textEnd, "doubletapTimeout")  // new name
            || (TokenMatches(arg1, textEnd, "doubletapDelay")) // deprecated alias - old name
            ) {
        uint16_t delay = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        DoubleTapSwitchLayerTimeout = delay;
        DoubletapConditionTimeout = delay;
    }
    else if (TokenMatches(arg1, textEnd, "autoRepeatDelay")) {
        uint16_t delay = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        AutoRepeatInitialDelay = delay;
    }
    else if (TokenMatches(arg1, textEnd, "autoRepeatRate")) {
        uint16_t delay = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        AutoRepeatDelayRate = delay;
    }
    else if (TokenMatches(arg1, textEnd, "chordingDelay")) {
        uint32_t d = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        ChordingDelay = d;
    }
    else if (TokenMatches(arg1, textEnd, "autoShiftDelay")) {
        uint32_t d = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        AutoShiftDelay = d;
    }
    else if (TokenMatches(arg1, textEnd, "i2cBaudRate")) {
        uint32_t baudRate = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        ChangeI2cBaudRate(baudRate);
    }
    else if (TokenMatches(arg1, textEnd, "emergencyKey")) {
        uint16_t key = Macros_ParseInt(arg2, textEnd, NULL);
        if (Macros_DryRun) {
            return MacroResult_Finished;
        }
        EmergencyKey = Utils_KeyIdToKeyState(key);
    }
    else {
        Macros_ReportError("Parameter not recognized:", arg1, textEnd);
    }
    return MacroResult_Finished;
}
