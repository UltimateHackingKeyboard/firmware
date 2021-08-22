#include "macros.h"
#include "arduino_hid/ConsumerAPI.h"
#include "arduino_hid/SystemAPI.h"
#include "config_parser/parse_keymap.h"
#include "config_parser/config_globals.h"
#include "macro_shortcut_parser.h"
#include "str_utils.h"


char MacroShortcutParser_ScancodeToCharacter(uint16_t scancode)
{
    switch (scancode) {
        case HID_KEYBOARD_SC_A ... HID_KEYBOARD_SC_Z:
            return scancode - HID_KEYBOARD_SC_A + 'a';
        default:
            return ' ';
    }
}

uint8_t MacroShortcutParser_CharacterToScancode(char character)
{
    switch (character) {
        case 'A' ... 'Z':
        case 'a' ... 'z':
            return HID_KEYBOARD_SC_A - 1 + (character & 0x1F);
        case '1' ... '9':
            return HID_KEYBOARD_SC_1_AND_EXCLAMATION - 1 + (character & 0x0F);
        case ')':
        case '0':
            return HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS;
        case '!':
            return HID_KEYBOARD_SC_1_AND_EXCLAMATION;
        case '@':
            return HID_KEYBOARD_SC_2_AND_AT;
        case '#':
            return HID_KEYBOARD_SC_3_AND_HASHMARK;
        case '$':
            return HID_KEYBOARD_SC_4_AND_DOLLAR;
        case '%':
            return HID_KEYBOARD_SC_5_AND_PERCENTAGE;
        case '^':
            return HID_KEYBOARD_SC_6_AND_CARET;
        case '&':
            return HID_KEYBOARD_SC_7_AND_AMPERSAND;
        case '*':
            return HID_KEYBOARD_SC_8_AND_ASTERISK;
        case '(':
            return HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS;
        case '`':
        case '~':
            return HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE;
        case '[':
        case '{':
            return HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE;
        case ']':
        case '}':
            return HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE;
        case ';':
        case ':':
            return HID_KEYBOARD_SC_SEMICOLON_AND_COLON;
        case '\'':
        case '\"':
            return HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE;
        case '+':
        case '=':
            return HID_KEYBOARD_SC_EQUAL_AND_PLUS;
        case '\\':
        case '|':
            return HID_KEYBOARD_SC_BACKSLASH_AND_PIPE;
        case '.':
        case '>':
            return HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN;
        case ',':
        case '<':
            return HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN;
        case '/':
        case '\?':
            return HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK;
        case '-':
        case '_':
            return HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE;
        case '\n':
            return HID_KEYBOARD_SC_ENTER;
        case ' ':
            return HID_KEYBOARD_SC_SPACE;
    }
    return 0;
}

bool MacroShortcutParser_CharacterToShift(char character)
{
    switch (character) {
        case 'A' ... 'Z':
        case ')':
        case '!':
        case '@':
        case '#':
        case '$':
        case '%':
        case '^':
        case '&':
        case '*':
        case '(':
        case '~':
        case '{':
        case '}':
        case ':':
        case '\"':
        case '+':
        case '|':
        case '>':
        case '<':
        case '\?':
        case '_':
            return true;
    }
    return false;
}

