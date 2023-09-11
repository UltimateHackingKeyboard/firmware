#include "macros_vars.h"
#include "postponer.h"
#include "macro_keyid_parser.h"
#include "utils.h"
#include "str_utils.h"
#include "macros.h"
#include <stdint.h>
#include <stdio.h>
#include "config_parser/config_globals.h"
#include "debug.h"
#include "macro_set_command.h"

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

static macro_variable_t consumeParenthessExpression(parser_context_t* ctx);
static macro_variable_t consumeValue(parser_context_t* ctx);
static macro_variable_t negate(parser_context_t *ctx, macro_variable_t res);
static macro_variable_t consumeMinMaxOperation(parser_context_t* ctx, operator_t op);
static macro_variable_t negateBool(parser_context_t *ctx, macro_variable_t res);

static macro_variable_t intVar(int32_t value)
{
    return (macro_variable_t) { .asInt = value, .type = MacroVariableType_Int };
}

static macro_variable_t boolVar(bool value)
{
    return (macro_variable_t) { .asBool = value, .type = MacroVariableType_Bool };
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
        Macros_ReportError("Numeric value expected", ctx->at, ctx->end);
        return noneVar();
    }

    ConsumeWhite(ctx);
    return res;
}


macro_variable_t* Macros_ConsumeExistingWritableVariable(parser_context_t* ctx)
{
    //TODO: optimize this!
    for (uint8_t i = 0; i < macroVariableCount; i++) {
        if (ConsumeIdentifierByRef(ctx, macroVariables[i].name)) {
            return &macroVariables[i];
        }
    }
    Macros_ReportError("Variable not found:", ctx->at, ctx->end);
    return NULL;
}

// Expects <variable name>
static macro_variable_t consumeVariable(parser_context_t* ctx)
{
    //TODO: handle indirection, e.g., `$$<variable name>`

    if (Macros_DryRun) {
        if (!IsIdentifierChar(*ctx->at)) {
            Macros_ReportError("Identifier expected here:", ctx->at, ctx->at);
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

    Macros_ReportError("Variable not found:", ctx->at, ctx->end);
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

    parser_context_t bakCtx = *ctx;
    macro_variable_t configVal = Macro_TryReadConfigVal(ctx);

    if (configVal.type != MacroVariableType_None) {
        Macros_ReportError("Name already taken by configuration value:", bakCtx.at,  bakCtx.end);
        return NULL;
    }

    uint16_t len = IdentifierEnd(ctx) - ctx->at;
    if (len > 255) {
        Macros_ReportError("Variable name too long:", ctx->at,  ctx->end);
        return NULL;
    }

    if (Macros_DryRun) {
        ConsumeAnyIdentifier(ctx);
        return NULL;
    }

    if (macroVariableCount == MACRO_VARIABLE_COUNT_MAX) {
        Macros_ReportError("Too many variables. Can't allocate:", ctx->at, ctx->end);
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
        return intVar(ctx->macroState->ms.commandAddress);
    }
    else if (ConsumeToken(ctx, "queuedKeyId")) {
        ConsumeUntilDot(ctx);
        int8_t queueIdx = Macros_LegacyConsumeInt(ctx);
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
            Macros_ReportError("KeyId abbreviation expected", ctx->at, ctx->end);
            return noneVar();
        }
        return intVar(keyId);
    }
    else if ((res = Macro_TryReadConfigVal(ctx)).type != MacroVariableType_None) {
        return res;
    } else {
        return consumeVariable(ctx);
    }
}

static macro_variable_t consumeValue(parser_context_t* ctx)
{
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
        default:
            goto failed;
    }

failed:
    if (IsIdentifierChar(*ctx->at)) {
        Macros_ReportErrorPrintf(ctx->at, "Parsing failed, did you mean '$%s'?", OneWord(ctx));
    } else {
        Macros_ReportError("Could not parse", ctx->at, ctx->end);
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
                Macros_ReportError("Division by zero attempted!", opCtx->at, opCtx->at);
                return 0;
            }
            return a/b;
        case Operator_Mod:
            Macros_ReportError("Modulo is not supported for floats!", opCtx->at, opCtx->at);
            return 1.0f;
        case Operator_Min:
            return MIN(a, b);
        case Operator_Max:
            return MAX(a, b);
        default:
            Macros_ReportError("Unexpected operation: ", opCtx->at, opCtx->end);
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
                Macros_ReportError("Division by zero attempted!", opCtx->at, opCtx->at);
                return 0;
            }
            return a/b;
        case Operator_Mod:
            if (b == 0) {
                Macros_ReportError("Modulo by zero attempted!", opCtx->at, opCtx->at);
                return 0;
            }
            return a%b;
        case Operator_Min:
            return MIN(a, b);
        case Operator_Max:
            return MAX(a, b);
        default:
            Macros_ReportError("Unexpected operation: ", opCtx->at, opCtx->end);
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
            Macros_ReportError("Unexpected operation: ", opCtx->at, opCtx->end);
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
            Macros_ReportError("Unexpected operation: ", opCtx->at, opCtx->end);
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
            Macros_ReportError("Unexpected operation: ", opCtx->at, opCtx->end);
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
        Macros_ReportError("Expected parameter list.", ctx->at, ctx->at);
        return noneVar();
    }
    macro_variable_t accumulator = consumeValue(ctx);

    while (ConsumeToken(ctx, ",")) {
        macro_variable_t var = consumeValue(ctx);
        accumulator = computeOperation(accumulator, op, ctx, var);
    }

    if (!ConsumeToken(ctx, ")"))
    {
        Macros_ReportError("Expected closing parenthess at the end of parameter list.", ctx->at, ctx->at);
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
        parser_context_t opCtx = *ctx;
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
        parser_context_t opCtx = *ctx;
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
    parser_context_t opCtx = *ctx;
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
        parser_context_t opCtx = *ctx;
        if (ConsumeToken(ctx, "&&")) {
            op = Operator_Or;
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
        parser_context_t opCtx = *ctx;
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
                Macros_ReportError("Failed to parse expression, closing parenthess expected.", ctx->at, ctx->at);
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

