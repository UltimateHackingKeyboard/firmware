#include "macros/set_command.h"
#include "attributes.h"
#include "config_parser/parse_config.h"
#include "config_parser/parse_keymap.h"
#include "key_states.h"
#include "layer.h"
#include "ledmap.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "macros/shortcut_parser.h"
#include "macros/vars.h"
#include "macros/commands.h"
#include "module.h"
#include "secondary_role_driver.h"
#include "slot.h"
#include "timer.h"
#include "keymap.h"
#include "key_matrix.h"
#include "usb_report_updater.h"
#include "led_display.h"
#include "postponer.h"
#include "macro_recorder.h"
#include "str_utils.h"
#include "utils.h"
#include "layer_switcher.h"
#include "mouse_controller.h"
#include "mouse_keys.h"
#include "debug.h"
#include "caret_config.h"
#include "config_parser/parse_macro.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include <stdint.h>
#include "config_manager.h"
#include "led_manager.h"

#ifdef __ZEPHYR__
#include "state_sync.h"
#include "bt_conn.h"
#include "bt_manager.h"
#else
#include "init_peripherals.h"
#endif

typedef enum {
    SetCommandAction_Write,
    SetCommandAction_Read,
} set_command_action_t;

#define DEFINE_INT_LIMITS(LOWER_BOUND, UPPER_BOUND)            \
    bool useLimits = true;                                     \
    int32_t lowerBound = LOWER_BOUND;                          \
    int32_t upperBound = UPPER_BOUND;                          \

#define DEFINE_FLOAT_LIMITS(LOWER_BOUND, UPPER_BOUND)          \
    bool useLimits = true;                                     \
    float lowerBound = LOWER_BOUND;                            \
    float upperBound = UPPER_BOUND;                            \

#define DEFINE_NONE_LIMITS()                                   \
    bool useLimits = false;                                    \
    int32_t lowerBound = 0;                                    \
    int32_t upperBound = 0;                                    \


#define ASSIGN_NO_LIMITS(TPE, RET, RETPARAM, DST, SRC)         \
    if (action == SetCommandAction_Read) {                     \
        return RET(RETPARAM);                                  \
    }                                                          \
    TPE res = SRC;                                             \
    if (Macros_ParserError || Macros_DryRun) {                 \
        return noneVar();                                      \
    }                                                          \
    DST = res;                                                 \


#define ASSIGN_CUSTOM5(TPE, RET, RETPARAM, DST, SRC)     \
    if (action == SetCommandAction_Read) {                     \
        return RET(RETPARAM);                                  \
    }                                                          \
    TPE res = SRC;                                             \
    if (Macros_ParserError || Macros_DryRun) {                 \
        return noneVar();                                      \
    }                                                          \
    if (useLimits) {                                           \
        res = coalesce_ ## TPE(lowerBound, res, upperBound);  \
    }                                                          \
    DST = res;                                                 \

#define ASSIGN_INT_MUL(DST, M)                                 \
    if (action == SetCommandAction_Read) {                     \
        return intVar(DST/(M));                                \
    }                                                          \
    uint32_t res = Macros_ConsumeInt(ctx)*(M);           \
    if (Macros_ParserError || Macros_DryRun) {                 \
        return noneVar();                                      \
    }                                                          \
    if (useLimits) {                                           \
        res = coalesce_ ## int32_t(lowerBound, res, upperBound);   \
    }                                                          \
    DST = res;                                                 \

#define ASSIGN_INT2(DST, DST2)                                 \
    if (action == SetCommandAction_Read) {                     \
        return intVar(DST);                                    \
    }                                                          \
    int32_t res = Macros_ConsumeInt(ctx);               \
    if (Macros_ParserError || Macros_DryRun) {                 \
        return noneVar();                                      \
    }                                                          \
    if (useLimits) {                                           \
        res = coalesce_ ## int32_t(lowerBound, res, upperBound);   \
    }                                                          \
    DST = res;                                                 \
    DST2 = res;                                                \

#define ASSIGN_CUSTOM(TPE, RET, DST, SRC) ASSIGN_CUSTOM5(TPE, RET, DST, DST, SRC)
#define ASSIGN_ENUM(DST, SRC) ASSIGN_NO_LIMITS(uint8_t, intVar, DST, DST, SRC)
#define ASSIGN_FLOAT(DST) ASSIGN_CUSTOM(float, floatVar, DST, Macros_ConsumeFloat(ctx))
#define ASSIGN_BOOL(DST) ASSIGN_NO_LIMITS(bool, boolVar, DST, DST, Macros_ConsumeBool(ctx))
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

