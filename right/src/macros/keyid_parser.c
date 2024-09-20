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
        {"7", 0},
        {"8", 1},
        {"9", 2},
        {"0", 3},
        {"-", 4},
        {"minusAndUnderscore", 4},
        {"=", 5},
        {"equalAndPlus", 5},
        {"backspace", 6},
        {"y", 7},
        {"u", 8},
        {"i", 9},
        {"o", 10},
        {"p", 11},
        {"[", 12},
        {"openingBracketAndOpeningBrace", 12},
        {"]", 13},
        {"closingBracketAndClosingBrace", 13},
        {"|", 14},
        {"h", 15},
        {"j", 16},
        {"k", 17},
        {"l", 18},
        {";", 19},
        {"semicolonAndColon", 19},
        {"'", 20},
        {"apostropheAndQuote", 20},
        {"enter", 21},
        {"n", 22},
        {"m", 23},
        {",", 24},
        {"commaAndLessThanSign", 24},
        {".", 25},
        {"dotAndGreaterThanSign", 25},
        {"/", 26},
        {"slashAndQuestionMark", 26},
        {"rightShift", 27},
        {"rightSpace", 28},
        {"rightFn", 29},
        {"rightAlt", 30},
        {"rightSuper", 31},
        {"rightCtrl", 32},
        {"rightMod", 33},
        {"rightFn2", 34},
        {"f7", 35},
        {"f8", 36},
        {"f9", 37},
        {"f10", 38},
        {"f11", 39},
        {"f12", 40},
        {"print", 41},
        {"delete", 42},
        {"insert", 43},
        {"scrollLock", 44},
        {"pause", 45},
        {"home", 46},
        {"pageUp", 47},
        {"end", 48},
        {"pageDown", 49},
        {"previous", 50},
        {"upArrow", 51},
        {"next", 52},
        {"leftArrow", 53},
        {"downArrow", 54},
        {"rightArrow", 55},
        {"graveAccentAndTilde", 64},
        {"`", 64},
        {"1", 65},
        {"2", 66},
        {"3", 67},
        {"4", 68},
        {"5", 69},
        {"6", 70},
        {"tab", 71},
        {"q", 72},
        {"w", 73},
        {"e", 74 },
        {"r", 75},
        {"t", 76},
        {"capsLock", 77},
        {"leftMouse", 77},
        {"a", 78},
        {"s", 79},
        {"d", 80 },
        {"f", 81},
        {"g", 82},
        {"leftShift", 83},
        {"isoKey", 84},
        {"z", 85},
        {"x", 86},
        {"c", 87},
        {"v", 88},
        {"b", 89},
        {"leftCtrl", 90},
        {"leftSuper", 91},
        {"leftAlt", 92},
        {"leftFn", 93},
        {"leftMod", 94},
        {"leftSpace", 95},
        {"leftFn2", 96},
        {"escape", 97},
        {"f1", 98},
        {"f2", 99},
        {"f3", 100},
        {"f4", 101},
        {"f5", 102},
        {"f6", 103},
        {"leftModule.key1", 128},
        {"leftModule.key2", 129},
        {"leftModule.key3", 130},
        {"leftModule.leftButton", 131},
        {"leftModule.middleButton", 132},
        {"leftModule.rightButton", 133},
        {"rightModule.leftButton", 192},
        {"rightModule.rightButton", 193},
        {"", 255},
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
