#include "macro_set_command.h"
#include "config_parser/parse_config.h"
#include "layer.h"
#include "ledmap.h"
#include "macros.h"
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

    if (IS_MODIFIER_LAYER(layerId)) {
        Macros_ReportError("Navigation mode cannot be changed for modifier layers!", NULL, NULL);
        return;
    }

    if (Macros_ParserError) {
        return;
    }

    module->navigationModes[layerId] = modeId;
}

static void moduleSpeed(const char* arg1, const char *textEnd, module_configuration_t* module, uint8_t moduleId)
{
    const char* arg2 = NextTok(arg1, textEnd);

    if (TokenMatches(arg1, textEnd, "baseSpeed")) {
        module->baseSpeed = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "speed")) {
        module->speed = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "xceleration")) {
        module->xceleration = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "caretSpeedDivisor")) {
        module->caretSpeedDivisor = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "scrollSpeedDivisor")) {
        module->scrollSpeedDivisor = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "pinchZoomDivisor") && moduleId == ModuleId_TouchpadRight) {
        module->pinchZoomSpeedDivisor = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "pinchZoomMode") && moduleId == ModuleId_TouchpadRight) {
        TouchpadPinchZoomMode = ParseNavigationModeId(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "axisLockSkew")) {
        module->axisLockSkew = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "axisLockFirstTickSkew")) {
        module->axisLockFirstTickSkew = ParseFloat(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "cursorAxisLock")) {
        module->cursorAxisLock = Macros_ParseBoolean(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "scrollAxisLock")) {
        module->scrollAxisLock = Macros_ParseBoolean(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "caretAxisLock")) {
        module->caretAxisLock = Macros_ParseBoolean(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "swapAxes")) {
        module->swapAxes = Macros_ParseBoolean(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "invertScrollDirection")) {
        module->invertScrollDirection = Macros_ParseBoolean(arg2, textEnd);
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
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
        SecondaryRoles_AdvancedStrategyTimeout = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else if (TokenMatches(arg1, textEnd, "timeoutAction")) {
        if (TokenMatches(arg2, textEnd, "primary")) {
            SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Primary;
        }
        else if (TokenMatches(arg2, textEnd, "secondary")) {
            SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Secondary;
        }
        else {
            Macros_ReportError("parameter not recognized:", arg2, textEnd);
        }
    }
    else if (TokenMatches(arg1, textEnd, "safetyMargin")) {
        SecondaryRoles_AdvancedStrategySafetyMargin = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else if (TokenMatches(arg1, textEnd, "triggerByRelease")) {
        SecondaryRoles_AdvancedStrategyTriggerByRelease = Macros_ParseBoolean(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "doubletapToPrimary")) {
        SecondaryRoles_AdvancedStrategyDoubletapToPrimary = Macros_ParseBoolean(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "doubletapTime")) {
        SecondaryRoles_AdvancedStrategyDoubletapTime = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
}

static void secondaryRoles(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "defaultStrategy")) {
        const char* arg2 = NextTok(arg1, textEnd);
        if (TokenMatches(arg2, textEnd, "simple")) {
            SecondaryRoles_Strategy = SecondaryRoleStrategy_Simple;
        }
        else if (TokenMatches(arg2, textEnd, "advanced")) {
            SecondaryRoles_Strategy = SecondaryRoleStrategy_Advanced;
        }
        else {
            Macros_ReportError("parameter not recognized:", arg2, textEnd);
        }
    }
    else if (TokenMatches(arg1, textEnd, "advanced")) {
        secondaryRoleAdvanced(proceedByDot(arg1, textEnd), textEnd);
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
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
        Macros_ReportError("scroll or move expected", NULL, NULL);
    }

    const char* arg2 = proceedByDot(arg1, textEnd);
    const char* arg3 = NextTok(arg2, textEnd);

    if (TokenMatches(arg2, textEnd, "initialSpeed")) {
        state->initialSpeed = Macros_ParseInt(arg3, textEnd, NULL) / state->intMultiplier;
    }
    else if (TokenMatches(arg2, textEnd, "baseSpeed")) {
        state->baseSpeed = Macros_ParseInt(arg3, textEnd, NULL) / state->intMultiplier;
    }
    else if (TokenMatches(arg2, textEnd, "initialAcceleration")) {
        state->acceleration = Macros_ParseInt(arg3, textEnd, NULL) / state->intMultiplier;
    }
    else if (TokenMatches(arg2, textEnd, "deceleratedSpeed")) {
        state->deceleratedSpeed = Macros_ParseInt(arg3, textEnd, NULL) / state->intMultiplier;
    }
    else if (TokenMatches(arg2, textEnd, "acceleratedSpeed")) {
        state->acceleratedSpeed = Macros_ParseInt(arg3, textEnd, NULL) / state->intMultiplier;
    }
    else if (TokenMatches(arg2, textEnd, "axisSkew")) {
        state->axisSkew = ParseFloat(arg3, textEnd);
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
}

static void stickyModifiers(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "never")) {
        StickyModifierStrategy = Stick_Never;
    }
    else if (TokenMatches(arg1, textEnd, "smart")) {
        StickyModifierStrategy = Stick_Smart;
    }
    else if (TokenMatches(arg1, textEnd, "always")) {
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
    else if (TokenMatches(arg1, textEnd, "extendedCommands")) {
        /* this option was removed -> accept the command & do nothing */
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
}

static void backlightStrategy(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "functional")) {
        SetLedBacklightingMode(BacklightingMode_Functional);
        LedSlaveDriver_UpdateLeds();
    }
    else if (TokenMatches(arg1, textEnd, "constantRgb")) {
        SetLedBacklightingMode(BacklightingMode_ConstantRGB);
        LedSlaveDriver_UpdateLeds();
    }
    else if (TokenMatches(arg1, textEnd, "perKeyRgb")) {
        if (PerKeyDataPresent) {
            SetLedBacklightingMode(BacklightingMode_PerKeyRgb);
            LedSlaveDriver_UpdateLeds();
        } else {
            Macros_ReportError("Cannot set perKeyRgb mode when perKeyRgb maps are not available. Please, consult Agent's led section...", NULL, NULL);
        }
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
}