static int32_t coalesce_int32_t(int32_t lowerBound, int32_t val, int32_t upperBound) {
    if (val < lowerBound) {
        return lowerBound;
    } else if (val > upperBound) {
        return upperBound;
    } else {
        return val;
    }
}

static float coalesce_float(float lowerBound, float val, float upperBound) {
    if (val < lowerBound) {
        return lowerBound;
    } else if (val > upperBound) {
        return upperBound;
    } else {
        return val;
    }
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
        DEFINE_FLOAT_LIMITS(0.0f, 1000.0f);
        ASSIGN_FLOAT(module->baseSpeed);
    }
    else if (ConsumeToken(ctx, "speed")) {
        DEFINE_FLOAT_LIMITS(0.0f, 1000.0f);
        ASSIGN_FLOAT(module->speed);
    }
    else if (ConsumeToken(ctx, "xceleration")) {
        DEFINE_FLOAT_LIMITS(0.0f, 1000.0f);
        ASSIGN_FLOAT(module->xceleration);
    }
    else if (ConsumeToken(ctx, "caretSpeedDivisor")) {
        DEFINE_FLOAT_LIMITS(1.0f, 1000.0f);
        ASSIGN_FLOAT(module->caretSpeedDivisor);
    }
    else if (ConsumeToken(ctx, "scrollSpeedDivisor")) {
        DEFINE_FLOAT_LIMITS(1.0f, 1000.0f);
        ASSIGN_FLOAT(module->scrollSpeedDivisor);
    }
    else if (ConsumeToken(ctx, "pinchZoomDivisor") && moduleId == ModuleId_TouchpadRight) {
        DEFINE_FLOAT_LIMITS(1.0f, 1000.0f);
        ASSIGN_FLOAT(module->pinchZoomSpeedDivisor);
    }
    else if (ConsumeToken(ctx, "pinchZoomMode") && moduleId == ModuleId_TouchpadRight) {
        DEFINE_NONE_LIMITS();
        ASSIGN_CUSTOM(int32_t, intVar, Cfg.TouchpadPinchZoomMode, ConsumeNavigationModeId(ctx));
    }
    else if (ConsumeToken(ctx, "axisLockSkew")) {
        DEFINE_FLOAT_LIMITS(0.0f, 1000.0f);
        ASSIGN_FLOAT(module->axisLockSkew);
    }
    else if (ConsumeToken(ctx, "axisLockFirstTickSkew")) {
        DEFINE_FLOAT_LIMITS(0.0f, 1000.0f);
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
    else if (ConsumeToken(ctx, "holdContinuationTimeout") && moduleId == ModuleId_TouchpadRight) {
        DEFINE_INT_LIMITS(0, 65535);
        ASSIGN_INT(Cfg.HoldContinuationTimeout);
    }
    else {
        return moduleSpeed(ctx, action, module, moduleId);
    }
    return noneVar();
}

