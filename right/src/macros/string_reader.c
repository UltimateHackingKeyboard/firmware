#include "macros/string_reader.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "macros/vars.h"

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef enum {
   StringType_Raw,
   StringType_DoubleQuote,
   StringType_SingleQuote,
} string_type_t;


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

static char consumeExpressionChar(parser_context_t* ctx, uint16_t* index)
{
    macro_variable_t res = Macros_ConsumeAnyValue(ctx);
    UnconsumeWhite(ctx);
    char c;

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

char Macros_ConsumeCharOfString(parser_context_t* ctx, uint16_t* stringOffset, uint16_t* index, uint16_t* subIndex)
{
    const char* a = ctx->at;
    const char* aEnd = ctx->end;

    a += *stringOffset;

    string_type_t stringType;
    switch (*a) {
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

    a += *index;

    if (a == aEnd) {
        ctx->at = ctx->end;
        return '\0';
    }

    switch(*a) {
        case '\\':
            if (stringType == StringType_SingleQuote || a+1 == aEnd) {
                goto normalChar;
            } else {
                (*index)++;
                a++;
                switch (*a) {
                    case 'n':
                        (*index)++;
                        return '\n';
                    default:
                        (*index)++;
                        return *a;
                }
            }
        case '"':
            if (stringType == StringType_DoubleQuote) {
                a++;
                (*index)++;
                return tryConsumeAnotherStringLiteral(ctx, stringOffset, index, subIndex);
            } else {
                goto normalChar;
            }
        case '\'':
            if (stringType == StringType_SingleQuote) {
                a++;
                (*index)++;
                return tryConsumeAnotherStringLiteral(ctx, stringOffset, index, subIndex);
            } else {
                goto normalChar;
            }
        case '\n':
            return '\0';
        case '$':
            if (stringType == StringType_SingleQuote) {
                goto normalChar;
            } else {
                parser_context_t ctx2 = { .macroState = ctx->macroState, .begin = ctx->begin, .at = a, .end = aEnd };
                ConsumeCommentsAsWhite(false);
                char res = consumeExpressionChar(&ctx2, subIndex);
                ConsumeCommentsAsWhite(true);
                if (*subIndex == 0) {
                    *index += ctx2.at - a;
                }
                return res;
            }
        default:
        normalChar:
            (*index)++;
            return *a;
    }
}
