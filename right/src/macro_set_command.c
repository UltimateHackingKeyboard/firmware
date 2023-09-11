#include "macro_set_command.h"
#include "config_parser/parse_config.h"
#include "config_parser/parse_keymap.h"
#include "key_states.h"
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
#include "mouse_keys.h"
#include "debug.h"
#include "caret_config.h"
#include "config_parser/parse_macro.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "init_peripherals.h"
#include "macros_vars.h"
#include <stdint.h>

typedef enum {
    SetCommandAction_Write,
    SetCommandAction_Read,
} set_command_action_t;

#define ASSIGN_CUSTOM5(TPE, RET, RETPARAM, DST, SRC)           \
    if (action == SetCommandAction_Read) {                     \
        return RET(RETPARAM);                                  \
    }                                                          \
    TPE res = SRC;                                             \
    if (Macros_ParserError || Macros_DryRun) {                 \
        return noneVar();                                      \
    }                                                          \
    DST = res;                                                 \

#define ASSIGN_INT_MUL(DST, M)                                 \
    if (action == SetCommandAction_Read) {                     \
        return intVar(DST/(M));                                \
    }                                                          \
    uint32_t res = Macros_LegacyConsumeInt(ctx)*(M);           \
    if (Macros_ParserError || Macros_DryRun) {                 \
        return noneVar();                                      \
    }                                                          \
    DST = res;                                                 \

#define ASSIGN_INT2(DST, DST2)                                 \
    if (action == SetCommandAction_Read) {                     \
        return intVar(DST);                                    \
    }                                                          \
    uint32_t res = Macros_LegacyConsumeInt(ctx);               \
    if (Macros_ParserError || Macros_DryRun) {                 \
        return noneVar();                                      \
    }                                                          \
    DST = res;                                                 \
    DST2 = res;                                                \

#define ASSIGN_CUSTOM(TPE, RET, DST, SRC) ASSIGN_CUSTOM5(TPE, RET, DST, DST, SRC)
#define ASSIGN_ENUM(DST, SRC) ASSIGN_CUSTOM(uint8_t, intVar, DST, SRC)
#define ASSIGN_FLOAT(DST) ASSIGN_CUSTOM(float, floatVar, DST, Macros_ConsumeFloat(ctx))
#define ASSIGN_BOOL(DST) ASSIGN_CUSTOM(bool, boolVar, DST, Macros_ConsumeBool(ctx))
#define ASSIGN_INT(DST) ASSIGN_CUSTOM(int32_t, intVar, DST, Macros_ConsumeInt(ctx))

static macro_variable_t noneVar()
{
    return (macro_variable_t) { .asInt = 1, .type = MacroVariableType_None };
}

static macro_variable_t intVar(int32_t value)
{
    return (macro_variable_t) { .asInt = value, .type = MacroVariableType_Int };
}

static macro_variable_t floatVar(float value)
{
    return (macro_variable_t) { .asFloat = value, .type = MacroVariableType_Float };
}

static macro_variable_t boolVar(bool value)
{
    return (macro_variable_t) { .asBool = value, .type = MacroVariableType_Bool };
}

static macro_variable_t moduleNavigationMode(parser_context_t* ctx, set_command_action_t action, module_configuration_t* module)
{
    parser_context_t layerCtx = *ctx;
    layer_id_t layerId = Macros_ConsumeLayerId(ctx);

    if (action == SetCommandAction_Read) {
        return intVar(module->navigationModes[layerId]);
    }

    navigation_mode_t modeId = ConsumeNavigationModeId(ctx);

    if (IS_MODIFIER_LAYER(layerId)) {
        Macros_ReportError("Navigation mode cannot be changed for modifier layers!", layerCtx.at, layerCtx.at);
        return noneVar();
    }

    if (Macros_ParserError) {
        return noneVar();
    }
    if (Macros_DryRun) {
        return noneVar();
    }

    module->navigationModes[layerId] = modeId;
    return noneVar();
}