static macro_variable_t secondaryRoleAdvanced(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "timeout")) {
        DEFINE_INT_LIMITS(0, 65535);
        ASSIGN_INT(Cfg.SecondaryRoles_AdvancedStrategyTimeout);
    }
    else if (ConsumeToken(ctx, "timeoutAction")) {
        DEFINE_NONE_LIMITS();
        ASSIGN_CUSTOM(int32_t, intVar, Cfg.SecondaryRoles_AdvancedStrategyTimeoutAction, ConsumeSecondaryRoleTimeoutAction(ctx));
    }
    else if (ConsumeToken(ctx, "safetyMargin")) {
        DEFINE_INT_LIMITS(-32768, 32767);
        ASSIGN_INT(Cfg.SecondaryRoles_AdvancedStrategySafetyMargin);
    }
    else if (ConsumeToken(ctx, "triggerByRelease")) {
        ASSIGN_BOOL(Cfg.SecondaryRoles_AdvancedStrategyTriggerByRelease);
    }
    else if (ConsumeToken(ctx, "triggerByPress")) {
        ASSIGN_BOOL(Cfg.SecondaryRoles_AdvancedStrategyTriggerByPress);
    }
    else if (ConsumeToken(ctx, "triggerByMouse")) {
        ASSIGN_BOOL(Cfg.SecondaryRoles_AdvancedStrategyTriggerByMouse);
    }
    else if (ConsumeToken(ctx, "doubletapToPrimary")) {
        ASSIGN_BOOL(Cfg.SecondaryRoles_AdvancedStrategyDoubletapToPrimary);
    }
    else if (ConsumeToken(ctx, "doubletapTime")) {
        DEFINE_INT_LIMITS(0, 65535);
        ASSIGN_INT(Cfg.SecondaryRoles_AdvancedStrategyDoubletapTimeout);
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static macro_variable_t secondaryRoles(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "defaultStrategy")) {
        DEFINE_NONE_LIMITS();
        ASSIGN_CUSTOM(int32_t, intVar, Cfg.SecondaryRoles_Strategy, ConsumeSecondaryRoleStrategy(ctx));
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

static macro_variable_t allowUnsecuredConnections(parser_context_t* ctx, set_command_action_t action)
{
    ASSIGN_BOOL(Cfg.Bt_AllowUnsecuredConnections);
    if (Cfg.Bt_AllowUnsecuredConnections) {
        Macros_ReportPrintf(ctx->at, "Warning: insecure connections were allowed. This may allow eavesdropping on your keyboard input!");
    }

    return noneVar();
}

static macro_variable_t bluetooth(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "enabled")) {
        bool newBtEnabled = Cfg.Bt_Enabled;
        ASSIGN_BOOL(newBtEnabled);
#if DEVICE_IS_UHK80_RIGHT
        Bt_SetEnabled(newBtEnabled);
#endif
    } else if (ConsumeToken(ctx, "allowUnsecuredConnections")) {
        return allowUnsecuredConnections(ctx, action);
    } else if (ConsumeToken(ctx, "peripheralConnectionCount")) {
#ifdef __ZEPHYR__
        DEFINE_INT_LIMITS(1, PERIPHERAL_CONNECTION_COUNT);
#else
        DEFINE_INT_LIMITS(1, 1);
#endif
        ASSIGN_INT(Cfg.Bt_MaxPeripheralConnections);
    } else if (ConsumeToken(ctx, "alwaysAdvertiseHid")) {
        ASSIGN_BOOL(Cfg.Bt_AlwaysAdvertiseHid);
#if DEVICE_IS_UHK80_RIGHT
        BtManager_StartScanningAndAdvertisingAsync();
#endif
    } else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static macro_variable_t mouseKeys(parser_context_t* ctx, set_command_action_t action)
{
    mouse_kinetic_state_t* state = &Cfg.MouseMoveState;

    if (ConsumeToken(ctx, "move")) {
        state = &Cfg.MouseMoveState;
    } else if (ConsumeToken(ctx, "scroll")) {
        state = &Cfg.MouseScrollState;
    } else {
        Macros_ReportError("Scroll or move expected!", ctx->at, ctx->at);
        return noneVar();
    }

    ConsumeUntilDot(ctx);

    if (ConsumeToken(ctx, "initialSpeed")) {
        DEFINE_INT_LIMITS(0, 255);
        ASSIGN_INT_MUL(state->initialSpeed, 1.0f/state->intMultiplier);
    }
    else if (ConsumeToken(ctx, "baseSpeed")) {
        DEFINE_INT_LIMITS(0, 255);
        ASSIGN_INT_MUL(state->baseSpeed, 1.0f/state->intMultiplier);
    }
    else if (ConsumeToken(ctx, "initialAcceleration")) {
        DEFINE_INT_LIMITS(0, 255);
        ASSIGN_INT_MUL(state->acceleration, 1.0f/state->intMultiplier);
    }
    else if (ConsumeToken(ctx, "deceleratedSpeed")) {
        DEFINE_INT_LIMITS(0, 255);
        ASSIGN_INT_MUL(state->deceleratedSpeed, 1.0f/state->intMultiplier);
    }
    else if (ConsumeToken(ctx, "acceleratedSpeed")) {
        DEFINE_INT_LIMITS(0, 255);
        ASSIGN_INT_MUL(state->acceleratedSpeed, 1.0f/state->intMultiplier);
    }
    else if (ConsumeToken(ctx, "axisSkew")) {
        DEFINE_FLOAT_LIMITS(0.001f, 1000.0f);
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
        ASSIGN_ENUM(Cfg.StickyModifierStrategy, Stick_Never);
    }
    else if (ConsumeToken(ctx, "smart")) {
        ASSIGN_ENUM(Cfg.StickyModifierStrategy, Stick_Smart);
    }
    else if (ConsumeToken(ctx, "always")) {
        ASSIGN_ENUM(Cfg.StickyModifierStrategy, Stick_Always);
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }
    return noneVar();
}