lookup_record_t lookup_table[] = {
        {"", 0, scType_basic},
        {"enter", HID_KEYBOARD_SC_ENTER, scType_basic},
        {"escape", HID_KEYBOARD_SC_ESCAPE, scType_basic},
        {"backspace", HID_KEYBOARD_SC_BACKSPACE, scType_basic},
        {"tab", HID_KEYBOARD_SC_TAB, scType_basic},
        {"space", HID_KEYBOARD_SC_SPACE, scType_basic},
        {"minusAndUnderscore", HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE, scType_basic},
        {"equalAndPlus", HID_KEYBOARD_SC_EQUAL_AND_PLUS, scType_basic},
        {"openingBracketAndOpeningBrace", HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE, scType_basic},
        {"closingBracketAndClosingBrace", HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE, scType_basic},
        {"backslashAndPipe", HID_KEYBOARD_SC_BACKSLASH_AND_PIPE, scType_basic},
        {"nonUsHashmarkAndTilde", HID_KEYBOARD_SC_NON_US_HASHMARK_AND_TILDE, scType_basic},
        {"semicolonAndColon", HID_KEYBOARD_SC_SEMICOLON_AND_COLON, scType_basic},
        {"apostropheAndQuote", HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE, scType_basic},
        {"graveAccentAndTilde", HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE, scType_basic},
        {"commaAndLessThanSign", HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN, scType_basic},
        {"dotAndGreaterThanSign", HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN, scType_basic},
        {"slashAndQuestionMark", HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK, scType_basic},
        {"capsLock", HID_KEYBOARD_SC_CAPS_LOCK, scType_basic},
        {"f1", HID_KEYBOARD_SC_F1, scType_basic},
        {"f2", HID_KEYBOARD_SC_F2, scType_basic},
        {"f3", HID_KEYBOARD_SC_F3, scType_basic},
        {"f4", HID_KEYBOARD_SC_F4, scType_basic},
        {"f5", HID_KEYBOARD_SC_F5, scType_basic},
        {"f6", HID_KEYBOARD_SC_F6, scType_basic},
        {"f7", HID_KEYBOARD_SC_F7, scType_basic},
        {"f8", HID_KEYBOARD_SC_F8, scType_basic},
        {"f9", HID_KEYBOARD_SC_F9, scType_basic},
        {"f10", HID_KEYBOARD_SC_F10, scType_basic},
        {"f11", HID_KEYBOARD_SC_F11, scType_basic},
        {"f12", HID_KEYBOARD_SC_F12, scType_basic},
        {"printScreen", HID_KEYBOARD_SC_PRINT_SCREEN, scType_basic},
        {"scrollLock", HID_KEYBOARD_SC_SCROLL_LOCK, scType_basic},
        {"pause", HID_KEYBOARD_SC_PAUSE, scType_basic},
        {"insert", HID_KEYBOARD_SC_INSERT, scType_basic},
        {"home", HID_KEYBOARD_SC_HOME, scType_basic},
        {"pageUp", HID_KEYBOARD_SC_PAGE_UP, scType_basic},
        {"delete", HID_KEYBOARD_SC_DELETE, scType_basic},
        {"end", HID_KEYBOARD_SC_END, scType_basic},
        {"pageDown", HID_KEYBOARD_SC_PAGE_DOWN, scType_basic},
        {"rightArrow", HID_KEYBOARD_SC_RIGHT_ARROW, scType_basic},
        {"leftArrow", HID_KEYBOARD_SC_LEFT_ARROW, scType_basic},
        {"downArrow", HID_KEYBOARD_SC_DOWN_ARROW, scType_basic},
        {"upArrow", HID_KEYBOARD_SC_UP_ARROW, scType_basic},
        {"right", HID_KEYBOARD_SC_RIGHT_ARROW, scType_basic},
        {"left", HID_KEYBOARD_SC_LEFT_ARROW, scType_basic},
        {"down", HID_KEYBOARD_SC_DOWN_ARROW, scType_basic},
        {"up", HID_KEYBOARD_SC_UP_ARROW, scType_basic},
        {"numLock", HID_KEYBOARD_SC_NUM_LOCK, scType_basic},
        {"keypadSlash", HID_KEYBOARD_SC_KEYPAD_SLASH, scType_basic},
        {"keypadAsterisk", HID_KEYBOARD_SC_KEYPAD_ASTERISK, scType_basic},
        {"keypadMinus", HID_KEYBOARD_SC_KEYPAD_MINUS, scType_basic},
        {"keypadPlus", HID_KEYBOARD_SC_KEYPAD_PLUS, scType_basic},
        {"keypadEnter", HID_KEYBOARD_SC_KEYPAD_ENTER, scType_basic},
        {"keypad1AndEnd", HID_KEYBOARD_SC_KEYPAD_1_AND_END, scType_basic},
        {"keypad2AndDownArrow", HID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW, scType_basic},
        {"keypad3AndPageDown", HID_KEYBOARD_SC_KEYPAD_3_AND_PAGE_DOWN, scType_basic},
        {"keypad4AndLeftArrow", HID_KEYBOARD_SC_KEYPAD_4_AND_LEFT_ARROW, scType_basic},
        {"keypad5", HID_KEYBOARD_SC_KEYPAD_5, scType_basic},
        {"keypad6AndRightArrow", HID_KEYBOARD_SC_KEYPAD_6_AND_RIGHT_ARROW, scType_basic},
        {"keypad7AndHome", HID_KEYBOARD_SC_KEYPAD_7_AND_HOME, scType_basic},
        {"keypad8AndUpArrow", HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW, scType_basic},
        {"keypad9AndPageUp", HID_KEYBOARD_SC_KEYPAD_9_AND_PAGE_UP, scType_basic},
        {"keypad0AndInsert", HID_KEYBOARD_SC_KEYPAD_0_AND_INSERT, scType_basic},
        {"np1", HID_KEYBOARD_SC_KEYPAD_1_AND_END, scType_basic},
        {"np2", HID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW, scType_basic},
        {"np3", HID_KEYBOARD_SC_KEYPAD_3_AND_PAGE_DOWN, scType_basic},
        {"np4", HID_KEYBOARD_SC_KEYPAD_4_AND_LEFT_ARROW, scType_basic},
        {"np5", HID_KEYBOARD_SC_KEYPAD_5, scType_basic},
        {"np6", HID_KEYBOARD_SC_KEYPAD_6_AND_RIGHT_ARROW, scType_basic},
        {"np7", HID_KEYBOARD_SC_KEYPAD_7_AND_HOME, scType_basic},
        {"np8", HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW, scType_basic},
        {"np9", HID_KEYBOARD_SC_KEYPAD_9_AND_PAGE_UP, scType_basic},
        {"np0", HID_KEYBOARD_SC_KEYPAD_0_AND_INSERT, scType_basic},
        {"keypadDotAndDelete", HID_KEYBOARD_SC_KEYPAD_DOT_AND_DELETE, scType_basic},
        {"nonUsBackslashAndPipe", HID_KEYBOARD_SC_NON_US_BACKSLASH_AND_PIPE, scType_basic},
        {"backslashAndPipeIso", HID_KEYBOARD_SC_NON_US_BACKSLASH_AND_PIPE, scType_basic},
        {"application", HID_KEYBOARD_SC_APPLICATION, scType_basic},
        {"power", HID_KEYBOARD_SC_POWER, scType_basic},
        {"keypadEqualSign", HID_KEYBOARD_SC_KEYPAD_EQUAL_SIGN, scType_basic},
        {"f13", HID_KEYBOARD_SC_F13, scType_basic},
        {"f14", HID_KEYBOARD_SC_F14, scType_basic},
        {"f15", HID_KEYBOARD_SC_F15, scType_basic},
        {"f16", HID_KEYBOARD_SC_F16, scType_basic},
        {"f17", HID_KEYBOARD_SC_F17, scType_basic},
        {"f18", HID_KEYBOARD_SC_F18, scType_basic},
        {"f19", HID_KEYBOARD_SC_F19, scType_basic},
        {"f20", HID_KEYBOARD_SC_F20, scType_basic},
        {"f21", HID_KEYBOARD_SC_F21, scType_basic},
        {"f22", HID_KEYBOARD_SC_F22, scType_basic},
        {"f23", HID_KEYBOARD_SC_F23, scType_basic},
        {"f24", HID_KEYBOARD_SC_F24, scType_basic},
        {"execute", HID_KEYBOARD_SC_EXECUTE, scType_basic},
        {"help", HID_KEYBOARD_SC_HELP, scType_basic},
        {"menu", HID_KEYBOARD_SC_MENU, scType_basic},
        {"select", HID_KEYBOARD_SC_SELECT, scType_basic},
        {"stop", HID_KEYBOARD_SC_STOP, scType_basic},
        {"again", HID_KEYBOARD_SC_AGAIN, scType_basic},
        {"undo", HID_KEYBOARD_SC_UNDO, scType_basic},
        {"cut", HID_KEYBOARD_SC_CUT, scType_basic},
        {"copy", HID_KEYBOARD_SC_COPY, scType_basic},
        {"paste", HID_KEYBOARD_SC_PASTE, scType_basic},
        {"find", HID_KEYBOARD_SC_FIND, scType_basic},
        {"mute", HID_KEYBOARD_SC_MUTE, scType_basic},
        {"volumeUp", HID_KEYBOARD_SC_VOLUME_UP, scType_basic},
        {"volumeDown", HID_KEYBOARD_SC_VOLUME_DOWN, scType_basic},
        {"lockingCapsLock", HID_KEYBOARD_SC_LOCKING_CAPS_LOCK, scType_basic},
        {"lockingNumLock", HID_KEYBOARD_SC_LOCKING_NUM_LOCK, scType_basic},
        {"lockingScrollLock", HID_KEYBOARD_SC_LOCKING_SCROLL_LOCK, scType_basic},
        {"keypadComma", HID_KEYBOARD_SC_KEYPAD_COMMA, scType_basic},
        {"keypadEqualSignAs400", HID_KEYBOARD_SC_KEYPAD_EQUAL_SIGN_AS400, scType_basic},
        {"international1", HID_KEYBOARD_SC_INTERNATIONAL1, scType_basic},
        {"international2", HID_KEYBOARD_SC_INTERNATIONAL2, scType_basic},
        {"international3", HID_KEYBOARD_SC_INTERNATIONAL3, scType_basic},
        {"international4", HID_KEYBOARD_SC_INTERNATIONAL4, scType_basic},
        {"international5", HID_KEYBOARD_SC_INTERNATIONAL5, scType_basic},
        {"international6", HID_KEYBOARD_SC_INTERNATIONAL6, scType_basic},
        {"international7", HID_KEYBOARD_SC_INTERNATIONAL7, scType_basic},
        {"international8", HID_KEYBOARD_SC_INTERNATIONAL8, scType_basic},
        {"international9", HID_KEYBOARD_SC_INTERNATIONAL9, scType_basic},
        {"lang1", HID_KEYBOARD_SC_LANG1, scType_basic},
        {"lang2", HID_KEYBOARD_SC_LANG2, scType_basic},
        {"lang3", HID_KEYBOARD_SC_LANG3, scType_basic},
        {"lang4", HID_KEYBOARD_SC_LANG4, scType_basic},
        {"lang5", HID_KEYBOARD_SC_LANG5, scType_basic},
        {"lang6", HID_KEYBOARD_SC_LANG6, scType_basic},
        {"lang7", HID_KEYBOARD_SC_LANG7, scType_basic},
        {"lang8", HID_KEYBOARD_SC_LANG8, scType_basic},
        {"lang9", HID_KEYBOARD_SC_LANG9, scType_basic},
        {"alternateErase", HID_KEYBOARD_SC_ALTERNATE_ERASE, scType_basic},
        {"sysreq", HID_KEYBOARD_SC_SYSREQ, scType_basic},
        {"cancel", HID_KEYBOARD_SC_CANCEL, scType_basic},
        {"clear", HID_KEYBOARD_SC_CLEAR, scType_basic},
        {"prior", HID_KEYBOARD_SC_PRIOR, scType_basic},
        {"return", HID_KEYBOARD_SC_RETURN, scType_basic},
        {"separator", HID_KEYBOARD_SC_SEPARATOR, scType_basic},
        {"out", HID_KEYBOARD_SC_OUT, scType_basic},
        {"oper", HID_KEYBOARD_SC_OPER, scType_basic},
        {"clearAndAgain", HID_KEYBOARD_SC_CLEAR_AND_AGAIN, scType_basic},
        {"crselAndProps", HID_KEYBOARD_SC_CRSEL_AND_PROPS, scType_basic},
        {"exsel", HID_KEYBOARD_SC_EXSEL, scType_basic},
        {"keypad00", HID_KEYBOARD_SC_KEYPAD_00, scType_basic},
        {"keypad000", HID_KEYBOARD_SC_KEYPAD_000, scType_basic},
        {"thousandsSeparator", HID_KEYBOARD_SC_THOUSANDS_SEPARATOR, scType_basic},
        {"decimalSeparator", HID_KEYBOARD_SC_DECIMAL_SEPARATOR, scType_basic},
        {"currencyUnit", HID_KEYBOARD_SC_CURRENCY_UNIT, scType_basic},
        {"currencySubUnit", HID_KEYBOARD_SC_CURRENCY_SUB_UNIT, scType_basic},
        {"keypadOpeningParenthesis", HID_KEYBOARD_SC_KEYPAD_OPENING_PARENTHESIS, scType_basic},
        {"keypadClosingParenthesis", HID_KEYBOARD_SC_KEYPAD_CLOSING_PARENTHESIS, scType_basic},
        {"keypadOpeningBrace", HID_KEYBOARD_SC_KEYPAD_OPENING_BRACE, scType_basic},
        {"keypadClosingBrace", HID_KEYBOARD_SC_KEYPAD_CLOSING_BRACE, scType_basic},
        {"keypadTab", HID_KEYBOARD_SC_KEYPAD_TAB, scType_basic},
        {"keypadBackspace", HID_KEYBOARD_SC_KEYPAD_BACKSPACE, scType_basic},
        {"keypadA", HID_KEYBOARD_SC_KEYPAD_A, scType_basic},
        {"keypadB", HID_KEYBOARD_SC_KEYPAD_B, scType_basic},
        {"keypadC", HID_KEYBOARD_SC_KEYPAD_C, scType_basic},
        {"keypadD", HID_KEYBOARD_SC_KEYPAD_D, scType_basic},
        {"keypadE", HID_KEYBOARD_SC_KEYPAD_E, scType_basic},
        {"keypadF", HID_KEYBOARD_SC_KEYPAD_F, scType_basic},
        {"keypadXor", HID_KEYBOARD_SC_KEYPAD_XOR, scType_basic},
        {"keypadCaret", HID_KEYBOARD_SC_KEYPAD_CARET, scType_basic},
        {"keypadPercentage", HID_KEYBOARD_SC_KEYPAD_PERCENTAGE, scType_basic},
        {"keypadLessThanSign", HID_KEYBOARD_SC_KEYPAD_LESS_THAN_SIGN, scType_basic},
        {"keypadGreaterThanSign", HID_KEYBOARD_SC_KEYPAD_GREATER_THAN_SIGN, scType_basic},
        {"keypadAmp", HID_KEYBOARD_SC_KEYPAD_AMP, scType_basic},
        {"keypadAmpAmp", HID_KEYBOARD_SC_KEYPAD_AMP_AMP, scType_basic},
        {"keypadPipe", HID_KEYBOARD_SC_KEYPAD_PIPE, scType_basic},
        {"keypadPipePipe", HID_KEYBOARD_SC_KEYPAD_PIPE_PIPE, scType_basic},
        {"keypadColon", HID_KEYBOARD_SC_KEYPAD_COLON, scType_basic},
        {"keypadHashmark", HID_KEYBOARD_SC_KEYPAD_HASHMARK, scType_basic},
        {"keypadSpace", HID_KEYBOARD_SC_KEYPAD_SPACE, scType_basic},
        {"keypadAt", HID_KEYBOARD_SC_KEYPAD_AT, scType_basic},
        {"keypadExclamationSign", HID_KEYBOARD_SC_KEYPAD_EXCLAMATION_SIGN, scType_basic},
        {"keypadMemoryStore", HID_KEYBOARD_SC_KEYPAD_MEMORY_STORE, scType_basic},
        {"keypadMemoryRecall", HID_KEYBOARD_SC_KEYPAD_MEMORY_RECALL, scType_basic},
        {"keypadMemoryClear", HID_KEYBOARD_SC_KEYPAD_MEMORY_CLEAR, scType_basic},
        {"keypadMemoryAdd", HID_KEYBOARD_SC_KEYPAD_MEMORY_ADD, scType_basic},
        {"keypadMemorySubtract", HID_KEYBOARD_SC_KEYPAD_MEMORY_SUBTRACT, scType_basic},
        {"keypadMemoryMultiply", HID_KEYBOARD_SC_KEYPAD_MEMORY_MULTIPLY, scType_basic},
        {"keypadMemoryDivide", HID_KEYBOARD_SC_KEYPAD_MEMORY_DIVIDE, scType_basic},
        {"keypadPlusAndMinus", HID_KEYBOARD_SC_KEYPAD_PLUS_AND_MINUS, scType_basic},
        {"keypadClear", HID_KEYBOARD_SC_KEYPAD_CLEAR, scType_basic},
        {"keypadClearEntry", HID_KEYBOARD_SC_KEYPAD_CLEAR_ENTRY, scType_basic},
        {"keypadBinary", HID_KEYBOARD_SC_KEYPAD_BINARY, scType_basic},
        {"keypadOctal", HID_KEYBOARD_SC_KEYPAD_OCTAL, scType_basic},
        {"keypadDecimal", HID_KEYBOARD_SC_KEYPAD_DECIMAL, scType_basic},
        {"keypadHexadecimal", HID_KEYBOARD_SC_KEYPAD_HEXADECIMAL, scType_basic},
        {"leftControl", HID_KEYBOARD_SC_LEFT_CONTROL, scType_basic},
        {"leftShift", HID_KEYBOARD_SC_LEFT_SHIFT, scType_basic},
        {"leftAlt", HID_KEYBOARD_SC_LEFT_ALT, scType_basic},
        {"leftGui", HID_KEYBOARD_SC_LEFT_GUI, scType_basic},
        {"rightControl", HID_KEYBOARD_SC_RIGHT_CONTROL, scType_basic},
        {"rightShift", HID_KEYBOARD_SC_RIGHT_SHIFT, scType_basic},
        {"rightAlt", HID_KEYBOARD_SC_RIGHT_ALT, scType_basic},
        {"rightGui", HID_KEYBOARD_SC_RIGHT_GUI, scType_basic},
        {"mediaVolumeMute", MEDIA_VOLUME_MUTE, scType_media},
        {"mediaVolumeUp", MEDIA_VOLUME_UP, scType_media},
        {"mediaVolumeDown", MEDIA_VOLUME_DOWN, scType_media},
        {"mediaRecord", MEDIA_RECORD, scType_media},
        {"mediaFastForward", MEDIA_FAST_FORWARD, scType_media},
        {"mediaRewind", MEDIA_REWIND, scType_media},
        {"mediaNext", MEDIA_NEXT, scType_media},
        {"mediaPrevious", MEDIA_PREVIOUS, scType_media},
        {"mediaStop", MEDIA_STOP, scType_media},
        {"mediaPlayPause", MEDIA_PLAY_PAUSE, scType_media},
        {"mediaPause", MEDIA_PAUSE, scType_media},
        {"systemPowerDown", SYSTEM_POWER_DOWN, scType_system},
        {"systemSleep", SYSTEM_SLEEP, scType_system},
        {"systemWakeUp", SYSTEM_WAKE_UP, scType_system},
        {"mouseBtnLeft", MouseButton_Left, scType_mouseBtn},
        {"mouseBtnRight", MouseButton_Right, scType_mouseBtn},
        {"mouseBtnMiddle", MouseButton_Middle, scType_mouseBtn},
        {"mouseBtn1", MouseButton_Left, scType_mouseBtn},
        {"mouseBtn2", MouseButton_Right, scType_mouseBtn},
        {"mouseBtn3", MouseButton_Middle, scType_mouseBtn},
        {"mouseBtn4", MouseButton_4, scType_mouseBtn},
        {"mouseBtn5", MouseButton_5, scType_mouseBtn},
        {"mouseBtn6", MouseButton_6, scType_mouseBtn},
        {"mouseBtn7", MouseButton_7, scType_mouseBtn},
        {"mouseBtn8", MouseButton_8, scType_mouseBtn},
};

