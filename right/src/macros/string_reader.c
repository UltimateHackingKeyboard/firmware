#include "macros/string_reader.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "macros/vars.h"
#include "str_utils.h"

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef enum {
   StringType_Raw,
   StringType_DoubleQuote,
   StringType_SingleQuote,
} string_type_t;


static char Macros_ConsumeCharInString(parser_context_t* ctx, string_type_t stringType, const char* at, uint16_t* index, uint16_t* subIndex);

static char consumeExpressionCharOfInt(const macro_variable_t* variable, uint16_t* idx)
{
    char buffer[20];
    sprintf(buffer, "%" PRId32, variable->asInt);
    char res = buffer[*idx];
    if (buffer[*idx+1] == '\0') {
        *idx = 0;
    } else {
        (*idx)++;
    }
    return res;
}

static char consumeExpressionCharOfFloat(const macro_variable_t* variable, uint16_t* idx)
{
    // printf in our library does not handle floats... so lets handle it manually
    char buffer[20];
    float value = variable->asFloat;

    int integerPart = (int)value;
    float fractionalPart = value - integerPart;
    int8_t remainingPrecision = 3;
    bool firstSignificantEncountered = false;

    // Convert integer part to string
    int index = 0;
    do {
        buffer[index++] = '0' + integerPart % 10;
        firstSignificantEncountered |= integerPart;
        integerPart /= 10;
        remainingPrecision -= firstSignificantEncountered ? 1 : 0;
    } while (integerPart > 0);

    // Reverse the integer part in the buffer
    for (int i = 0; i < index / 2; i++) {
        char temp = buffer[i];
        buffer[i] = buffer[index - i - 1];
        buffer[index - i - 1] = temp;
    }

    // Add decimal point
    buffer[index++] = '.';

    remainingPrecision = MAX(1, remainingPrecision);

    // Convert fractional part
    while (remainingPrecision > 0) {
        fractionalPart *= 10;
        firstSignificantEncountered |= fractionalPart >= 1.0f;
        int digit = (int)fractionalPart;
        buffer[index++] = '0' + digit;
        fractionalPart -= digit;
        remainingPrecision -= firstSignificantEncountered ? 1 : 0;
    }

    // Null-terminate the string
    buffer[index] = '\0';

    char res = buffer[*idx];
    if (buffer[*idx+1] == '\0') {
        *idx = 0;
    } else {
        (*idx)++;
    }
    return res;
}

static char consumeExpressionCharOfBool(const macro_variable_t* variable, uint16_t* idx)
{
    if (variable->asBool) {
        return '1';
    } else {
        return '0';
    }
}

// Beware, this has high complexity
static char consumeCharOfTemplate(parser_context_t* ctx, string_type_t stringType, uint16_t* index)
{
    uint16_t nestedIndex = 0;
    uint16_t nestedSubIndex = 0;

    char c1 = '\0'; // char we are interested in
    char c2 = '\a'; // next char to know if we reached the end

    for (uint16_t i = 0; i < *index + 2 && c2; i++) {
        c1 = c2;
        c2 = Macros_ConsumeCharInString(ctx, stringType, ctx->at + nestedIndex, &nestedIndex, &nestedSubIndex);
    }

    if (c1 && c2) {
        (*index)++;
    } else {
        *index = 0;
    }

    return c1;
}

static char consumeExpressionChar(parser_context_t* ctx, string_type_t stringType, uint16_t* index)
{
    char c;
    if (TryExpandMacroTemplateOnce(ctx)) {
        // Call tree of this never expands or unexpands this context, so we can safely perform a pop after.
        // (If there is an expansion, it is handled within a new context copy.)
        c = consumeCharOfTemplate(ctx, stringType, index);
        PopParserContext(ctx);

        if (*index == 0) {
            UnconsumeWhite(ctx);
        }
        return c;
    } else {
        macro_variable_t res = Macros_ConsumeAnyValue(ctx);
        UnconsumeWhite(ctx);

        switch (res.type) {
            case MacroVariableType_Int:
                c = consumeExpressionCharOfInt(&res, index);
                break;
            case MacroVariableType_Float:
                c = consumeExpressionCharOfFloat(&res, index);
                break;
            case MacroVariableType_Bool:
                c = consumeExpressionCharOfBool(&res, index);
                break;
            case MacroVariableType_None:
                c = '?';
                break;
            default:
                Macros_ReportErrorNum("Unrecognized variable type", res.type, ctx->at);
                return '\0';
        }
    }

    if (Macros_ParserError) {
        ctx->at++;
        *index = 0;
    }
    return c;
}