static macro_variable_t macroEngineScheduler(parser_context_t* ctx, set_command_action_t action)
{
    if (ConsumeToken(ctx, "preemptive")) {
        ASSIGN_ENUM(Cfg.Macros_Scheduler, Scheduler_Preemptive);
    }
    else if (ConsumeToken(ctx, "blocking")) {
        ASSIGN_ENUM(Cfg.Macros_Scheduler, Scheduler_Blocking);
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
        DEFINE_INT_LIMITS(0, 255);
        ASSIGN_INT(Cfg.Macros_MaxBatchSize);
    }
    else if (ConsumeToken(ctx, "extendedCommands")) {
        Macros_ConsumeInt(ctx);
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
        return intVar(Cfg.BacklightingMode);
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
    EventVector_Set(EventVector_LedMapUpdateNeeded);
    return noneVar();
}

static macro_variable_t keyRgb(parser_context_t* ctx, set_command_action_t action)
{
    layer_id_t layerId = Macros_ConsumeLayerId(ctx);

    ConsumeUntilDot(ctx);

    uint16_t keyId = Macros_TryConsumeKeyId(ctx);

    if (keyId == 255) {
        Macros_ReportError("Failed to decode keyid!", ctx->at, ctx->at);
        return noneVar();
    }

    if (action == SetCommandAction_Read) {
        Macros_ReportError("Reading RGB values not supported!", ConsumedToken(ctx), ConsumedToken(ctx));
        return noneVar();
    }

    rgb_t rgb;
    rgb.red = Macros_ConsumeInt(ctx);
    rgb.green = Macros_ConsumeInt(ctx);
    rgb.blue = Macros_ConsumeInt(ctx);

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

#ifdef __ZEPHYR__
    if (slotIdx != SlotId_RightKeyboardHalf) {
        StateSync_UpdateLayer(layerId, true);
    }
#endif
    EventVector_Set(EventVector_LedMapUpdateNeeded);
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
        rgb.red = Macros_ConsumeInt(ctx);
        rgb.green = Macros_ConsumeInt(ctx);
        rgb.blue = Macros_ConsumeInt(ctx);

        if (Macros_DryRun) {
            return noneVar();
        }

        Cfg.LedMap_ConstantRGB = rgb;
        Ledmap_SetLedBacklightingMode(BacklightingMode_ConstantRGB);
        EventVector_Set(EventVector_LedMapUpdateNeeded);

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
        if (action == SetCommandAction_Read) {
            Macros_ReportError("Reading global fade timeout is not supported!", ConsumedToken(ctx), ConsumedToken(ctx));
            return noneVar();
        }

        uint32_t res = Macros_ConsumeInt(ctx)*1000;

        if (Macros_ParserError || Macros_DryRun) {
            return noneVar();
        }

        Cfg.KeyBacklightFadeOutTimeout = res;
        Cfg.KeyBacklightFadeOutBatteryTimeout = res;
        Cfg.DisplayFadeOutTimeout = res;
        Cfg.DisplayFadeOutBatteryTimeout = res;
    } else if (ConsumeToken(ctx, "keyBacklightFadeTimeout")) {
        DEFINE_NONE_LIMITS();
        ASSIGN_INT_MUL(Cfg.KeyBacklightFadeOutTimeout, 1000);
    } else if (ConsumeToken(ctx, "displayFadeTimeout")) {
        DEFINE_NONE_LIMITS();
        ASSIGN_INT_MUL(Cfg.DisplayFadeOutTimeout, 1000);
    } else if (ConsumeToken(ctx, "keyBacklightFadeBatteryTimeout")) {
        DEFINE_NONE_LIMITS();
        ASSIGN_INT_MUL(Cfg.KeyBacklightFadeOutBatteryTimeout, 1000);
    } else if (ConsumeToken(ctx, "displayFadeBatteryTimeout")) {
        DEFINE_NONE_LIMITS();
        ASSIGN_INT_MUL(Cfg.DisplayFadeOutBatteryTimeout, 1000);
    } else if (ConsumeToken(ctx, "brightness")) {
        DEFINE_FLOAT_LIMITS(1.0f/256.0f, 255.0f);
        ASSIGN_FLOAT(Cfg.LedBrightnessMultiplier);
    } else if (ConsumeToken(ctx, "enabled")) {
        ASSIGN_BOOL(Cfg.LedsEnabled);
    } else if (ConsumeToken(ctx, "alwaysOn")) {
        ASSIGN_BOOL(Cfg.LedsAlwaysOn);
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
    }

    EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
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

    uint16_t keyId = Macros_TryConsumeKeyId(ctx);

    if (keyId == 255) {
        Macros_ReportError("Failed to decode keyid!", ctx->at, ctx->at);
        return noneVar();
    }

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

#ifdef __ZEPHYR__
    StateSync_UpdateLayer(layerId, true);
#endif

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
        return intVar(Cfg.LayerConfig[layerId].modifierLayerMask);
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

    Cfg.LayerConfig[layerId].modifierLayerMask = mask;
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
    else if (ConsumeToken(ctx, "bluetooth")) {
        ConsumeUntilDot(ctx);
        return bluetooth(ctx, action);
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
        ASSIGN_BOOL(Cfg.DiagonalSpeedCompensation);
    }
    else if (ConsumeToken(ctx, "stickyModifiers")) {
        return stickyModifiers(ctx, action);
    }
    else if (ConsumeToken(ctx, "debounceDelay")) {
        DEFINE_INT_LIMITS(0, 255);
        ASSIGN_INT2(Cfg.DebounceTimePress, Cfg.DebounceTimeRelease);
    }
    else if (ConsumeToken(ctx, "keystrokeDelay")) {
        DEFINE_INT_LIMITS(0, 65535);
        ASSIGN_INT(Cfg.KeystrokeDelay);
    }
    else if (
            ConsumeToken(ctx, "doubletapTimeout")  // new name
            || (ConsumeToken(ctx, "doubletapDelay")) // deprecated alias - old name
            ) {
        DEFINE_INT_LIMITS(0, 65535);
        ASSIGN_INT(Cfg.DoubletapTimeout);
    }
    else if ( ConsumeToken(ctx, "holdTimeout")) {
        DEFINE_INT_LIMITS(0, 65535);
        ASSIGN_INT(Cfg.HoldTimeout);
    }
    else if (ConsumeToken(ctx, "autoRepeatDelay")) {
        DEFINE_INT_LIMITS(0, 65535);
        ASSIGN_INT(Cfg.AutoRepeatInitialDelay);
    }
    else if (ConsumeToken(ctx, "autoRepeatRate")) {
        DEFINE_INT_LIMITS(0, 65535);
        ASSIGN_INT(Cfg.AutoRepeatDelayRate);
    }
    else if (ConsumeToken(ctx, "oneShotTimeout")) {
        DEFINE_INT_LIMITS(0, 65535);
        ASSIGN_INT(Cfg.Macros_OneShotTimeout);
    }
    else if (ConsumeToken(ctx, "chordingDelay")) {
        DEFINE_INT_LIMITS(0, 255);
        ASSIGN_INT(Cfg.ChordingDelay);
    }
    else if (ConsumeToken(ctx, "autoShiftDelay")) {
        DEFINE_INT_LIMITS(0, 65535);
        ASSIGN_INT(Cfg.AutoShiftDelay);
    }
    else if (ConsumeToken(ctx, "allowUnsecuredConnections")) {
        return allowUnsecuredConnections(ctx, action);
    }
#ifndef __ZEPHYR__
    else if (ConsumeToken(ctx, "i2cBaudRate")) {
        if (action == SetCommandAction_Read) {
            return intVar(Cfg.I2cBaudRate);
        }

        uint32_t baudRate = Macros_ConsumeInt(ctx);
        if (Macros_DryRun) {
            return noneVar();
        }
        Cfg.I2cBaudRate = baudRate;
        ChangeI2cBaudRate(baudRate);
    }
#endif
    else if (ConsumeToken(ctx, "emergencyKey")) {
        ASSIGN_NO_LIMITS(key_state_t*, noneVar,, Cfg.EmergencyKey, Utils_KeyIdToKeyState(Macros_ConsumeInt(ctx)));
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
