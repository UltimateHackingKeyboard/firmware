#include "macros/vars.h"
#include "macros/string_reader.h"
#include "postponer.h"
#include "macros/keyid_parser.h"
#include "typedefs.h"
#include "utils.h"
#include "str_utils.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "config_parser/config_globals.h"
#include "debug.h"
#include "macros/set_command.h"
#include "config_manager.h"
#include "str_utils.h"
#include <math.h>
#include <inttypes.h>
#include "trace.h"
#include "usb_report_updater.h"

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef enum {
    Operator_Add,
    Operator_Sub,
    Operator_Mul,
    Operator_Div,
    Operator_Mod,
    Operator_And,
    Operator_Or,
    Operator_Gt,
    Operator_Ge,
    Operator_Lt,
    Operator_Le,
    Operator_Eq,
    Operator_Ne,
    Operator_Min,
    Operator_Max,

} operator_t;

macro_variable_t macroVariables[MACRO_VARIABLE_COUNT_MAX];
uint8_t macroVariableCount = 0;

macro_argument_t macroArguments[MAX_MACRO_ARGUMENT_POOL_SIZE];
// uint8_t macroArgumentCount = 0;

static macro_variable_t consumeArgumentAsValue(parser_context_t* ctx);
static macro_variable_t consumeParenthessExpression(parser_context_t* ctx);
static macro_variable_t consumeValue(parser_context_t* ctx);
static macro_variable_t negate(parser_context_t *ctx, macro_variable_t res);
static macro_variable_t consumeMinMaxOperation(parser_context_t* ctx, operator_t op);
static macro_variable_t negateBool(parser_context_t *ctx, macro_variable_t res);

macro_result_t Macros_ProcessStatsVariablesCommand(void) {
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    Macros_Printf("Variables:");
    for (uint8_t i = 0; i < macroVariableCount; i++) {
        char buffer[32];
        Macros_SerializeVar(buffer, sizeof(buffer), macroVariables[i]);
        Macros_Printf("  %.*s: %s", EXPAND_REF(macroVariables[i].name), buffer);
    }

    return MacroResult_Finished;
}

static macro_variable_t intVar(int32_t value)
{
    return (macro_variable_t) { .asInt = value, .type = MacroVariableType_Int };
}

static macro_variable_t boolVar(bool value)
{
    return (macro_variable_t) { .asBool = value, .type = MacroVariableType_Bool };
}

static macro_variable_t stringVar(string_ref_t value)
{
    return (macro_variable_t) { .asStringRef = value, .type = MacroVariableType_String };
}

static macro_variable_t noneVar()
{
    return (macro_variable_t) { .asInt = 1, .type = MacroVariableType_None };
}

static macro_variable_t consumeNumericValue(parser_context_t* ctx)
{
    macro_variable_t res = { .type = MacroVariableType_Int, .asInt = 0 };

    bool numFound = false;
    while(*ctx->at > 47 && *ctx->at < 58 && ctx->at < ctx->end) {
        res.asInt = res.asInt*10 + ((uint8_t)(*ctx->at))-48;
        ctx->at++;
        numFound = true;
    }
    if (*ctx->at == '.') {
        res.type = MacroVariableType_Float;
        res.asFloat = (float) res.asInt;
        ctx->at++;

        float b = 0.1;
        while(*ctx->at > 47 && *ctx->at < 58 && ctx->at < ctx->end) {
            res.asFloat += (((uint8_t)(*ctx->at))-48)*b;
            b = b*0.1f;
            ctx->at++;
            numFound = true;
        }
    }
    if (!numFound) {
        Macros_ReportErrorTok(ctx, "Numeric value expected");
        return noneVar();
    }

    ConsumeWhite(ctx);
    return res;
}

static macro_variable_t consumeStringLiteral(parser_context_t* ctx)
{
    const char* stringStart = ctx->at;
    uint8_t nestingLevelBefore = ctx->nestingLevel;

    Macros_ConsumeStringToken(ctx);

    if (ctx->nestingLevel != nestingLevelBefore) {
        Macros_ReportError("String literal crossed context boundary", stringStart, ctx->at);
        return noneVar();
    }

    uint16_t offset = stringStart - (const char*)ValidatedUserConfigBuffer.buffer;
    uint8_t len = ctx->at - stringStart;

    return stringVar((string_ref_t){ .offset = offset, .len = len });
}


macro_variable_t* Macros_ConsumeExistingWritableVariable(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        if (!IsIdentifierChar(*ctx->at)) {
            Macros_ReportErrorPos(ctx, "Identifier expected here:");
            return NULL;
        }
        ConsumeAnyIdentifier(ctx);
        return NULL;
    }

    //TODO: optimize this!
    for (uint8_t i = 0; i < macroVariableCount; i++) {
        if (ConsumeIdentifierByRef(ctx, macroVariables[i].name)) {
            return &macroVariables[i];
        }
    }
    return NULL;
}

