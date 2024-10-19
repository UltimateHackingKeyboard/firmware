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
        {"'", 20},
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
        {";", 19},
        {"=", 5},
        {"[", 12},
        {"]", 13},
        {"`", 64},
        {"a", 78},
        {"apostropheAndQuote", 20},
        {"b", 89},
        {"backspace", 6},
        {"c", 87},
        {"capsLock", 77},
        {"closingBracketAndClosingBrace", 13},
        {"commaAndLessThanSign", 24},
        {"d", 80 },
        {"delete", 42},
        {"dotAndGreaterThanSign", 25},
        {"downArrow", 54},
        {"e", 74 },
        {"end", 48},
        {"enter", 21},
        {"equalAndPlus", 5},
        {"escape", 97},
        {"f", 81},
        {"f1", 98},
        {"f10", 38},
        {"f11", 39},
        {"f12", 40},
        {"f2", 99},
        {"f3", 100},
        {"f4", 101},
        {"f5", 102},
        {"f6", 103},
        {"f7", 35},
        {"f8", 36},
        {"f9", 37},
        {"g", 82},
        {"graveAccentAndTilde", 64},
        {"h", 15},
        {"home", 46},
        {"i", 9},
        {"insert", 43},
        {"isoKey", 84},
        {"j", 16},
        {"k", 17},
        {"l", 18},
        {"leftAlt", 92},
        {"leftArrow", 53},
        {"leftCtrl", 90},
        {"leftFn", 93},
        {"leftFn2", 96},
        {"leftMod", 94},
        {"leftModule.key1", 128},
        {"leftModule.key2", 129},
        {"leftModule.key3", 130},
        {"leftModule.leftButton", 131},
        {"leftModule.middleButton", 132},
        {"leftModule.rightButton", 133},
        {"leftMouse", 77},
        {"leftShift", 83},
        {"leftSpace", 95},
        {"leftSuper", 91},
        {"m", 23},
        {"minusAndUnderscore", 4},
        {"n", 22},
        {"next", 52},
        {"o", 10},
        {"openingBracketAndOpeningBrace", 12},
        {"p", 11},
        {"pageDown", 49},
        {"pageUp", 47},
        {"pause", 45},
        {"previous", 50},
        {"print", 41},
        {"q", 72},
        {"r", 75},
        {"rightAlt", 30},
        {"rightArrow", 55},
        {"rightCtrl", 32},
        {"rightFn", 29},
        {"rightFn2", 34},
        {"rightMod", 33},
        {"rightModule.leftButton", 192},
        {"rightModule.rightButton", 193},
        {"rightShift", 27},
        {"rightSpace", 28},
        {"rightSuper", 31},
        {"s", 79},
        {"scrollLock", 44},
        {"semicolonAndColon", 19},
        {"slashAndQuestionMark", 26},
        {"t", 76},
        {"tab", 71},
        {"u", 8},
        {"upArrow", 51},
        {"v", 88},
        {"w", 73},
        {"x", 86},
        {"y", 7},
        {"z", 85},
        {"|", 14},
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
    // this gets identifier till the next dot only
    const char* end1 = IdentifierEnd(ctx);
    const lookup_record_t* record = lookup(0, lookup_size-1, ctx->at, end1);

    // if failed, try consume with dot
    if (record == NULL && *end1 == '.' && end1+1 < ctx->end) {
        parser_context_t ctx2 = *ctx;
        ctx2.at = end1 + 1;
        const char* end2 = IdentifierEnd(&ctx2);
        record = lookup(0, lookup_size-1, ctx->at, end2);
    }

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
