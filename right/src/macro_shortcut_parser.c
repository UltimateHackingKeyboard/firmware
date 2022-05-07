#include "key_action.h"
#include "macros.h"
#include "arduino_hid/ConsumerAPI.h"
#include "arduino_hid/SystemAPI.h"
#include "config_parser/parse_keymap.h"
#include "config_parser/config_globals.h"
#include "macro_shortcut_parser.h"
#include "str_utils.h"

static const lookup_record_t* lookup(uint8_t begin, uint8_t end, const char* str, const char* strEnd);

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

const lookup_record_t lookup_table[] = {
        // ALWAYS keep the array sorted by `LC_ALL=C sort`
        {"", 0, scType_basic},
        {"again", HID_KEYBOARD_SC_AGAIN, scType_basic},
        {"alternateErase", HID_KEYBOARD_SC_ALTERNATE_ERASE, scType_basic},
        {"apostropheAndQuote", HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE, scType_basic},
        {"application", HID_KEYBOARD_SC_APPLICATION, scType_basic},
        {"backslashAndPipe", HID_KEYBOARD_SC_BACKSLASH_AND_PIPE, scType_basic},
        {"backslashAndPipeIso", HID_KEYBOARD_SC_NON_US_BACKSLASH_AND_PIPE, scType_basic},
        {"backspace", HID_KEYBOARD_SC_BACKSPACE, scType_basic},
        {"cancel", HID_KEYBOARD_SC_CANCEL, scType_basic},
        {"capsLock", HID_KEYBOARD_SC_CAPS_LOCK, scType_basic},
        {"clear", HID_KEYBOARD_SC_CLEAR, scType_basic},
        {"clearAndAgain", HID_KEYBOARD_SC_CLEAR_AND_AGAIN, scType_basic},
        {"closingBracketAndClosingBrace", HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE, scType_basic},
        {"commaAndLessThanSign", HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN, scType_basic},
        {"copy", HID_KEYBOARD_SC_COPY, scType_basic},
        {"crselAndProps", HID_KEYBOARD_SC_CRSEL_AND_PROPS, scType_basic},
        {"currencySubUnit", HID_KEYBOARD_SC_CURRENCY_SUB_UNIT, scType_basic},
        {"currencyUnit", HID_KEYBOARD_SC_CURRENCY_UNIT, scType_basic},
        {"cut", HID_KEYBOARD_SC_CUT, scType_basic},
        {"decimalSeparator", HID_KEYBOARD_SC_DECIMAL_SEPARATOR, scType_basic},
        {"delete", HID_KEYBOARD_SC_DELETE, scType_basic},
        {"dotAndGreaterThanSign", HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN, scType_basic},
        {"down", HID_KEYBOARD_SC_DOWN_ARROW, scType_basic},
        {"downArrow", HID_KEYBOARD_SC_DOWN_ARROW, scType_basic},
        {"end", HID_KEYBOARD_SC_END, scType_basic},
        {"enter", HID_KEYBOARD_SC_ENTER, scType_basic},
        {"equalAndPlus", HID_KEYBOARD_SC_EQUAL_AND_PLUS, scType_basic},
        {"escape", HID_KEYBOARD_SC_ESCAPE, scType_basic},
        {"execute", HID_KEYBOARD_SC_EXECUTE, scType_basic},
        {"exsel", HID_KEYBOARD_SC_EXSEL, scType_basic},
        {"f1", HID_KEYBOARD_SC_F1, scType_basic},
        {"f10", HID_KEYBOARD_SC_F10, scType_basic},
        {"f11", HID_KEYBOARD_SC_F11, scType_basic},
        {"f12", HID_KEYBOARD_SC_F12, scType_basic},
        {"f13", HID_KEYBOARD_SC_F13, scType_basic},
        {"f14", HID_KEYBOARD_SC_F14, scType_basic},
        {"f15", HID_KEYBOARD_SC_F15, scType_basic},
        {"f16", HID_KEYBOARD_SC_F16, scType_basic},
        {"f17", HID_KEYBOARD_SC_F17, scType_basic},
        {"f18", HID_KEYBOARD_SC_F18, scType_basic},
        {"f19", HID_KEYBOARD_SC_F19, scType_basic},
        {"f2", HID_KEYBOARD_SC_F2, scType_basic},
        {"f20", HID_KEYBOARD_SC_F20, scType_basic},
        {"f21", HID_KEYBOARD_SC_F21, scType_basic},
        {"f22", HID_KEYBOARD_SC_F22, scType_basic},
        {"f23", HID_KEYBOARD_SC_F23, scType_basic},
        {"f24", HID_KEYBOARD_SC_F24, scType_basic},
        {"f3", HID_KEYBOARD_SC_F3, scType_basic},
        {"f4", HID_KEYBOARD_SC_F4, scType_basic},
        {"f5", HID_KEYBOARD_SC_F5, scType_basic},
        {"f6", HID_KEYBOARD_SC_F6, scType_basic},
        {"f7", HID_KEYBOARD_SC_F7, scType_basic},
        {"f8", HID_KEYBOARD_SC_F8, scType_basic},
        {"f9", HID_KEYBOARD_SC_F9, scType_basic},
        {"find", HID_KEYBOARD_SC_FIND, scType_basic},
        {"graveAccentAndTilde", HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE, scType_basic},
        {"help", HID_KEYBOARD_SC_HELP, scType_basic},
        {"home", HID_KEYBOARD_SC_HOME, scType_basic},
        {"insert", HID_KEYBOARD_SC_INSERT, scType_basic},
        {"international1", HID_KEYBOARD_SC_INTERNATIONAL1, scType_basic},
        {"international2", HID_KEYBOARD_SC_INTERNATIONAL2, scType_basic},
        {"international3", HID_KEYBOARD_SC_INTERNATIONAL3, scType_basic},
        {"international4", HID_KEYBOARD_SC_INTERNATIONAL4, scType_basic},
        {"international5", HID_KEYBOARD_SC_INTERNATIONAL5, scType_basic},
        {"international6", HID_KEYBOARD_SC_INTERNATIONAL6, scType_basic},
        {"international7", HID_KEYBOARD_SC_INTERNATIONAL7, scType_basic},
        {"international8", HID_KEYBOARD_SC_INTERNATIONAL8, scType_basic},
        {"international9", HID_KEYBOARD_SC_INTERNATIONAL9, scType_basic},
        {"keypad00", HID_KEYBOARD_SC_KEYPAD_00, scType_basic},
        {"keypad000", HID_KEYBOARD_SC_KEYPAD_000, scType_basic},
        {"keypad0AndInsert", HID_KEYBOARD_SC_KEYPAD_0_AND_INSERT, scType_basic},
        {"keypad1AndEnd", HID_KEYBOARD_SC_KEYPAD_1_AND_END, scType_basic},
        {"keypad2AndDownArrow", HID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW, scType_basic},
        {"keypad3AndPageDown", HID_KEYBOARD_SC_KEYPAD_3_AND_PAGE_DOWN, scType_basic},
        {"keypad4AndLeftArrow", HID_KEYBOARD_SC_KEYPAD_4_AND_LEFT_ARROW, scType_basic},
        {"keypad5", HID_KEYBOARD_SC_KEYPAD_5, scType_basic},
        {"keypad6AndRightArrow", HID_KEYBOARD_SC_KEYPAD_6_AND_RIGHT_ARROW, scType_basic},
        {"keypad7AndHome", HID_KEYBOARD_SC_KEYPAD_7_AND_HOME, scType_basic},
        {"keypad8AndUpArrow", HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW, scType_basic},
        {"keypad9AndPageUp", HID_KEYBOARD_SC_KEYPAD_9_AND_PAGE_UP, scType_basic},
        {"keypadA", HID_KEYBOARD_SC_KEYPAD_A, scType_basic},
        {"keypadAmp", HID_KEYBOARD_SC_KEYPAD_AMP, scType_basic},
        {"keypadAmpAmp", HID_KEYBOARD_SC_KEYPAD_AMP_AMP, scType_basic},
        {"keypadAsterisk", HID_KEYBOARD_SC_KEYPAD_ASTERISK, scType_basic},
        {"keypadAt", HID_KEYBOARD_SC_KEYPAD_AT, scType_basic},
        {"keypadB", HID_KEYBOARD_SC_KEYPAD_B, scType_basic},
        {"keypadBackspace", HID_KEYBOARD_SC_KEYPAD_BACKSPACE, scType_basic},
        {"keypadBinary", HID_KEYBOARD_SC_KEYPAD_BINARY, scType_basic},
        {"keypadC", HID_KEYBOARD_SC_KEYPAD_C, scType_basic},
        {"keypadCaret", HID_KEYBOARD_SC_KEYPAD_CARET, scType_basic},
        {"keypadClear", HID_KEYBOARD_SC_KEYPAD_CLEAR, scType_basic},
        {"keypadClearEntry", HID_KEYBOARD_SC_KEYPAD_CLEAR_ENTRY, scType_basic},
        {"keypadClosingBrace", HID_KEYBOARD_SC_KEYPAD_CLOSING_BRACE, scType_basic},
        {"keypadClosingParenthesis", HID_KEYBOARD_SC_KEYPAD_CLOSING_PARENTHESIS, scType_basic},
        {"keypadColon", HID_KEYBOARD_SC_KEYPAD_COLON, scType_basic},
        {"keypadComma", HID_KEYBOARD_SC_KEYPAD_COMMA, scType_basic},
        {"keypadD", HID_KEYBOARD_SC_KEYPAD_D, scType_basic},
        {"keypadDecimal", HID_KEYBOARD_SC_KEYPAD_DECIMAL, scType_basic},
        {"keypadDotAndDelete", HID_KEYBOARD_SC_KEYPAD_DOT_AND_DELETE, scType_basic},
        {"keypadE", HID_KEYBOARD_SC_KEYPAD_E, scType_basic},
        {"keypadEnter", HID_KEYBOARD_SC_KEYPAD_ENTER, scType_basic},
        {"keypadEqualSign", HID_KEYBOARD_SC_KEYPAD_EQUAL_SIGN, scType_basic},
        {"keypadEqualSignAs400", HID_KEYBOARD_SC_KEYPAD_EQUAL_SIGN_AS400, scType_basic},
        {"keypadExclamationSign", HID_KEYBOARD_SC_KEYPAD_EXCLAMATION_SIGN, scType_basic},
        {"keypadF", HID_KEYBOARD_SC_KEYPAD_F, scType_basic},
        {"keypadGreaterThanSign", HID_KEYBOARD_SC_KEYPAD_GREATER_THAN_SIGN, scType_basic},
        {"keypadHashmark", HID_KEYBOARD_SC_KEYPAD_HASHMARK, scType_basic},
        {"keypadHexadecimal", HID_KEYBOARD_SC_KEYPAD_HEXADECIMAL, scType_basic},
        {"keypadLessThanSign", HID_KEYBOARD_SC_KEYPAD_LESS_THAN_SIGN, scType_basic},
        {"keypadMemoryAdd", HID_KEYBOARD_SC_KEYPAD_MEMORY_ADD, scType_basic},
        {"keypadMemoryClear", HID_KEYBOARD_SC_KEYPAD_MEMORY_CLEAR, scType_basic},
        {"keypadMemoryDivide", HID_KEYBOARD_SC_KEYPAD_MEMORY_DIVIDE, scType_basic},
        {"keypadMemoryMultiply", HID_KEYBOARD_SC_KEYPAD_MEMORY_MULTIPLY, scType_basic},
        {"keypadMemoryRecall", HID_KEYBOARD_SC_KEYPAD_MEMORY_RECALL, scType_basic},
        {"keypadMemoryStore", HID_KEYBOARD_SC_KEYPAD_MEMORY_STORE, scType_basic},
        {"keypadMemorySubtract", HID_KEYBOARD_SC_KEYPAD_MEMORY_SUBTRACT, scType_basic},
        {"keypadMinus", HID_KEYBOARD_SC_KEYPAD_MINUS, scType_basic},
        {"keypadOctal", HID_KEYBOARD_SC_KEYPAD_OCTAL, scType_basic},
        {"keypadOpeningBrace", HID_KEYBOARD_SC_KEYPAD_OPENING_BRACE, scType_basic},
        {"keypadOpeningParenthesis", HID_KEYBOARD_SC_KEYPAD_OPENING_PARENTHESIS, scType_basic},
        {"keypadPercentage", HID_KEYBOARD_SC_KEYPAD_PERCENTAGE, scType_basic},
        {"keypadPipe", HID_KEYBOARD_SC_KEYPAD_PIPE, scType_basic},
        {"keypadPipePipe", HID_KEYBOARD_SC_KEYPAD_PIPE_PIPE, scType_basic},
        {"keypadPlus", HID_KEYBOARD_SC_KEYPAD_PLUS, scType_basic},
        {"keypadPlusAndMinus", HID_KEYBOARD_SC_KEYPAD_PLUS_AND_MINUS, scType_basic},
        {"keypadSlash", HID_KEYBOARD_SC_KEYPAD_SLASH, scType_basic},
        {"keypadSpace", HID_KEYBOARD_SC_KEYPAD_SPACE, scType_basic},
        {"keypadTab", HID_KEYBOARD_SC_KEYPAD_TAB, scType_basic},
        {"keypadXor", HID_KEYBOARD_SC_KEYPAD_XOR, scType_basic},
        {"lang1", HID_KEYBOARD_SC_LANG1, scType_basic},
        {"lang2", HID_KEYBOARD_SC_LANG2, scType_basic},
        {"lang3", HID_KEYBOARD_SC_LANG3, scType_basic},
        {"lang4", HID_KEYBOARD_SC_LANG4, scType_basic},
        {"lang5", HID_KEYBOARD_SC_LANG5, scType_basic},
        {"lang6", HID_KEYBOARD_SC_LANG6, scType_basic},
        {"lang7", HID_KEYBOARD_SC_LANG7, scType_basic},
        {"lang8", HID_KEYBOARD_SC_LANG8, scType_basic},
        {"lang9", HID_KEYBOARD_SC_LANG9, scType_basic},
        {"left", HID_KEYBOARD_SC_LEFT_ARROW, scType_basic},
        {"leftAlt", HID_KEYBOARD_SC_LEFT_ALT, scType_basic},
        {"leftArrow", HID_KEYBOARD_SC_LEFT_ARROW, scType_basic},
        {"leftControl", HID_KEYBOARD_SC_LEFT_CONTROL, scType_basic},
        {"leftGui", HID_KEYBOARD_SC_LEFT_GUI, scType_basic},
        {"leftShift", HID_KEYBOARD_SC_LEFT_SHIFT, scType_basic},
        {"lockingCapsLock", HID_KEYBOARD_SC_LOCKING_CAPS_LOCK, scType_basic},
        {"lockingNumLock", HID_KEYBOARD_SC_LOCKING_NUM_LOCK, scType_basic},
        {"lockingScrollLock", HID_KEYBOARD_SC_LOCKING_SCROLL_LOCK, scType_basic},
        {"mediaFastForward", MEDIA_FAST_FORWARD, scType_media},
        {"mediaNext", MEDIA_NEXT, scType_media},
        {"mediaPause", MEDIA_PAUSE, scType_media},
        {"mediaPlayPause", MEDIA_PLAY_PAUSE, scType_media},
        {"mediaPrevious", MEDIA_PREVIOUS, scType_media},
        {"mediaRecord", MEDIA_RECORD, scType_media},
        {"mediaRewind", MEDIA_REWIND, scType_media},
        {"mediaStop", MEDIA_STOP, scType_media},
        {"mediaVolumeDown", MEDIA_VOLUME_DOWN, scType_media},
        {"mediaVolumeMute", MEDIA_VOLUME_MUTE, scType_media},
        {"mediaVolumeUp", MEDIA_VOLUME_UP, scType_media},
        {"menu", HID_KEYBOARD_SC_MENU, scType_basic},
        {"minusAndUnderscore", HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE, scType_basic},
        {"mouseBtn1", MouseButton_Left, scType_mouseBtn},
        {"mouseBtn2", MouseButton_Right, scType_mouseBtn},
        {"mouseBtn3", MouseButton_Middle, scType_mouseBtn},
        {"mouseBtn4", MouseButton_4, scType_mouseBtn},
        {"mouseBtn5", MouseButton_5, scType_mouseBtn},
        {"mouseBtn6", MouseButton_6, scType_mouseBtn},
        {"mouseBtn7", MouseButton_7, scType_mouseBtn},
        {"mouseBtn8", MouseButton_8, scType_mouseBtn},
        {"mouseBtnLeft", MouseButton_Left, scType_mouseBtn},
        {"mouseBtnMiddle", MouseButton_Middle, scType_mouseBtn},
        {"mouseBtnRight", MouseButton_Right, scType_mouseBtn},
        {"mute", HID_KEYBOARD_SC_MUTE, scType_basic},
        {"nonUsBackslashAndPipe", HID_KEYBOARD_SC_NON_US_BACKSLASH_AND_PIPE, scType_basic},
        {"nonUsHashmarkAndTilde", HID_KEYBOARD_SC_NON_US_HASHMARK_AND_TILDE, scType_basic},
        {"np0", HID_KEYBOARD_SC_KEYPAD_0_AND_INSERT, scType_basic},
        {"np1", HID_KEYBOARD_SC_KEYPAD_1_AND_END, scType_basic},
        {"np2", HID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW, scType_basic},
        {"np3", HID_KEYBOARD_SC_KEYPAD_3_AND_PAGE_DOWN, scType_basic},
        {"np4", HID_KEYBOARD_SC_KEYPAD_4_AND_LEFT_ARROW, scType_basic},
        {"np5", HID_KEYBOARD_SC_KEYPAD_5, scType_basic},
        {"np6", HID_KEYBOARD_SC_KEYPAD_6_AND_RIGHT_ARROW, scType_basic},
        {"np7", HID_KEYBOARD_SC_KEYPAD_7_AND_HOME, scType_basic},
        {"np8", HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW, scType_basic},
        {"np9", HID_KEYBOARD_SC_KEYPAD_9_AND_PAGE_UP, scType_basic},
        {"numLock", HID_KEYBOARD_SC_NUM_LOCK, scType_basic},
        {"openingBracketAndOpeningBrace", HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE, scType_basic},
        {"oper", HID_KEYBOARD_SC_OPER, scType_basic},
        {"out", HID_KEYBOARD_SC_OUT, scType_basic},
        {"pageDown", HID_KEYBOARD_SC_PAGE_DOWN, scType_basic},
        {"pageUp", HID_KEYBOARD_SC_PAGE_UP, scType_basic},
        {"paste", HID_KEYBOARD_SC_PASTE, scType_basic},
        {"pause", HID_KEYBOARD_SC_PAUSE, scType_basic},
        {"power", HID_KEYBOARD_SC_POWER, scType_basic},
        {"printScreen", HID_KEYBOARD_SC_PRINT_SCREEN, scType_basic},
        {"prior", HID_KEYBOARD_SC_PRIOR, scType_basic},
        {"return", HID_KEYBOARD_SC_RETURN, scType_basic},
        {"right", HID_KEYBOARD_SC_RIGHT_ARROW, scType_basic},
        {"rightAlt", HID_KEYBOARD_SC_RIGHT_ALT, scType_basic},
        {"rightArrow", HID_KEYBOARD_SC_RIGHT_ARROW, scType_basic},
        {"rightControl", HID_KEYBOARD_SC_RIGHT_CONTROL, scType_basic},
        {"rightGui", HID_KEYBOARD_SC_RIGHT_GUI, scType_basic},
        {"rightShift", HID_KEYBOARD_SC_RIGHT_SHIFT, scType_basic},
        {"scrollLock", HID_KEYBOARD_SC_SCROLL_LOCK, scType_basic},
        {"select", HID_KEYBOARD_SC_SELECT, scType_basic},
        {"semicolonAndColon", HID_KEYBOARD_SC_SEMICOLON_AND_COLON, scType_basic},
        {"separator", HID_KEYBOARD_SC_SEPARATOR, scType_basic},
        {"slashAndQuestionMark", HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK, scType_basic},
        {"space", HID_KEYBOARD_SC_SPACE, scType_basic},
        {"stop", HID_KEYBOARD_SC_STOP, scType_basic},
        {"sysreq", HID_KEYBOARD_SC_SYSREQ, scType_basic},
        {"systemPowerDown", SYSTEM_POWER_DOWN, scType_system},
        {"systemSleep", SYSTEM_SLEEP, scType_system},
        {"systemWakeUp", SYSTEM_WAKE_UP, scType_system},
        {"tab", HID_KEYBOARD_SC_TAB, scType_basic},
        {"thousandsSeparator", HID_KEYBOARD_SC_THOUSANDS_SEPARATOR, scType_basic},
        {"undo", HID_KEYBOARD_SC_UNDO, scType_basic},
        {"up", HID_KEYBOARD_SC_UP_ARROW, scType_basic},
        {"upArrow", HID_KEYBOARD_SC_UP_ARROW, scType_basic},
        {"volumeDown", HID_KEYBOARD_SC_VOLUME_DOWN, scType_basic},
        {"volumeUp", HID_KEYBOARD_SC_VOLUME_UP, scType_basic},
};