// Expects <variable name>
static macro_variable_t consumeVariable(parser_context_t* ctx)
{
    //TODO: handle indirection, e.g., `$$<variable name>`

    if (Macros_DryRun) {
        if (!IsIdentifierChar(*ctx->at)) {
            Macros_ReportErrorPos(ctx, "Identifier expected here:");
            return noneVar();
        }
        ConsumeAnyIdentifier(ctx);
        return (macro_variable_t){ .type = MacroVariableType_Int, .asInt = 1 };
    }

    //TODO: optimize this!
    for (uint8_t i = 0; i < macroVariableCount; i++) {
        if (ConsumeIdentifierByRef(ctx, macroVariables[i].name)) {
            return macroVariables[i];
        }
    }

    ConsumeAnyIdentifier(ctx);
    return (macro_variable_t){};
}

// Expects <variable name>
static macro_variable_t* consumeVarAndAllocate(parser_context_t* ctx)
{
    //TODO: optimize this!
    for (uint8_t i = 0; i < macroVariableCount; i++) {
        if (ConsumeIdentifierByRef(ctx, macroVariables[i].name)) {
            return &macroVariables[i];
        }
    }

    // TODO: Is this needed at all? Looks like something left over
    // CTX_COPY(bakCtx, *ctx);
    macro_variable_t configVal = Macro_TryReadConfigVal(ctx);

    if (configVal.type != MacroVariableType_None) {
        Macros_ReportErrorTok(ctx, "Name already taken by configuration value:");
        return NULL;
    }

    uint16_t len = IdentifierEnd(ctx) - ctx->at;
    if (len > 255) {
        Macros_ReportErrorTok(ctx, "Variable name too long:");
        return NULL;
    }

    if (Macros_DryRun) {
        ConsumeAnyIdentifier(ctx);
        return NULL;
    }

    if (macroVariableCount == MACRO_VARIABLE_COUNT_MAX) {
        Macros_ReportErrorPrintf(ctx->at, "Too many variables. Can't allocate more than %d variables:", MACRO_VARIABLE_COUNT_MAX);
        ConsumeAnyIdentifier(ctx);
        return NULL;
    }


    macro_variable_t* res = &macroVariables[macroVariableCount];
    macroVariables[macroVariableCount].name = (string_ref_t) {
        .offset = ctx->at - (const char*)ValidatedUserConfigBuffer.buffer,
        .len = len
    };
    macroVariableCount++;
    ctx->at += len;

    ConsumeWhite(ctx);
    return res;
}

static macro_variable_t coalesceType(parser_context_t* ctx, macro_variable_t value, macro_variable_type_t dstType)
{
    if (value.type == dstType) {
        return value;
    }

    switch (dstType) {
        case MacroVariableType_Int:
            switch (value.type) {
                case MacroVariableType_Int:
                    value.asInt = value.asInt;
                    break;
                case MacroVariableType_Float:
                    value.asInt = (int32_t)value.asFloat;
                    break;
                case MacroVariableType_Bool:
                    value.asInt = value.asBool ? 1 : 0;
                    break;
                case MacroVariableType_String:
                    Macros_ReportError("Cannot convert string to int", NULL, NULL);
                    return noneVar();
                case MacroVariableType_None:
                    break;
                default:
                    Macros_ReportErrorNum("Unexpected variable type:", value.type, NULL);
                    break;
            }
            break;
        case MacroVariableType_Float:
            switch (value.type) {
                case MacroVariableType_Int:
                    value.asFloat = (float)value.asInt;
                    break;
                case MacroVariableType_Float:
                    value.asFloat = value.asFloat;
                    break;
                case MacroVariableType_Bool:
                    value.asFloat = value.asBool ? 1.0 : 0.0;
                    break;
                case MacroVariableType_String:
                    Macros_ReportError("Cannot convert string to float", NULL, NULL);
                    return noneVar();
                case MacroVariableType_None:
                    break;
                default:
                    Macros_ReportErrorNum("Unexpected variable type:", value.type, NULL);
                    break;
            }
            break;
        case MacroVariableType_Bool:
            switch (value.type) {
                case MacroVariableType_Int:
                    value.asBool = value.asInt != 0;
                    break;
                case MacroVariableType_Float:
                    value.asBool = value.asFloat != 0.0f;
                    break;
                case MacroVariableType_Bool:
                    value.asBool = !!value.asBool;
                    break;
                case MacroVariableType_String:
                    Macros_ReportError("Cannot convert string to bool", NULL, NULL);
                    return noneVar();
                case MacroVariableType_None:
                    break;
                default:
                    Macros_ReportErrorNum("Unexpected variable type:", value.type, NULL);
                    break;
            }
            break;
        case MacroVariableType_String:
            switch (value.type) {
                case MacroVariableType_Int:
                case MacroVariableType_Float:
                case MacroVariableType_Bool:
                    Macros_ReportError("Cannot convert to string", NULL, NULL);
                    return noneVar();
                case MacroVariableType_String:
                    // Already a string, no conversion needed
                    break;
                case MacroVariableType_None:
                    break;
                default:
                    Macros_ReportErrorNum("Unexpected variable type:", value.type, NULL);
                    break;
            }
            break;
        case MacroVariableType_None:
            break;
        default:
            Macros_ReportErrorNum("Unexpected variable type:", dstType, NULL);
            break;
    }
    value.type = dstType;
    return value;
}