static macro_variable_t moduleSpeed(parser_context_t* ctx, set_command_action_t action, module_configuration_t* module, uint8_t moduleId)
{
    if (ConsumeToken(ctx, "baseSpeed")) {
        ASSIGN_FLOAT(module->baseSpeed);
    }
    else if (ConsumeToken(ctx, "speed")) {
        ASSIGN_FLOAT(module->speed);
    }
    else if (ConsumeToken(ctx, "xceleration")) {
        ASSIGN_FLOAT(module->xceleration);
    }
    else if (ConsumeToken(ctx, "caretSpeedDivisor")) {
        ASSIGN_FLOAT(module->caretSpeedDivisor);
    }
    else if (ConsumeToken(ctx, "scrollSpeedDivisor")) {
        ASSIGN_FLOAT(module->scrollSpeedDivisor);
    }
    else if (ConsumeToken(ctx, "pinchZoomDivisor") && moduleId == ModuleId_TouchpadRight) {
        ASSIGN_FLOAT(module->pinchZoomSpeedDivisor);
    }
    else if (ConsumeToken(ctx, "pinchZoomMode") && moduleId == ModuleId_TouchpadRight) {
        ASSIGN_CUSTOM(int32_t, intVar, TouchpadPinchZoomMode, ConsumeNavigationModeId(ctx));
    }
    else if (ConsumeToken(ctx, "axisLockSkew")) {
        ASSIGN_FLOAT(module->axisLockSkew);
    }
    else if (ConsumeToken(ctx, "axisLockFirstTickSkew")) {
        ASSIGN_FLOAT(module->axisLockFirstTickSkew);
    }
    else if (ConsumeToken(ctx, "cursorAxisLock")) {
        ASSIGN_BOOL(module->cursorAxisLock);
    }
    else if (ConsumeToken(ctx, "scrollAxisLock")) {
        ASSIGN_BOOL(module->scrollAxisLock);
    }
    else if (ConsumeToken(ctx, "caretAxisLock")) {
        ASSIGN_BOOL(module->caretAxisLock);
    }
    else if (ConsumeToken(ctx, "swapAxes")) {
        ASSIGN_BOOL(module->swapAxes);
    }
    else if (ConsumeToken(ctx, "invertScrollDirection")) {
        Macros_ReportWarn("Command deprecated. Please, replace invertScrollDirection by invertScrollDirectionY.", ConsumedToken(ctx), ConsumedToken(ctx));
        ASSIGN_BOOL(module->invertScrollDirectionY);
    }
    else if (ConsumeToken(ctx, "invertScrollDirectionY")) {
        ASSIGN_BOOL(module->invertScrollDirectionY);
    }
    else if (ConsumeToken(ctx, "invertScrollDirectionX")) {
        ASSIGN_BOOL(module->invertScrollDirectionX);
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static macro_variable_t module(parser_context_t* ctx, set_command_action_t action)
{
    module_id_t moduleId = ConsumeModuleId(ctx);
    module_configuration_t* module = GetModuleConfiguration(moduleId);

    ConsumeUntilDot(ctx);

    if (Macros_ParserError) {
        return noneVar();
    }

    if (ConsumeToken(ctx, "navigationMode")) {
        ConsumeUntilDot(ctx);
        return moduleNavigationMode(ctx, action, module);
    }
    else {
        return moduleSpeed(ctx, action, module, moduleId);
    }
}

static macro_variable_t secondaryRoleAdvanced(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "timeout")) {
        ASSIGN_INT(SecondaryRoles_AdvancedStrategyTimeout);
    }
    else if (ConsumeToken(ctx, "timeoutAction")) {
        ASSIGN_CUSTOM(int32_t, intVar, SecondaryRoles_AdvancedStrategyTimeoutAction, ConsumeSecondaryRoleTimeoutAction(ctx));
    }
    else if (ConsumeToken(ctx, "safetyMargin")) {
        ASSIGN_INT(SecondaryRoles_AdvancedStrategySafetyMargin);
    }
    else if (ConsumeToken(ctx, "triggerByRelease")) {
        ASSIGN_BOOL(SecondaryRoles_AdvancedStrategyTriggerByRelease);
    }
    else if (ConsumeToken(ctx, "doubletapToPrimary")) {
        ASSIGN_BOOL(SecondaryRoles_AdvancedStrategyDoubletapToPrimary);
    }
    else if (ConsumeToken(ctx, "doubletapTime")) {
        ASSIGN_INT(SecondaryRoles_AdvancedStrategyDoubletapTime);
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static macro_variable_t secondaryRoles(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "defaultStrategy")) {
        ASSIGN_CUSTOM(int32_t, intVar, SecondaryRoles_Strategy, ConsumeSecondaryRoleStrategy(ctx));
    }
    else if (ConsumeToken(ctx, "advanced")) {
        ConsumeUntilDot(ctx);
        return secondaryRoleAdvanced(ctx, action);
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static macro_variable_t mouseKeys(parser_context_t* ctx, set_command_action_t action)
{
    mouse_kinetic_state_t* state = &MouseMoveState;

    if (ConsumeToken(ctx, "move")) {
        state = &MouseMoveState;
    } else if (ConsumeToken(ctx, "scroll")) {
        state = &MouseScrollState;
    } else {
        Macros_ReportError("Scroll or move expected!", ctx->at, ctx->at);
        return noneVar();
    }

    ConsumeUntilDot(ctx);

    if (ConsumeToken(ctx, "initialSpeed")) {
        ASSIGN_INT_MUL(state->initialSpeed, 1.0f/state->intMultiplier);
    }
    else if (ConsumeToken(ctx, "baseSpeed")) {
        ASSIGN_INT_MUL(state->baseSpeed, 1.0f/state->intMultiplier);
    }
    else if (ConsumeToken(ctx, "initialAcceleration")) {
        ASSIGN_INT_MUL(state->acceleration, 1.0f/state->intMultiplier);
    }
    else if (ConsumeToken(ctx, "deceleratedSpeed")) {
        ASSIGN_INT_MUL(state->deceleratedSpeed, 1.0f/state->intMultiplier);
    }
    else if (ConsumeToken(ctx, "acceleratedSpeed")) {
        ASSIGN_INT_MUL(state->acceleratedSpeed, 1.0f/state->intMultiplier);
    }
    else if (ConsumeToken(ctx, "axisSkew")) {
        ASSIGN_FLOAT(state->axisSkew);
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static macro_variable_t stickyModifiers(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "never")) {
        ASSIGN_ENUM(StickyModifierStrategy, Stick_Never);
    }
    else if (ConsumeToken(ctx, "smart")) {
        ASSIGN_ENUM(StickyModifierStrategy, Stick_Smart);
    }
    else if (ConsumeToken(ctx, "always")) {
        ASSIGN_ENUM(StickyModifierStrategy, Stick_Always);
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static macro_variable_t macroEngineScheduler(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "preemptive")) {
        ASSIGN_ENUM(Macros_Scheduler, Scheduler_Preemptive);
    }
    else if (ConsumeToken(ctx, "blocking")) {
        ASSIGN_ENUM(Macros_Scheduler, Scheduler_Blocking);
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static macro_variable_t macroEngine(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "scheduler")) {
        return macroEngineScheduler(ctx, action);
    }
    else if (ConsumeToken(ctx, "batchSize")) {
        ASSIGN_INT(Macros_MaxBatchSize);
    }
    else if (ConsumeToken(ctx, "extendedCommands")) {
        Macros_LegacyConsumeInt(ctx);
        /* this option was removed -> accept the command & do nothing */
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static macro_variable_t backlightStrategy(parser_context_t* ctx, set_command_action_t action)
{
    if (action == SetCommandAction_Read) {
        return intVar(BacklightingMode);
    }

    backlighting_mode_t res = 0;

    if (ConsumeToken(ctx, "functional")) {
        res = BacklightingMode_Functional;
    }
    else if (ConsumeToken(ctx, "constantRgb")) {
        res = BacklightingMode_ConstantRGB;
    }
    else if (ConsumeToken(ctx, "perKeyRgb")) {
        if (PerKeyRgbPresent) {
            res = BacklightingMode_PerKeyRgb;
        } else {
            Macros_ReportError("Cannot set perKeyRgb mode when perKeyRgb maps are not available. Please, consult Agent's led section...", ConsumedToken(ctx), ConsumedToken(ctx));
        }
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }

    if (Macros_ParserError) {
        return noneVar();
    }
    if (Macros_DryRun) {
        return noneVar();
    }

    Ledmap_SetLedBacklightingMode(res);
    Ledmap_UpdateBacklightLeds();
    return noneVar();
}

static macro_variable_t keyRgb(parser_context_t* ctx, set_command_action_t action)
{
    layer_id_t layerId = Macros_ConsumeLayerId(ctx);

    ConsumeUntilDot(ctx);

    uint8_t keyId = Macros_LegacyConsumeInt(ctx);

    if (action == SetCommandAction_Read) {
        Macros_ReportError("Reading RGB values not supported!", ConsumedToken(ctx), ConsumedToken(ctx));
        return noneVar();
    }

    rgb_t rgb;
    rgb.red = Macros_LegacyConsumeInt(ctx);
    rgb.green = Macros_LegacyConsumeInt(ctx);
    rgb.blue = Macros_LegacyConsumeInt(ctx);

    if (Macros_ParserError) {
        return noneVar();
    }
    if (Macros_DryRun) {
        return noneVar();
    }

    uint8_t slotIdx = keyId/64;
    uint8_t inSlotIdx = keyId%64;

    CurrentKeymap[layerId][slotIdx][inSlotIdx].colorOverridden = true;
    CurrentKeymap[layerId][slotIdx][inSlotIdx].color = rgb;

    Ledmap_UpdateBacklightLeds();
    return noneVar();
}


static macro_variable_t constantRgb(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "rgb")) {
        if (action == SetCommandAction_Read) {
            Macros_ReportError("Reading RGB values not supported!", ConsumedToken(ctx), ConsumedToken(ctx));
            return noneVar();
        }

        rgb_t rgb;
        rgb.red = Macros_LegacyConsumeInt(ctx);
        rgb.green = Macros_LegacyConsumeInt(ctx);
        rgb.blue = Macros_LegacyConsumeInt(ctx);

        if (Macros_DryRun) {
            return noneVar();
        }

        LedMap_ConstantRGB = rgb;
        Ledmap_SetLedBacklightingMode(BacklightingMode_ConstantRGB);
        Ledmap_UpdateBacklightLeds();

        return noneVar();
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
        return noneVar();
    }
}

static macro_variable_t leds(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "fadeTimeout")) {
        ASSIGN_INT_MUL(LedsFadeTimeout, 1000);
    } else if (ConsumeToken(ctx, "brightness")) {
        ASSIGN_FLOAT(LedBrightnessMultiplier);
    } else if (ConsumeToken(ctx, "enabled")) {
        ASSIGN_BOOL(LedsEnabled);
    } else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }

    LedSlaveDriver_UpdateLeds();
    return noneVar();
}