size_t lookup_size = sizeof(lookup_table)/sizeof(lookup_table[0]);

void testLookup()
{
    for (uint8_t i = 0; i < lookup_size - 1; i++) {
        if (!StrLessOrEqual(lookup_table[i].id, NULL, lookup_table[i+1].id, NULL)) {
            Macros_ReportError("Shortcut table is not properly sorted!", lookup_table[i].id, NULL);
        }
    }
}

void ShortcutParser_initialize()
{
    testLookup();
}


static bool parseSingleChar(char c, macro_action_t* outMacroAction, key_action_t* outKeyAction)
{
    if (outMacroAction != NULL) {
        outMacroAction->type = MacroActionType_Key;
        outMacroAction->key.type = KeystrokeType_Basic;
        outMacroAction->key.scancode = MacroShortcutParser_CharacterToScancode(c);
        outMacroAction->key.outputModMask = MacroShortcutParser_CharacterToShift(c) ? HID_KEYBOARD_MODIFIER_LEFTSHIFT : 0;
    }

    if (outKeyAction != NULL) {
        outKeyAction->type = KeyActionType_Keystroke;
        outKeyAction->keystroke.keystrokeType = KeystrokeType_Basic;
        outKeyAction->keystroke.scancode = MacroShortcutParser_CharacterToScancode(c);
        outKeyAction->keystroke.modifiers = MacroShortcutParser_CharacterToShift(c) ? HID_KEYBOARD_MODIFIER_LEFTSHIFT : 0;
    }

    return true;
}