static macro_variable_t consumeDollarExpression(parser_context_t* ctx)
{
    macro_variable_t res;
    if (*ctx->at == '(') {
        return consumeParenthessExpression(ctx);
    }
    else if (ConsumeToken(ctx, "thisKeyId")) {
        return intVar(Utils_KeyStateToKeyId(ctx->macroState->ms.currentMacroKey));
    }
    else if (ConsumeToken(ctx, "currentAddress")) {
        return intVar(ctx->macroState->ls->ms.commandAddress);
    }
    else if (ConsumeToken(ctx, "currentTime")) {
        return intVar(Timer_GetCurrentTime() & 0x7FFFFFFF);
    }
    else if (ConsumeToken(ctx, "queuedKeyId")) {
        ConsumeUntilDot(ctx);
        int8_t queueIdx = Macros_ConsumeInt(ctx);
        if (queueIdx >= PostponerQuery_PendingKeypressCount()) {
            if (!Macros_DryRun) {
                Macros_ReportError("Not enough pending keys! Note that this is zero-indexed!",  ConsumedToken(ctx), ConsumedToken(ctx));
                return noneVar();
            }
            return intVar(0);
        }
        return intVar(PostponerExtended_PendingId(queueIdx));
    }
    else if (ConsumeToken(ctx, "keyId")) {
        ConsumeUntilDot(ctx);
        uint8_t keyId = MacroKeyIdParser_TryConsumeKeyId(ctx);
        if (keyId == 255) {
            Macros_ReportErrorTok(ctx, "KeyId abbreviation expected");
            return noneVar();
        }
        return intVar(keyId);
    }
    else if (ConsumeToken(ctx, "uhk")) {
        ConsumeUntilDot(ctx);
        if (ConsumeToken(ctx, "name")) {
            return stringVar(Cfg.DeviceName);
        } else {
            Macros_ReportErrorTok(ctx, "UHK property not recognized:");
            return noneVar();
        }
    }
    else if (ConsumeToken(ctx, "macroArg")) {
        return consumeArgumentAsValue(ctx);
    }
    else if ((res = Macro_TryReadConfigVal(ctx)).type != MacroVariableType_None) {
        return res;
    } else {
        return consumeVariable(ctx);
    }
}

static macro_variable_t consumeValue(parser_context_t* ctx)
{
    if (*ctx->at == '&') {
        TryExpandMacroTemplateOnce(ctx);
        if (Macros_ParserError) {
            return noneVar();
        }
    }

    switch (*ctx->at) {
        case '+':
            ConsumeWhiteAt(ctx, ctx->at+1);
            return consumeValue(ctx);
        case '-':
            ConsumeWhiteAt(ctx, ctx->at+1);
            return negate(ctx, consumeValue(ctx));
        case '!':
            ConsumeWhiteAt(ctx, ctx->at+1);
            return negateBool(ctx, consumeValue(ctx));
        case '.':
        case '0' ... '9':
            return consumeNumericValue(ctx);
        case '"':
        case '\'':
            return consumeStringLiteral(ctx);
        case 'f':
            if (ConsumeToken(ctx, "false")) {
                return (macro_variable_t){ .type = MacroVariableType_Bool, .asBool = false };
            }
            else {
                goto failed;
            }
        case 'm':
            if (ConsumeToken(ctx, "min")) {
                return consumeMinMaxOperation(ctx, Operator_Min);
            }
            else  if (ConsumeToken(ctx, "max")) {
                return consumeMinMaxOperation(ctx, Operator_Max);
            }
            else {
                goto failed;
            }

        case 't':
            if (ConsumeToken(ctx, "true")) {
                return (macro_variable_t){ .type = MacroVariableType_Bool, .asBool = true };
            }
            else {
                goto failed;
            }

        case '$':
            ctx->at++;
            return consumeDollarExpression(ctx);
        case '(':
            return consumeParenthessExpression(ctx);
        case '#':
            Macros_ReportErrorPos(ctx, "Registers were removed. Please, replace them with named variables. E.g., `setVar foo 1` and `$foo`.");
            return noneVar();
        case '%':
            Macros_ReportErrorPos(ctx, "`%` notation was removed. Please, replace it with $queuedKeyId.<index> notation. E.g., `$queuedKeyId.1`.");
            return noneVar();
        case '@':
            Macros_ReportErrorPos(ctx, "`@` notation was removed. Please, replace it with $currentAddress. E.g., `@3` with `$($currentAddress + 3)`.");
            return noneVar();
        default:
            goto failed;
    }

failed:
    if (IsIdentifierChar(*ctx->at)) {
        Macros_ReportErrorPrintf(ctx->at, "Parsing failed, did you mean '$%s'?", OneWord(ctx));
    } else {
        Macros_ReportErrorTok(ctx, "Could not parse");
    }
    return noneVar();
}

