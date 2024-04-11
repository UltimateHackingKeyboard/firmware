#include "macros/core.h"
#include "macros/status_buffer.h"
#include "macros/keyid_parser.h"
#include "str_utils.h"
#include <stddef.h>
#include <stdint.h>
#include "debug.h"


typedef struct {
    const char* id;
    uint8_t keyId;
} lookup_record_t;

static const lookup_record_t lookup_table[] = {
        // ALWAYS keep the array sorted by `LC_ALL=C sort`
        {"", 255},
        {"'", 19},
        {",", 24},
        {"-", 4},
        {".", 25},
        {"/", 26},
        {"0", 3},
        {"1", 65},
        {"2", 66},
        {"3", 67},
        {"4", 68},
        {"5", 69},
        {"6", 70},
        {"7", 0},
        {"8", 1},
        {"9", 2},
        {";", 18},
        {"=", 5},
        {"[", 11},
        {"]", 12},
        {"`", 64},
        {"a", 79},
        {"apostropheAndQuote", 19},
        {"b", 91},
        {"backspace", 6},
        {"c", 89},
        {"capsLock", 78},
        {"closingBracketAndClosingBrace", 12},
        {"commaAndLessThanSign", 24},
        {"d", 81 },
        {"delete", 42},
        {"dotAndGreaterThanSign", 25},
        {"downArrow", 54},
        {"e", 74 },
        {"end", 48},
        {"enter", 20},
        {"equalAndPlus", 5},
        {"escape", 98},
        {"f", 82},
        {"f1", 99},
        {"f10", 38},
        {"f11", 39},
        {"f12", 40},
        {"f2", 100},
        {"f3", 101},
        {"f4", 102},
        {"f5", 103},
        {"f6", 104},
        {"f7", 35},
        {"f8", 36},
        {"f9", 37},
        {"g", 84},
        {"graveAccentAndTilde", 63},
        {"h", 21},
        {"home", 46},
        {"i", 8},
        {"insert", 43},
        {"isoKey", 86},
        {"j", 15},
        {"k", 16},
        {"l", 17},
        {"leftAlt", 94},
        {"leftArrow", 53},
        {"leftCtrl", 92},
        {"leftFn", 95},
        {"leftFn2", 105},
        {"leftMod", 97},
        {"leftModule.key1", 128},
        {"leftModule.key2", 129},
        {"leftModule.key3", 130},
        {"leftModule.leftButton", 131},
        {"leftModule.middleButton", 132},
        {"leftModule.rightButton", 133},
        {"leftMouse", 78},
        {"leftShift", 85},
        {"leftSpace", 96},
        {"leftSuper", 93},
        {"m", 23},
        {"minusAndUnderscore", 4},
        {"n", 22},
        {"next", 52},
        {"o", 9},
        {"openingBracketAndOpeningBrace", 11},
        {"p", 10},
        {"pageDown", 49},
        {"pageUp", 47},
        {"pause", 45},
        {"previous", 50},
        {"print", 41},
        {"q", 72},
        {"r", 75},
        {"rightAlt", 32},
        {"rightArrow", 55},
        {"rightCtrl", 34},
        {"rightFn", 31},
        {"rightFn2", 56},
        {"rightMod", 30},
        {"rightModule.leftButton", 192},
        {"rightModule.rightButton", 193},
        {"rightShift", 27},
        {"rightSpace", 29},
        {"rightSuper", 33},
        {"s", 80},
        {"scrollLock", 44},
        {"semicolonAndColon", 18},
        {"slashAndQuestionMark", 26},
        {"t", 77},
        {"tab", 71},
        {"u", 7},
        {"upArrow", 51},
        {"v", 90},
        {"w", 73},
        {"x", 88},
        {"y", 14},
        {"z", 87},
        {"|", 13},
};

static size_t lookup_size = sizeof(lookup_table)/sizeof(lookup_table[0]);

static void testLookup()
{
    for (uint8_t i = 0; i < lookup_size - 1; i++) {
        if (!StrLessOrEqual(lookup_table[i].id, NULL, lookup_table[i+1].id, NULL)) {
            Macros_ReportError("Keyid table is not properly sorted!", lookup_table[i].id, NULL);
        }
    }
}

void KeyIdParser_initialize()
{
    testLookup();
}

static const lookup_record_t* lookup(uint8_t begin, uint8_t end, const char* str, const char* strEnd)
{
    uint8_t pivot = begin + (end-begin)/2;
    if (begin == end) {
        if (StrLessOrEqual(str, strEnd, lookup_table[pivot].id, NULL) && StrLessOrEqual(lookup_table[pivot].id, NULL, str, strEnd)) {
            return &lookup_table[pivot];
        } else {
            return NULL;
        }
    }
    else if (StrLessOrEqual(str, strEnd, lookup_table[pivot].id, NULL)) {
        return lookup(begin, pivot, str, strEnd);
    } else {
        return lookup(pivot + 1, end, str, strEnd);
    }
}

uint8_t MacroKeyIdParser_TryConsumeKeyId(parser_context_t* ctx)
{
    const lookup_record_t* record = lookup(0, lookup_size-1, ctx->at, IdentifierEnd(ctx));

    if (record == NULL) {
        return 255;
    }

    ConsumeToken(ctx, record->id);

    return record->keyId;
}

const char* MacroKeyIdParser_KeyIdToAbbreviation(uint8_t keyId)
{
    for (uint8_t i = 0; i < lookup_size - 1; i++) {
        if (lookup_table[i].keyId == keyId) {
            return lookup_table[i].id;
        }
    }
    return "?";
}