static bool parseMods(const char* str, const char* strEnd, macro_action_t* outMacroAction, key_action_t* outKeyAction)
{
    uint8_t inputModMask = 0;
    uint8_t outputModMask = 0;
    uint8_t stickyModMask = 0;
    uint8_t* modMask = &outputModMask;
    bool explicitModType = false;
    bool explicitSubAction = false;
    bool success = true;
    bool left = true;
    macro_sub_action_t subAction = 0;
    if (outMacroAction != NULL) {
        subAction = outMacroAction->key.action;
    }
    while(str < strEnd) {
        switch(*str) {
        case 'L':
            left = true;
            break;
        case 'R':
            left = false;
            break;
        case 'S':
            *modMask |= left ? HID_KEYBOARD_MODIFIER_LEFTSHIFT : HID_KEYBOARD_MODIFIER_RIGHTSHIFT;
            left = true;
            break;
        case 'C':
            *modMask |= left ? HID_KEYBOARD_MODIFIER_LEFTCTRL : HID_KEYBOARD_MODIFIER_RIGHTCTRL;
            left = true;
            break;
        case 'A':
            *modMask |= left ? HID_KEYBOARD_MODIFIER_LEFTALT : HID_KEYBOARD_MODIFIER_RIGHTALT;
            left = true;
            break;
        case 'W':
        case 'G':
            *modMask |= left ? HID_KEYBOARD_MODIFIER_LEFTGUI : HID_KEYBOARD_MODIFIER_RIGHTGUI;
            left = true;
            break;
        case 's':
            modMask = &stickyModMask;
            explicitModType = true;
            break;
        case 'i':
            modMask = &inputModMask;
            explicitModType = true;
            break;
        case 'o':
            modMask = &outputModMask;
            explicitModType = true;
            break;
        case 'p':
            subAction = MacroSubAction_Press;
            explicitSubAction = true;
            break;
        case 'r':
            subAction = MacroSubAction_Release;
            explicitSubAction = true;
            break;
        case 't':
            subAction = MacroSubAction_Tap;
            explicitSubAction = true;
            break;
        case 'h':
            subAction = MacroSubAction_Hold;
            explicitSubAction = true;
            break;
        default:
            success = false;
            break;
        }
        str++;
    }

    if (success && outMacroAction != NULL) {
        if (outMacroAction->type != MacroActionType_Key && inputModMask != 0) {
            Macros_ReportError("This action is not allowed to have modifiers!", str, strEnd);
        } else {
            outMacroAction->key.inputModMask = inputModMask;
            outMacroAction->key.outputModMask = outputModMask;
            outMacroAction->key.stickyModMask = stickyModMask;
        }
        outMacroAction->key.action = subAction;
    }

    if (success && outKeyAction != NULL) {
        uint8_t jointMask = inputModMask | outputModMask | stickyModMask;

        if (explicitModType || explicitSubAction) {
            Macros_ReportError("iosprth are not supported in this context!", str, strEnd);
        } else if (outKeyAction->type != KeyActionType_Keystroke && jointMask != 0) {
            Macros_ReportError("This action is not allowed to have modifiers!", str, strEnd);
        } else {
            outKeyAction->keystroke.modifiers = jointMask;
        }
    }

    return success;
}