static macro_variable_t negate(parser_context_t *ctx, macro_variable_t res)
{
    switch (res.type) {
        case MacroVariableType_Int:
            res.asInt = -res.asInt;
            break;
        case MacroVariableType_Float:
            res.asFloat = -res.asFloat;
            break;
        case MacroVariableType_Bool:
            res.asInt = -res.asBool;
            res.type = MacroVariableType_Int;
            break;
        case MacroVariableType_String:
            Macros_ReportError("Cannot negate a string", NULL, NULL);
            return noneVar();
        case MacroVariableType_None:
            break;
        default:
            Macros_ReportErrorNum("Unexpected variable type:", res.type, ctx->at);
            break;
    }
    return res;
}


static macro_variable_t negateBool(parser_context_t *ctx, macro_variable_t res)
{
    res = coalesceType(ctx, res, MacroVariableType_Bool);
    res.asBool = !res.asBool;
    return res;
}

static macro_variable_type_t determineCommonType(macro_variable_type_t a, macro_variable_type_t b)
{
    if (a == MacroVariableType_None || b == MacroVariableType_None) {
        return MacroVariableType_None;
    } else if (a == MacroVariableType_String || b == MacroVariableType_String) {
        return MacroVariableType_String;
    } else if (a == MacroVariableType_Float || b == MacroVariableType_Float) {
        return MacroVariableType_Float;
    } else if (a == MacroVariableType_Int || b == MacroVariableType_Int) {
        return MacroVariableType_Int;
    } else {
        return MacroVariableType_Int;
    }
}

static float computeFloatOperation(float a, operator_t op, parser_context_t* opCtx, float b)
{
    switch (op) {
        case Operator_Add:
            return a+b;
        case Operator_Sub:
            return a-b;
        case Operator_Mul:
            return a*b;
        case Operator_Div:
            if (b == 0.0f) {
                Macros_ReportErrorPos(opCtx, "Division by zero attempted!");
                return 0;
            }
            return a/b;
        case Operator_Mod:
            Macros_ReportErrorPos(opCtx, "Modulo is not supported for floats!");
            return 1.0f;
        case Operator_Min:
            return MIN(a, b);
        case Operator_Max:
            return MAX(a, b);
        default:
            Macros_ReportErrorTok(opCtx, "Unexpected operation: ");
            return 1.0f;
    }
}

static int32_t computeIntOperation(int32_t a, operator_t op, parser_context_t* opCtx, int32_t b)
{
    switch (op) {
        case Operator_Add:
            return a+b;
        case Operator_Sub:
            return a-b;
        case Operator_Mul:
            return a*b;
        case Operator_Div:
            if (b == 0) {
                Macros_ReportErrorPos(opCtx, "Division by zero attempted!");
                return 0;
            }
            return a/b;
        case Operator_Mod:
            if (b == 0) {
                Macros_ReportErrorPos(opCtx, "Modulo by zero attempted!");
                return 0;
            }
            return a%b;
        case Operator_Min:
            return MIN(a, b);
        case Operator_Max:
            return MAX(a, b);
        default:
            Macros_ReportErrorTok(opCtx, "Unexpected operation: ");
            return 1;
    }
}

static bool computeFloatEqOperation(float a, operator_t op, parser_context_t* opCtx, float b)
{
    switch(op) {
        case Operator_Eq:
            return a == b;
        case Operator_Ne:
            return a != b;
        case Operator_Gt:
            return a > b;
        case Operator_Ge:
            return a >= b;
        case Operator_Lt:
            return a < b;
        case Operator_Le:
            return a <= b;
        default:
            Macros_ReportErrorTok(opCtx, "Unexpected operation: ");
            return false;
    }
}