static char tryConsumeAnotherStringLiteral(parser_context_t* ctx, uint16_t* stringOffset, uint16_t* index, uint16_t* subIndex)
{
    const char* a = ctx->at;
    const char* aEnd = ctx->end;
    a += *stringOffset;
    a += *index;

    if (a == aEnd) {
        ctx->at = a;
        return '\0';
    }

    switch (*a) {
        case '\'':
        case '"':
            *stringOffset = *stringOffset + *index;
            *index = 0;
            return Macros_ConsumeCharOfString(ctx, stringOffset, index, subIndex);
        default:
            ctx->at = a;
            ConsumeWhite(ctx);
            *index = 0;
            *stringOffset = 0;
            *subIndex = 0;
            return '\0';
    }
}

bool Macros_CompareStringToken(parser_context_t* ctx, string_segment_t str) {
    CTX_COPY(ctx2, *ctx);
    ctx2.nestingBound = ctx->nestingLevel;
    uint16_t stringOffset = 0, textIndex = 0, textSubIndex = 0;
    const char* str2 = str.start;

    char c1, c2;
    while (true) {
        c1 = Macros_ConsumeCharOfString(&ctx2, &stringOffset, &textIndex, &textSubIndex);
        c2 = *str2;

        bool c1Ended = c1 == '\0';
        bool c2Ended = c2 == '\0' || str2 >= str.end;

        if (c1Ended || c2Ended) {
            return c1Ended && c2Ended;
        }

        if (c1 != c2) {
            return false;
        }

        str2++;
    }
}

void Macros_ConsumeStringToken(parser_context_t* ctx) {
    uint16_t stringOffset = 0, textIndex = 0, textSubIndex = 0;
    char c = 'a';
    while (c != '\0') {
        c = Macros_ConsumeCharOfString(ctx, &stringOffset, &textIndex, &textSubIndex);
    }
}

/**
 * ctx->at: beginning of the string-typed argument
 * stringOffset: beginning of the
 *
 * ```
 * write 'Hello'" you $attribute $name!"
 * ------> ctx->at
 *       <-----> stringOffset
 *              <----> index
 *                   "greenish" = attribute
 *                    <--> subIndex
 * ```
 **/
char Macros_ConsumeCharOfString(parser_context_t* ctx, uint16_t* stringOffset, uint16_t* index, uint16_t* subIndex)
{
    const char* at = ctx->at;

    at += *stringOffset;

    string_type_t stringType;
    switch (*at) {
        case '\'':
            stringType = StringType_SingleQuote;
            break;
        case '"':
            stringType = StringType_DoubleQuote;
            break;
        default:
            stringType = StringType_Raw;
            break;
    }

    if (*index == 0 && stringType != StringType_Raw) {
        (*index)++;
    }

    at += *index;

    // (This is correct, we don't want a context pop here.)
    if (at == ctx->end) {
        ctx->at = ctx->end;
        return '\0';
    }

    char maybeRes = Macros_ConsumeCharInString(ctx, stringType, at, index, subIndex);

    if (maybeRes == '\0') {
        return tryConsumeAnotherStringLiteral(ctx, stringOffset, index, subIndex);
    } else {
        return maybeRes;
    }
}


static char Macros_ConsumeCharInString(parser_context_t* ctx, string_type_t stringType, const char* at, uint16_t* index, uint16_t* subIndex)
{
    if (at >= ctx->end) {
        return '\0';
    }

    switch(*at) {
        case '\\':
            if (stringType == StringType_SingleQuote || at+1 == ctx->end) {
                goto normalChar;
            } else {
                (*index)++;
                at++;
                switch (*at) {
                    case 'n':
                        (*index)++;
                        return '\n';
                    default:
                        (*index)++;
                        return *at;
                }
            }
        case '"':
            if (stringType == StringType_DoubleQuote) {
                at++;
                (*index)++;
                return '\0';
            } else {
                goto normalChar;
            }
        case '\'':
            if (stringType == StringType_SingleQuote) {
                at++;
                (*index)++;
                return '\0';
            } else {
                goto normalChar;
            }
        case '\n':
            return '\0';
        case '$':
            if (stringType == StringType_SingleQuote) {
                goto normalChar;
            } else {
                parser_context_t ctx2 = {
                    .macroState = ctx->macroState,
                    .begin = ctx->begin,
                    .at = at,
                    .end = ctx->end,
                    .nestingLevel = ctx->nestingLevel,
                    .nestingBound = ctx->nestingLevel,
                };
                ConsumeCommentsAsWhite(false);
                char res = consumeExpressionChar(&ctx2, stringType, subIndex);
                ConsumeCommentsAsWhite(true);

                if (ctx2.nestingLevel != ctx->nestingLevel) {
                    Macros_ReportError("Macro template has overflown expression boundary! Undefined behavior coming!", ctx2.at, ctx2.end);
                    while (ctx2.nestingLevel > ctx->nestingLevel && PopParserContext(&ctx2)) {
                    }
                    *index += 1;
                    return '$';
                }

                if (*subIndex == 0) {
                    *index += ctx2.at - at;
                }
                return res;
            }
        default:
        normalChar:
            (*index)++;
            return *at;
    }
}