static macro_variable_t backlight(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "strategy")) {
        return backlightStrategy(ctx, action);
    }
    else if (ConsumeToken(ctx, "constantRgb")) {
        ConsumeUntilDot(ctx);
        return constantRgb(ctx, action);
    }
    else if (ConsumeToken(ctx, "keyRgb")) {
        ConsumeUntilDot(ctx);
        return keyRgb(ctx, action);
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static key_action_t parseKeyAction(parser_context_t* ctx)
{
    key_action_t action = { .type = KeyActionType_None };

    if (ConsumeToken(ctx, "macro")) {
        const char* end = TokEnd(ctx->at, ctx->end);
        uint8_t macroIndex = FindMacroIndexByName(ctx->at, end, true);

        action.type = KeyActionType_PlayMacro;
        action.playMacro.macroId = macroIndex;

        ctx->at = end;
        ConsumeWhite(ctx);
    }
    else if (ConsumeToken(ctx, "keystroke")) {
        const char* end = TokEnd(ctx->at, ctx->end);
        MacroShortcutParser_Parse(ctx->at, end, MacroSubAction_Press, NULL, &action);

        ctx->at = end;
        ConsumeWhite(ctx);
    }
    else if (ConsumeToken(ctx, "none")) {
        action.type = KeyActionType_None;
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }

    return action;
}


static macro_variable_t navigationModeAction(parser_context_t* ctx, set_command_action_t action)
{
    navigation_mode_t navigationMode = NavigationMode_Caret;
    bool positive = true;
    caret_axis_t axis = CaretAxis_Horizontal;

    navigationMode = ConsumeNavigationModeId(ctx);

    ConsumeUntilDot(ctx);

    if (action == SetCommandAction_Read) {
        Macros_ReportError("Reading actions is not supported!", ctx->at, ctx->at);
        return noneVar();
    }

    if(navigationMode < NavigationMode_RemappableFirst || navigationMode > NavigationMode_RemappableLast) {
        Macros_ReportError("Invalid or non-remapable navigation mode:", ConsumedToken(ctx), ConsumedToken(ctx));
    }

    if (Macros_ParserError) {
        return noneVar();
    }

    if (ConsumeToken(ctx, "left")) {
        axis = CaretAxis_Horizontal;
        positive = false;
    }
    else if (ConsumeToken(ctx, "up")) {
        axis = CaretAxis_Vertical;
        positive = true;
    }
    else if (ConsumeToken(ctx, "right")) {
        axis = CaretAxis_Horizontal;
        positive = true;
    }
    else if (ConsumeToken(ctx, "down")) {
        axis = CaretAxis_Vertical;
        positive = false;
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
        return noneVar();
    }

    key_action_t keyAction = parseKeyAction(ctx);

    if (Macros_ParserError) {
        return noneVar();
    }
    if (Macros_DryRun) {
        return noneVar();
    }

    SetModuleCaretConfiguration(navigationMode, axis, positive, keyAction);
    return noneVar();
}

static macro_variable_t keymapAction(parser_context_t* ctx, set_command_action_t action)
{
    uint8_t layerId = Macros_ConsumeLayerId(ctx);
    parser_context_t keyIdPos = *ctx;

    ConsumeUntilDot(ctx);

    uint16_t keyId = Macros_LegacyConsumeInt(ctx);

    if (action == SetCommandAction_Read) {
        Macros_ReportError("Reading actions is not supported!", ctx->at, ctx->at);
        return noneVar();
    }

    key_action_t keyAction = parseKeyAction(ctx);

    uint8_t slotIdx = keyId/64;
    uint8_t inSlotIdx = keyId%64;

    if(slotIdx > SLOT_COUNT || inSlotIdx > MAX_KEY_COUNT_PER_MODULE) {
        Macros_ReportErrorNum("Invalid key id:", keyId, keyIdPos.at);
    }

    if (Macros_ParserError) {
        return noneVar();
    }
    if (Macros_DryRun) {
        return noneVar();
    }

    key_action_t* actionSlot = &CurrentKeymap[layerId][slotIdx][inSlotIdx];

    *actionSlot = keyAction;
    return noneVar();
}

static macro_variable_t modLayerTriggers(parser_context_t* ctx, set_command_action_t action)
{
    uint8_t layerId = Macros_ConsumeLayerId(ctx);
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
        Macros_ReportError("This layer does not allow modifier triggers:", ConsumedToken(ctx), ConsumedToken(ctx));
        return noneVar();
    }

    if (action == SetCommandAction_Read) {
        return intVar(LayerConfig[layerId].modifierLayerMask);
    }

    uint8_t mask = 0;

    if (ConsumeToken(ctx, "left")) {
        mask = left;
    }
    else if (ConsumeToken(ctx, "right")) {
        mask = right;
    }
    else if (ConsumeToken(ctx, "both")) {
        mask = left | right;
    }
    else {
        Macros_ReportError("Specifier not recognized:", ctx->at, ctx->end);
    }

    if (Macros_ParserError) {
        return noneVar();
    }
    if (Macros_DryRun) {
        return noneVar();
    }

    LayerConfig[layerId].modifierLayerMask = mask;
    return noneVar();
}