static bool computeIntEqOperation(int32_t a, operator_t op, parser_context_t* opCtx, int32_t b)
{
    switch(op) {
        case Operator_Eq:
            return a == b;
        case Operator_Ne:
            return a != b;
        case Operator_Gt:
            return a > b;
        case Operator_Ge:
            return a >= b;
        case Operator_Lt:
            return a < b;
        case Operator_Le:
            return a <= b;
        default:
            Macros_ReportErrorTok(opCtx, "Unexpected operation: ");
            return false;
    }
}

static bool computeStringEqOperation(string_ref_t a, operator_t op, parser_context_t* opCtx, string_ref_t b)
{
    switch(op) {
        case Operator_Eq:
            return Macros_CompareStringRefs(a, b);
        case Operator_Ne:
            return !Macros_CompareStringRefs(a, b);
        default:
            Macros_ReportErrorTok(opCtx, "Only == and != operators are supported for strings");
            return false;
    }
}


static macro_variable_t computeBoolOperation(macro_variable_t a, operator_t op, parser_context_t* opCtx, macro_variable_t b)
{
    a = coalesceType(opCtx, a, MacroVariableType_Bool);
    b = coalesceType(opCtx, b, MacroVariableType_Bool);

    switch(op) {
        case Operator_And:
            return boolVar(a.asBool && b.asBool);
        case Operator_Or:
            return boolVar(a.asBool || b.asBool);
        default:
            Macros_ReportErrorTok(opCtx, "Unexpected operation: ");
            return noneVar();
    }
}

static macro_variable_t computeOperation(macro_variable_t a, operator_t op, parser_context_t* opCtx, macro_variable_t b)
{
    macro_variable_type_t dstType = determineCommonType(a.type, b.type);
    a = coalesceType(opCtx, a, dstType);
    b = coalesceType(opCtx, b, dstType);

    switch (dstType) {
        case MacroVariableType_Int:
            a.asInt = computeIntOperation(a.asInt, op, opCtx, b.asInt);
            break;
        case MacroVariableType_Float:
            a.asFloat = computeFloatOperation(a.asFloat, op, opCtx, b.asFloat);
            break;
        case MacroVariableType_Bool:
            a.asInt = computeIntOperation(a.asInt, op, opCtx, b.asInt);
            return noneVar();
        case MacroVariableType_String:
            Macros_ReportError("Arithmetic operations are not supported for strings", NULL, NULL);
            return noneVar();
        case MacroVariableType_None:
            return noneVar();
        default:
            Macros_ReportErrorNum("Unexpected variable type:", dstType, NULL);
            return noneVar();
    }
    if (Macros_ParserError) {
        a.type = MacroVariableType_None;
    }
    return a;
}


static macro_variable_t computeEqOperation(macro_variable_t a, operator_t op, parser_context_t* opCtx, macro_variable_t b)
{
    macro_variable_type_t dstType = determineCommonType(a.type, b.type);
    a = coalesceType(opCtx, a, dstType);
    b = coalesceType(opCtx, b, dstType);

    switch (dstType) {
        case MacroVariableType_Int:
            a.asBool = computeIntEqOperation(a.asInt, op, opCtx, b.asInt);
            a.type = MacroVariableType_Bool;
            break;
        case MacroVariableType_Float:
            a.asBool = computeFloatEqOperation(a.asFloat, op, opCtx, b.asFloat);
            a.type = MacroVariableType_Bool;
            break;
        case MacroVariableType_Bool:
            a.asBool = computeIntEqOperation(a.asBool, op, opCtx, b.asBool);
            a.type = MacroVariableType_Bool;
            return noneVar();
        case MacroVariableType_String:
            a.asBool = computeStringEqOperation(a.asStringRef, op, opCtx, b.asStringRef);
            a.type = MacroVariableType_Bool;
            break;
        case MacroVariableType_None:
            return noneVar();
        default:
            Macros_ReportErrorNum("Unexpected variable type:", dstType, NULL);
            return noneVar();
    }
    if (Macros_ParserError) {
        a.type = MacroVariableType_None;
    }
    return a;
}