size_t lookup_size = sizeof(lookup_table)/sizeof(lookup_table[0]);

static void sortLookup()
{
    for (uint8_t i = 0; i < lookup_size; i++) {
        for (uint8_t j = 0; j < lookup_size - 1; j++) {
            if (!StrLessOrEqual(lookup_table[j].id, NULL, lookup_table[j+1].id, NULL)) {
                lookup_record_t tmp = lookup_table[j];
                lookup_table[j] = lookup_table[j+1];
                lookup_table[j+1] = tmp;
            }
        }
    }
}

void ShortcutParser_initialize()
{
    sortLookup();
}


static macro_action_t parseSingleChar(char c)
{
    macro_action_t action;
    action.type = MacroActionType_Key;
    action.key.type = KeystrokeType_Basic;
    action.key.scancode = MacroShortcutParser_CharacterToScancode(c);
    action.key.modifierMask = MacroShortcutParser_CharacterToShift(c) ? HID_KEYBOARD_MODIFIER_LEFTSHIFT : 0;
    return action;
}

static void parseMods(const char* str, const char* strEnd, macro_action_t* action)
{
    const char* orig = str;
    uint8_t modMask = 0;
    bool left = true;
    bool sticky = false;
    macro_sub_action_t actionType = action->key.action;
    while(str < strEnd) {
        switch(*str) {
        case 'L':
            left = true;
            break;
        case 'R':
            left = false;
            break;
        case 'S':
            modMask |= left ? HID_KEYBOARD_MODIFIER_LEFTSHIFT : HID_KEYBOARD_MODIFIER_RIGHTSHIFT;
            left = true;
            break;
        case 'C':
            modMask |= left ? HID_KEYBOARD_MODIFIER_LEFTCTRL : HID_KEYBOARD_MODIFIER_RIGHTCTRL;
            left = true;
            break;
        case 'A':
            modMask |= left ? HID_KEYBOARD_MODIFIER_LEFTALT : HID_KEYBOARD_MODIFIER_RIGHTALT;
            left = true;
            break;
        case 'W':
        case 'G':
            modMask |= left ? HID_KEYBOARD_MODIFIER_LEFTGUI : HID_KEYBOARD_MODIFIER_RIGHTGUI;
            left = true;
            break;
        case 's':
            sticky = true;
            break;
        case 'p':
            actionType = MacroSubAction_Press;
            break;
        case 'r':
            actionType = MacroSubAction_Release;
            break;
        case 't':
            actionType = MacroSubAction_Tap;
            break;
        case 'h':
            actionType = MacroSubAction_Hold;
            break;
        default:
            Macros_ReportError("Unrecognized mod abbreviation:", orig, strEnd);
            break;
        }
        str++;
    }

    if (action->type != MacroActionType_Key && modMask != 0) {
        Macros_ReportError("This action is not allowed to have modifiers!", str, strEnd);
    } else {
        action->key.modifierMask = modMask;
        action->key.sticky = sticky;
    }
    action->key.action = actionType;
}