static macro_variable_t root(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "module")) {
        ConsumeUntilDot(ctx);
        return module(ctx, action);
    }
    else if (ConsumeToken(ctx, "secondaryRole")) {
        ConsumeUntilDot(ctx);
        return secondaryRoles(ctx, action);
    }
    else if (ConsumeToken(ctx, "mouseKeys")) {
        ConsumeUntilDot(ctx);
        return mouseKeys(ctx, action);
    }
    else if (ConsumeToken(ctx, "keymapAction")) {
        ConsumeUntilDot(ctx);
        return keymapAction(ctx, action);
    }
    else if (ConsumeToken(ctx, "navigationModeAction")) {
        ConsumeUntilDot(ctx);
        return navigationModeAction(ctx, action);
    }
    else if (ConsumeToken(ctx, "macroEngine")) {
        ConsumeUntilDot(ctx);
        return macroEngine(ctx, action);
    }
    else if (ConsumeToken(ctx, "backlight")) {
        ConsumeUntilDot(ctx);
        return backlight(ctx, action);
    }
    else if (ConsumeToken(ctx, "leds")) {
        ConsumeUntilDot(ctx);
        return leds(ctx, action);
    }
    else if (ConsumeToken(ctx, "modifierLayerTriggers")) {
        ConsumeUntilDot(ctx);
        return modLayerTriggers(ctx, action);
    }
    else if (ConsumeToken(ctx, "diagonalSpeedCompensation")) {
        ASSIGN_BOOL(DiagonalSpeedCompensation);
    }
    else if (ConsumeToken(ctx, "stickyModifiers")) {
        return stickyModifiers(ctx, action);
    }
    else if (ConsumeToken(ctx, "debounceDelay")) {
        ASSIGN_INT2(DebounceTimePress, DebounceTimeRelease);
    }
    else if (ConsumeToken(ctx, "keystrokeDelay")) {
        ASSIGN_INT(KeystrokeDelay);
    }
    else if (
            ConsumeToken(ctx, "doubletapTimeout")  // new name
            || (ConsumeToken(ctx, "doubletapDelay")) // deprecated alias - old name
            ) {
        ASSIGN_INT2(DoubleTapSwitchLayerTimeout, DoubletapConditionTimeout);
    }
    else if (ConsumeToken(ctx, "autoRepeatDelay")) {
        ASSIGN_INT(AutoRepeatInitialDelay);
    }
    else if (ConsumeToken(ctx, "autoRepeatRate")) {
        ASSIGN_INT(AutoRepeatDelayRate);
    }
    else if (ConsumeToken(ctx, "chordingDelay")) {
        ASSIGN_INT(ChordingDelay);
    }
    else if (ConsumeToken(ctx, "autoShiftDelay")) {
        ASSIGN_INT(AutoShiftDelay);
    }
    else if (ConsumeToken(ctx, "i2cBaudRate")) {
        if (action == SetCommandAction_Read) {
            return intVar(I2cMainBusRequestedBaudRateBps);
        }

        uint32_t baudRate = Macros_LegacyConsumeInt(ctx);
        if (Macros_DryRun) {
            return noneVar();
        }
        ChangeI2cBaudRate(baudRate);
    }
    else if (ConsumeToken(ctx, "emergencyKey")) {
        ASSIGN_CUSTOM5(key_state_t*, noneVar,, EmergencyKey, Utils_KeyIdToKeyState(Macros_LegacyConsumeInt(ctx)));
    }
    else if (action == SetCommandAction_Write) {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

macro_result_t Macro_ProcessSetCommand(parser_context_t* ctx)
{
    root(ctx, SetCommandAction_Write);
    return MacroResult_Finished;
}

macro_variable_t Macro_TryReadConfigVal(parser_context_t* ctx)
{
    macro_variable_t res = root(ctx, SetCommandAction_Read);
    return res;
}