static macro_variable_t consumeMinMaxOperation(parser_context_t* ctx, operator_t op)
{
    if (!ConsumeToken(ctx, "("))
    {
        Macros_ReportErrorPos(ctx, "Expected parameter list.");
        return noneVar();
    }
    macro_variable_t accumulator = consumeValue(ctx);

    while (ConsumeToken(ctx, ",")) {
        macro_variable_t var = consumeValue(ctx);
        accumulator = computeOperation(accumulator, op, ctx, var);
    }

    if (!ConsumeToken(ctx, ")"))
    {
        Macros_ReportErrorPos(ctx, "Expected closing parenthesis at the end of parameter list.");
        return noneVar();
    }

    return accumulator;
}

static macro_variable_t consumeMultiplicativeExpression(parser_context_t* ctx)
{
    macro_variable_t accumulator = consumeValue(ctx);
    operator_t op;

    ConsumeWhite(ctx);

    while (true) {
        CTX_COPY(opCtx, *ctx);
        switch (*ctx->at) {
            case '*':
                ConsumeWhiteAt(ctx, ctx->at+1);
                op = Operator_Mul;
                break;
            case '/':
                ConsumeWhiteAt(ctx, ctx->at+1);
                op = Operator_Div;
                break;
            case '%':
                ConsumeWhiteAt(ctx, ctx->at+1);
                op = Operator_Mod;
                break;
            break;
            default:
                return accumulator;
        }
        macro_variable_t var = consumeValue(ctx);
        accumulator = computeOperation(accumulator, op, &opCtx, var);
    }
}

static macro_variable_t consumeAdditiveExpression(parser_context_t* ctx)
{
    macro_variable_t accumulator = consumeMultiplicativeExpression(ctx);
    operator_t op;

    while (true) {
        CTX_COPY(opCtx, *ctx);
        switch (*ctx->at) {
            case '+':
                ConsumeWhiteAt(ctx, ctx->at+1);
                op = Operator_Add;
                break;
            case '-':
                ConsumeWhiteAt(ctx, ctx->at+1);
                op = Operator_Sub;
                break;
            default:
                return accumulator;
        }
        macro_variable_t var = consumeMultiplicativeExpression(ctx);
        accumulator = computeOperation(accumulator, op, &opCtx, var);
    }
}


static macro_variable_t consumeEqExpression(parser_context_t* ctx)
{
    macro_variable_t accumulator = consumeAdditiveExpression(ctx);
    CTX_COPY(opCtx, *ctx);
    operator_t op;

    switch (*ctx->at) {
        case '<':
            if (ConsumeToken(ctx, "<=")) {
                op = Operator_Le;
            } else if (ConsumeToken(ctx, "<")) {
                op = Operator_Lt;
            } else {
                goto def;
            }
            break;
        case '>':
            if (ConsumeToken(ctx, ">=")) {
                op = Operator_Ge;
            } else if (ConsumeToken(ctx, ">")) {
                op = Operator_Gt;
            } else {
                goto def;
            }
            break;
        case '!':
            if (ConsumeToken(ctx, "!=")) {
                op = Operator_Ne;
            } else {
                goto def;
            }
            break;
        case '=':
            if (ConsumeToken(ctx, "==")) {
                op = Operator_Eq;
            } else {
                goto def;
            }
            break;
        def:
        default:
            return accumulator;
    }

    macro_variable_t var = consumeAdditiveExpression(ctx);
    accumulator = computeEqOperation(accumulator, op, &opCtx, var);
    return accumulator;
}

static macro_variable_t consumeAndExpression(parser_context_t* ctx)
{
    macro_variable_t accumulator = consumeEqExpression(ctx);
    operator_t op;

    while (true) {
        CTX_COPY(opCtx, *ctx);
        if (ConsumeToken(ctx, "&&")) {
            op = Operator_And;
        } else {
            return accumulator;
        }
        macro_variable_t var = consumeEqExpression(ctx);
        accumulator = computeBoolOperation(accumulator, op, &opCtx, var);
    }
}

static macro_variable_t consumeOrExpression(parser_context_t* ctx)
{
    macro_variable_t accumulator = consumeAndExpression(ctx);
    operator_t op;

    while (true) {
        CTX_COPY(opCtx, *ctx);
        if (ConsumeToken(ctx, "||")) {
            op = Operator_Or;
        } else {
            return accumulator;
        }
        macro_variable_t var = consumeAndExpression(ctx);
        accumulator = computeBoolOperation(accumulator, op, &opCtx, var);
    }
}

static macro_variable_t consumeParenthessExpression(parser_context_t* ctx)
{
    ConsumeWhiteAt(ctx, ctx->at+1);

    macro_variable_t value = consumeOrExpression(ctx);

    switch (*ctx->at) {
        case ')':
            ConsumeWhiteAt(ctx, ctx->at+1);
            return value;
        default:
            if (!Macros_ParserError) {
                Macros_ReportErrorPos(ctx, "Failed to parse expression, closing parenthesis expected.");
            }
            return noneVar();
    }
}