static lookup_record_t* lookup(uint8_t begin, uint8_t end, const char* str, const char* strEnd)
{
    uint8_t pivot = begin + (end-begin)/2;
    if (begin == end) {
        if (StrLessOrEqual(str, strEnd, lookup_table[pivot].id, NULL) && StrLessOrEqual(lookup_table[pivot].id, NULL, str, strEnd)) {
            return &lookup_table[pivot];
        } else {
            Macros_ReportError("Unrecognized key abbreviation:", str, strEnd);
            return &lookup_table[0];
        }
    }
    else if (StrLessOrEqual(str, strEnd, lookup_table[pivot].id, NULL)) {
        return lookup(begin, pivot, str, strEnd);
    } else {
        return lookup(pivot + 1, end, str, strEnd);
    }
}

static macro_action_t parseAbbrev(const char* str, const char* strEnd)
{
    if (str + 1 == strEnd) {
        return parseSingleChar(*str);
    }

    lookup_record_t* record = lookup(0, lookup_size-1, str, strEnd);
    macro_action_t action;
    memset(&action, 0, sizeof action);

    switch(record->type) {
    case scType_basic:
        action.type = MacroActionType_Key;
        action.key.type = KeystrokeType_Basic;
        action.key.scancode = record->scancode;
        break;
    case scType_system:
        action.type = MacroActionType_Key;
        action.key.type = KeystrokeType_System;
        action.key.scancode = record->scancode;
        break;
    case scType_media:
        action.type = MacroActionType_Key;
        action.key.type = KeystrokeType_Media;
        action.key.scancode = record->scancode;
        break;
    case scType_mouseBtn:
        action.type = MacroActionType_MouseButton;
        action.mouseButton.mouseButtonsMask = record->scancode;
        break;
    }
    return action;
}

macro_action_t MacroShortcutParser_Parse(const char* str, const char* strEnd, macro_sub_action_t type)
{
    macro_action_t action;

    if (FindChar('-', str, strEnd) == strEnd) {
        //"-" notation not used
        action = parseAbbrev(str, strEnd);
        action.key.action = type;
    }
    else {
        const char* delim = FindChar('-', str, strEnd);
        action = parseAbbrev(delim+1, strEnd);
        action.key.action = type;

        parseMods(str, delim, &action);
    }
    return action;
}