static void constantRgb(const char* arg1, const char *textEnd)
{
    if (TokenMatches(arg1, textEnd, "rgb")) {
        const char* r = NextTok(arg1,  textEnd);
        const char* g = NextTok(r, textEnd);
        const char* b = NextTok(g, textEnd);
        LedMap_ConstantRGB.red = Macros_ParseInt(r, textEnd, NULL);
        LedMap_ConstantRGB.green = Macros_ParseInt(g, textEnd, NULL);
        LedMap_ConstantRGB.blue = Macros_ParseInt(b, textEnd, NULL);
        SetLedBacklightingMode(BacklightingMode_ConstantRGB);
        LedSlaveDriver_UpdateLeds();
    }
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }
}

static void leds(const char* arg1, const char *textEnd)
{
    const char* value = NextTok(arg1, textEnd);
    if (TokenMatches(arg1, textEnd, "fadeTimeout")) {
        LedSleepTimeout = 1000*Macros_ParseInt(value, textEnd, NULL);
    } else if (TokenMatches(arg1, textEnd, "brightness")) {
        LedBrightnessMultiplier = ParseFloat(value, textEnd);
    } else if (TokenMatches(arg1, textEnd, "enabled")) {
        LedsEnabled = Macros_ParseBoolean(value, textEnd);
    } else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
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
    else {
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
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
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
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
        Macros_ReportError("Invalid or non-remapable navigation mode", arg1, textEnd);
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
        Macros_ReportError("parameter not recognized:", arg1, textEnd);
    }

    key_action_t action = parseKeyAction(arg3, textEnd);

    if (Macros_ParserError) {
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
        Macros_ReportError("invalid key id:", arg2, textEnd);
    }

    if (Macros_ParserError) {
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
        Macros_ReportError("This layer does not allow modifier triggers.", arg1, specifier);
        return;
    }

    if (Macros_ParserError) {
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
        Macros_ReportError("Specifier not recognized", specifier, textEnd);
    }
}


macro_result_t MacroSetCommand(const char* arg1, const char *textEnd)
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
        DiagonalSpeedCompensation = Macros_ParseBoolean(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "stickyModifiers")) {
        stickyModifiers(arg2, textEnd);
    }
    else if (TokenMatches(arg1, textEnd, "debounceDelay")) {
        uint16_t time = Macros_ParseInt(arg2, textEnd, NULL);
        DebounceTimePress = time;
        DebounceTimeRelease = time;
    }
    else if (TokenMatches(arg1, textEnd, "keystrokeDelay")) {
        KeystrokeDelay = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else if (
            TokenMatches(arg1, textEnd, "doubletapTimeout")  // new name
            || (TokenMatches(arg1, textEnd, "doubletapDelay")) // deprecated alias - old name
            ) {
        uint16_t delay = Macros_ParseInt(arg2, textEnd, NULL);
        DoubleTapSwitchLayerTimeout = delay;
        DoubletapConditionTimeout = delay;
    }
    else if (TokenMatches(arg1, textEnd, "autoRepeatDelay")) {
        uint16_t delay = Macros_ParseInt(arg2, textEnd, NULL);
        AutoRepeatInitialDelay = delay;
    }
    else if (TokenMatches(arg1, textEnd, "autoRepeatRate")) {
        uint16_t delay = Macros_ParseInt(arg2, textEnd, NULL);
        AutoRepeatDelayRate = delay;
    }
    else if (TokenMatches(arg1, textEnd, "chordingDelay")) {
        ChordingDelay = Macros_ParseInt(arg2, textEnd, NULL);
    }
    else if (TokenMatches(arg1, textEnd, "i2cBaudRate")) {
        uint32_t baudRate = Macros_ParseInt(arg2, textEnd, NULL);
        ChangeI2cBaudRate(baudRate);
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