int32_t Macros_ConsumeInt(parser_context_t* ctx)
{

    macro_variable_t res = consumeValue(ctx);
    return coalesceType(ctx, res, MacroVariableType_Int).asInt;
}

float Macros_ConsumeFloat(parser_context_t* ctx)
{
    macro_variable_t res = consumeValue(ctx);
    return coalesceType(ctx, res, MacroVariableType_Float).asFloat;
}

bool Macros_ConsumeBool(parser_context_t* ctx)
{
    macro_variable_t res = consumeValue(ctx);
    return coalesceType(ctx, res, MacroVariableType_Bool).asBool;
}

macro_variable_t Macros_ConsumeAnyValue(parser_context_t *ctx)
{
    return consumeValue(ctx);
}

macro_result_t Macros_ProcessSetVarCommand(parser_context_t* ctx)
{
    macro_variable_t* dst = consumeVarAndAllocate(ctx);
    macro_variable_t src = consumeValue(ctx);

    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    if (dst != NULL) {
        dst->type = src.type;
        dst->asInt = src.asInt;
    }

    return MacroResult_Finished;
}

void Macros_SerializeVar(char* buffer, uint8_t len, macro_variable_t var) {
    switch (var.type) {
        case MacroVariableType_Float: {
            float intPart = 0;
            float fraPart = modff(var.asFloat, &intPart);
            int32_t intPartAsInt = (int32_t)intPart;
            int32_t fraPartAsInt = (int32_t)(fraPart * 1000 / 1);

            snprintf(buffer, len, "%" PRId32 ".%" PRId32 , intPartAsInt, fraPartAsInt);
            break;
        }
        case MacroVariableType_Int:
            snprintf(buffer, len, "%" PRId32, var.asInt);
            break;
        case MacroVariableType_Bool:
            snprintf(buffer, len, "%s", var.asBool ? "true" : "false");
            break;
        case MacroVariableType_String: {
            parser_context_t ctx = CreateStringRefContext(var.asStringRef);

            uint16_t stringOffset = 0;
            uint16_t textIndex = 0;
            uint16_t textSubIndex = 0;
            uint8_t bufferIndex = 0;

            char c;
            while (bufferIndex < len - 1) {
                c = Macros_ConsumeCharOfString(&ctx, &stringOffset, &textIndex, &textSubIndex);
                if (c == '\0') {
                    break;
                }
                buffer[bufferIndex++] = c;
            }
            buffer[bufferIndex] = '\0';
            break;
        }
        default:
            Macros_ReportErrorNum("Unexpected variable type:", var.type, NULL);
            break;
    }
}


ATTR_UNUSED static void test(const char* command, macro_variable_t expectedResult, const char* comment) {
#ifdef __ZEPHYR__
    Macros_ParserError = false;
    parser_context_t ctx = {
        .at = command,
        .begin = command,
        .end = command + strlen(command),
        .macroState = NULL,
        .nestingLevel = 0,
        .nestingBound = 0,
    };
    macro_variable_t res = Macros_ConsumeAnyValue(&ctx);

    bool isExpected = true;

    if (expectedResult.type != MacroVariableType_None) {
        macro_variable_t eq = computeEqOperation(res, Operator_Eq, &ctx, expectedResult);
        isExpected = eq.asBool;
    }

    if (!isExpected || Macros_ParserError) {
        LogU("  Test failed: '%s': %s\n", command, comment);
    } else {
        LogU("  Test succes: '%s': %s\n", command, comment);
    }
#endif
}

void MacroVariables_Reset(void) {
    macroVariableCount = 0;
}


void MacroVariables_RunTests(void) {
#ifdef __ZEPHYR__
    LogU("Running macro variable parser tests!\n");
    test("($leds.brightness / 1.5)", noneVar(), "Reads numeric expressions");
    test("$bluetooth.enabled", boolVar(Cfg.Bt_Enabled), "Reads bluetooth enabled");
    test("!$bluetooth.enabled", boolVar(!Cfg.Bt_Enabled), "Reads top level parentheses");
    test("(!$bluetooth.enabled)", boolVar(!Cfg.Bt_Enabled), "Reads negation");
    LogU("  tests finished!\n");
#endif
}