static serialized_mouse_action_t mouseBtnToSerializedMouseAction(mouse_button_t btn)
{
    switch (btn) {
        case MouseButton_Left:
            return SerializedMouseAction_LeftClick;
        case MouseButton_Right:
            return SerializedMouseAction_RightClick;
        case MouseButton_Middle:
            return SerializedMouseAction_MiddleClick;
        case MouseButton_4:
            return SerializedMouseAction_Button_4;
        case MouseButton_5:
            return SerializedMouseAction_Button_5;
        case MouseButton_6:
            return SerializedMouseAction_Button_6;
        case MouseButton_7:
            return SerializedMouseAction_Button_7;
        case MouseButton_8:
            return SerializedMouseAction_Button_8;
        default:
            Macros_ReportErrorNum("Unknown button encountered:", btn);
            return 0;
    }
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

static bool parseAbbrev(const char* str, const char* strEnd, macro_action_t* outMacroAction, key_action_t* outKeyAction)
{
    if (str + 1 == strEnd) {
        return parseSingleChar(*str, outMacroAction, outKeyAction);
    }

    const lookup_record_t* record = lookup(0, lookup_size-1, str, strEnd);

    if (record == NULL) {
        return false;
    }

    if (outMacroAction != NULL) {
        memset(outMacroAction, 0, sizeof *outMacroAction);

        switch(record->type) {
            case scType_basic:
                outMacroAction->type = MacroActionType_Key;
                outMacroAction->key.type = KeystrokeType_Basic;
                outMacroAction->key.scancode = record->scancode;
                break;
            case scType_system:
                outMacroAction->type = MacroActionType_Key;
                outMacroAction->key.type = KeystrokeType_System;
                outMacroAction->key.scancode = record->scancode;
                break;
            case scType_media:
                outMacroAction->type = MacroActionType_Key;
                outMacroAction->key.type = KeystrokeType_Media;
                outMacroAction->key.scancode = record->scancode;
                break;
            case scType_mouseBtn:
                outMacroAction->type = MacroActionType_MouseButton;
                outMacroAction->mouseButton.mouseButtonsMask = record->scancode;
                break;
        }
    }

    if (outKeyAction != NULL) {
        memset(outKeyAction, 0, sizeof *outKeyAction);

        switch(record->type) {
            case scType_basic:
                outKeyAction->type = KeyActionType_Keystroke;
                outKeyAction->keystroke.keystrokeType = KeystrokeType_Basic;
                outKeyAction->keystroke.scancode = record->scancode;
                break;
            case scType_system:
                outKeyAction->type = KeyActionType_Keystroke;
                outKeyAction->keystroke.keystrokeType = KeystrokeType_System;
                outKeyAction->keystroke.scancode = record->scancode;
                break;
            case scType_media:
                outKeyAction->type = KeyActionType_Keystroke;
                outKeyAction->keystroke.keystrokeType = KeystrokeType_Media;
                outKeyAction->keystroke.scancode = record->scancode;
                break;
            case scType_mouseBtn:
                outKeyAction->type = KeyActionType_Mouse;
                outKeyAction->mouseAction = mouseBtnToSerializedMouseAction(record->scancode);
                break;
        }
    }
    return true;
}

bool MacroShortcutParser_Parse(const char* str, const char* strEnd, macro_sub_action_t type, macro_action_t* outMacroAction, key_action_t* outKeyAction)
{

    bool success = false;

    if (FindChar('-', str, strEnd) == strEnd) {
        //"-" notation not used
        success = success || parseAbbrev(str, strEnd, outMacroAction, outKeyAction);
        success = success || parseMods(str, strEnd, outMacroAction, outKeyAction);

        if (outMacroAction != NULL) {
            outMacroAction->key.action = type;
        }
    }
    else {
        const char* delim = FindChar('-', str, strEnd);
        success = success || parseAbbrev(delim+1, strEnd, outMacroAction, outKeyAction);

        if (outMacroAction != NULL) {
            outMacroAction->key.action = type;
        }

        success = success && parseMods(str, delim, outMacroAction, outKeyAction);
    }

    if (!success) {
        Macros_ReportError("Unrecognized key abbreviation:", str, strEnd);
    }
    return success;
}