static macro_variable_t consumeArgumentAsValue(parser_context_t* ctx) {
    ConsumeUntilDot(ctx);
    uint8_t argId = Macros_ConsumeInt(ctx);

    if (S->ms.currentMacroArgumentOffset == 0) {
        Macros_ReportErrorPrintf(ctx->at, "Failed to retrieve argument %d, because this macro doesn't seem to have arguments assigned!", argId);
    }

    string_segment_t str = ParseMacroArgument(S->ms.currentMacroArgumentOffset, argId);

    if (str.start == NULL) {
        Macros_ReportErrorPrintf(ctx->at, "Failed to retrieve argument %d. Argument not found!", argId);
        return noneVar();
    }

    parser_context_t varCtx = (parser_context_t) {
        .at = str.start,
        .begin = str.start,
        .end = str.end,
        .macroState = ctx->macroState,
        .nestingLevel = ctx->nestingLevel,
        .nestingBound = ctx->nestingBound,
    };

    macro_variable_t res = consumeValue(&varCtx);

    return res;
}

static bool expandArgumentInplace(parser_context_t* ctx, uint8_t argNumber) {
    if (S->ms.currentMacroArgumentOffset == 0) {
        Macros_ReportErrorPrintf(ctx->at, "Failed to retrieve argument %d, because this macro doesn't seem to have arguments assigned!", argNumber);
    }

    string_segment_t str = ParseMacroArgument(S->ms.currentMacroArgumentOffset, argNumber);

    if (str.start == NULL) {
        // If this kind of substitution is not found, assume it is empty and do *not* throw an error.
        // Macros_ReportErrorPrintf(ctx->at, "Failed to retrieve argument %d. Argument not found!", argNumber);
        return false;
    }

    return PushParserContext(ctx, str.start, str.start, str.end);
}

bool TryExpandMacroTemplateOnce(parser_context_t* ctx) {
    ASSERT(*ctx->at == '&');

    ctx->at++;

    Trace_Printc("e1");

    if (ConsumeToken(ctx, "macroArg")) {
        ConsumeUntilDot(ctx);
        uint8_t argId = Macros_ConsumeInt(ctx);
        expandArgumentInplace(ctx, argId);
        return true;
    }

    ctx->at--;
    return false;
}

// ----------------------------------------
// macroArguments allocation and processing
// ----------------------------------------

string_ref_t createStringRef(const char *start, const char *end) {
    return (string_ref_t) {
        .offset = start - (const char*)ValidatedUserConfigBuffer.buffer,
        .len = (uint8_t)(end - start),
    };
}

string_segment_t stringRefToSegment(string_ref_t ref) {
    return (string_segment_t) {
        .start = (const char*)(ValidatedUserConfigBuffer.buffer + ref.offset),
        .end = (const char*)(ValidatedUserConfigBuffer.buffer + ref.offset + ref.len),
    };
}

const char *stringRefStart(string_ref_t ref) {
    return (const char *)(ValidatedUserConfigBuffer.buffer + ref.offset);
}

const char *stringRefEnd(string_ref_t ref) {
    return (const char *)(ValidatedUserConfigBuffer.buffer + ref.offset + ref.len);
}

macro_argument_alloc_result_t Macros_AllocateMacroArgument(
    macro_state_t *owner,
    const char *idStart, 
    const char *idEnd, 
    macro_argument_type_t type,
    uint8_t argNumber,
    macro_argref_t* outArgRef
) {
    // search for existing argument of same owner with the same identifier, error if found
    for (uint8_t i = 0; i < MACRO_ARGUMENT_POOL_SIZE; i++) {
        if (macroArguments[i].type != MacroArgType_Unused && macroArguments[i].owner == owner &&
            SegmentEqual(stringRefToSegment(macroArguments[i].name), (string_segment_t){ .start = idStart, .end = idEnd }) {
            return MacroArgAllocResult_DuplicateArgumentName;
        }
    }

    // search for an unused slot in the pool
    for (uint8_t i = 0; i < MACRO_ARGUMENT_POOL_SIZE; i++) {
        if (macroArguments[i].type == MacroArgType_Unused) {
            macroArguments[i].owner = owner;
            macroArguments[i].type = type;
            macroArguments[i].id = argNumber;
            macroArguments[i].name = createStringRef(idStart, idEnd);
            *outArgRef = i;
            return MacroArgAllocResult_Success;
        }
    }

    return MacroArgAllocResult_PoolLimitExceeded;
}

macro_argument_t *Macros_FindMacroArgumentByName(macro_state_t *owner, const char *nameStart, const char *nameEnd) {
    for (uint8_t i = 0; i < MACRO_ARGUMENT_POOL_SIZE; i++) {
        if (macroArguments[i].type != MacroArgType_Unused && macroArguments[i].owner == owner &&
            SegmentEqual(stringRefToSegment(macroArguments[i].name), (string_segment_t){ .start = nameStart, .end = nameEnd })) {
            return &macroArguments[i];
        }
    }
    return NULL;
}
