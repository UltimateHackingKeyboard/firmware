#include "macros.h"
#include "config_parser/parse_macro.h"
#include "config_parser/config_globals.h"
#include "timer.h"

macro_reference_t AllMacros[MAX_MACRO_NUM];
uint8_t AllMacrosCount;
bool MacroPlaying = false;
usb_mouse_report_t MacroMouseReport;
usb_basic_keyboard_report_t MacroBasicKeyboardReport;
usb_media_keyboard_report_t MacroMediaKeyboardReport;
usb_system_keyboard_report_t MacroSystemKeyboardReport;

static uint8_t currentMacroIndex;
static uint16_t currentMacroActionIndex;
static macro_action_t currentMacroAction;

uint8_t characterToScancode(char character)
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

bool characterToShift(char character)
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

void addBasicScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    uint16_t maxKeys = usbBasicKeyboardProtocol == 0 ? USB_BOOT_KEYBOARD_MAX_KEYS : USB_BASIC_KEYBOARD_MAX_KEYS;
    for (uint8_t i = 0; i < maxKeys; i++) {
        if (MacroBasicKeyboardReport.scancodes[i] == scancode) {
            return;
        }
    }
    for (uint8_t i = 0; i < maxKeys; i++) {
        if (!MacroBasicKeyboardReport.scancodes[i]) {
            MacroBasicKeyboardReport.scancodes[i] = scancode;
            break;
        }
    }
}

void deleteBasicScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    uint16_t maxKeys = usbBasicKeyboardProtocol == 0 ? USB_BOOT_KEYBOARD_MAX_KEYS : USB_BASIC_KEYBOARD_MAX_KEYS;
    for (uint8_t i = 0; i < maxKeys; i++) {
        if (MacroBasicKeyboardReport.scancodes[i] == scancode) {
            MacroBasicKeyboardReport.scancodes[i] = 0;
            return;
        }
    }
}

void addModifiers(uint8_t modifiers)
{
    MacroBasicKeyboardReport.modifiers |= modifiers;
}

void deleteModifiers(uint8_t modifiers)
{
    MacroBasicKeyboardReport.modifiers &= ~modifiers;
}

void addMediaScancode(uint16_t scancode)
{
    if (!scancode) {
        return;
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (MacroMediaKeyboardReport.scancodes[i] == scancode) {
            return;
        }
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (!MacroMediaKeyboardReport.scancodes[i]) {
            MacroMediaKeyboardReport.scancodes[i] = scancode;
            break;
        }
    }
}

void deleteMediaScancode(uint16_t scancode)
{
    if (!scancode) {
        return;
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (MacroMediaKeyboardReport.scancodes[i] == scancode) {
            MacroMediaKeyboardReport.scancodes[i] = 0;
            return;
        }
    }
}

void addSystemScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    for (uint8_t i = 0; i < USB_SYSTEM_KEYBOARD_MAX_KEYS; i++) {
        if (MacroSystemKeyboardReport.scancodes[i] == scancode) {
            return;
        }
    }
    for (uint8_t i = 0; i < USB_SYSTEM_KEYBOARD_MAX_KEYS; i++) {
        if (!MacroSystemKeyboardReport.scancodes[i]) {
            MacroSystemKeyboardReport.scancodes[i] = scancode;
            break;
        }
    }
}

void deleteSystemScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    for (uint8_t i = 0; i < USB_SYSTEM_KEYBOARD_MAX_KEYS; i++) {
        if (MacroSystemKeyboardReport.scancodes[i] == scancode) {
            MacroSystemKeyboardReport.scancodes[i] = 0;
            return;
        }
    }
}

void addScancode(uint16_t scancode, keystroke_type_t type)
{
    switch (type) {
        case KeystrokeType_Basic:
            addBasicScancode(scancode);
            break;
        case KeystrokeType_Media:
            addMediaScancode(scancode);
            break;
        case KeystrokeType_System:
            addSystemScancode(scancode);
            break;
    }
}

void deleteScancode(uint16_t scancode, keystroke_type_t type)
{
    switch (type) {
        case KeystrokeType_Basic:
            deleteBasicScancode(scancode);
            break;
        case KeystrokeType_Media:
            deleteMediaScancode(scancode);
            break;
        case KeystrokeType_System:
            deleteSystemScancode(scancode);
            break;
    }
}

bool processKeyAction(void)
{
    static uint8_t pressPhase = 0;

    pressPhase++;

    switch (currentMacroAction.key.action) {
        case MacroSubAction_Tap:
            switch(pressPhase) {
                case 1:
                    addModifiers(currentMacroAction.key.modifierMask);
                    return true;
                case 2:
                    addScancode(currentMacroAction.key.scancode, currentMacroAction.key.type);
                    return true;
                case 3:
                    deleteScancode(currentMacroAction.key.scancode, currentMacroAction.key.type);
                    return true;
                case 4:
                    deleteModifiers(currentMacroAction.key.modifierMask);
                    pressPhase = 0;
                    return false;
            }
            break;
        case MacroSubAction_Release:
            switch (pressPhase) {
                case 1:
                    deleteScancode(currentMacroAction.key.scancode, currentMacroAction.key.type);
                    return true;
                case 2:
                    deleteModifiers(currentMacroAction.key.modifierMask);
                    pressPhase = 0;
                    return false;
            }
            break;
        case MacroSubAction_Press:
            switch (pressPhase) {
                case 1:
                    addModifiers(currentMacroAction.key.modifierMask);
                    return true;
                case 2:
                    addScancode(currentMacroAction.key.scancode, currentMacroAction.key.type);
                    pressPhase = 0;
                    return false;
            }
            break;
    }
    return false;
}

bool processDelayAction(void)
{
    static bool inDelay;
    static uint32_t delayStart;

    if (inDelay) {
        if (Timer_GetElapsedTime(&delayStart) >= currentMacroAction.delay.delay) {
            inDelay = false;
        }
    } else {
        delayStart = CurrentTime;
        inDelay = true;
    }
    return inDelay;
}

bool processMouseButtonAction(void)
{
    static bool pressStarted;

    switch (currentMacroAction.mouseButton.action) {
        case MacroSubAction_Tap:
            if (!pressStarted) {
                pressStarted = true;
                MacroMouseReport.buttons |= currentMacroAction.mouseButton.mouseButtonsMask;
                return true;
            }
            pressStarted = false;
            MacroMouseReport.buttons &= ~currentMacroAction.mouseButton.mouseButtonsMask;
            break;
        case MacroSubAction_Release:
            MacroMouseReport.buttons &= ~currentMacroAction.mouseButton.mouseButtonsMask;
            break;
        case MacroSubAction_Press:
            MacroMouseReport.buttons |= currentMacroAction.mouseButton.mouseButtonsMask;
            break;
    }
    return false;
}

bool processMoveMouseAction(void)
{
    static bool inMotion;

    if (inMotion) {
        MacroMouseReport.x = 0;
        MacroMouseReport.y = 0;
        inMotion = false;
    } else {
        MacroMouseReport.x = currentMacroAction.moveMouse.x;
        MacroMouseReport.y = currentMacroAction.moveMouse.y;
        inMotion = true;
    }
    return inMotion;
}

bool processScrollMouseAction(void)
{
    static bool inMotion;

    if (inMotion) {
        MacroMouseReport.wheelX = 0;
        MacroMouseReport.wheelY = 0;
        inMotion = false;
    } else {
        MacroMouseReport.wheelX = currentMacroAction.scrollMouse.x;
        MacroMouseReport.wheelY = currentMacroAction.scrollMouse.y;
        inMotion = true;
    }
    return inMotion;
}

static void clearScancodes()
{
    uint8_t oldMods = MacroBasicKeyboardReport.modifiers;
    memset(&MacroBasicKeyboardReport, 0, sizeof MacroBasicKeyboardReport);
    MacroBasicKeyboardReport.modifiers = oldMods;
}

bool processTextAction(void)
{
    static uint16_t textIndex;
    static uint8_t reportIndex = USB_BASIC_KEYBOARD_MAX_KEYS;
    char character = 0;
    uint8_t scancode = 0;
    uint8_t mods = 0;

    // Precompute modifiers and scancode.
    if(textIndex != currentMacroAction.text.textLen) {
        character = currentMacroAction.text.text[textIndex];
        scancode = characterToScancode(character);
        mods = characterToShift(character) ? HID_KEYBOARD_MODIFIER_LEFTSHIFT : 0;
    }

    // If required modifiers differ, first clear scancodes and send empty report
    // containing only old modifiers. Then set new modifiers and send that new report.
    // Just then continue.
    if (mods != MacroBasicKeyboardReport.modifiers) {
        if (reportIndex != 0) {
            reportIndex = 0;
            clearScancodes();
            return true;
        } else {
            MacroBasicKeyboardReport.modifiers = mods;
            return true;
        }
    }

    // If all characters have been sent, finish.
    if (textIndex == currentMacroAction.text.textLen) {
        textIndex = 0;
        reportIndex = USB_BASIC_KEYBOARD_MAX_KEYS;
        memset(&MacroBasicKeyboardReport, 0, sizeof MacroBasicKeyboardReport);
        return false;
    }

    // Whenever the report is full, we clear the report and send it empty before continuing.
    if (reportIndex == USB_BASIC_KEYBOARD_MAX_KEYS) {
        reportIndex = 0;
        memset(&MacroBasicKeyboardReport, 0, sizeof MacroBasicKeyboardReport);
        return true;
    }

    // If current character is already contained in the report, we need to
    // release it first. We do so by artificially marking the report
    // full. Next call will do rest of the work for us.
    for (uint8_t i = 0; i < reportIndex; i++) {
        if (MacroBasicKeyboardReport.scancodes[i] == scancode) {
            reportIndex = USB_BASIC_KEYBOARD_MAX_KEYS;
            return true;
        }
    }

    // Send the scancode.
    MacroBasicKeyboardReport.scancodes[reportIndex++] = scancode;
    ++textIndex;
    return true;
}

bool processCurrentMacroAction(void)
{
    switch (currentMacroAction.type) {
        case MacroActionType_Delay:
            return processDelayAction();
        case MacroActionType_Key:
            return processKeyAction();
        case MacroActionType_MouseButton:
            return processMouseButtonAction();
        case MacroActionType_MoveMouse:
            return processMoveMouseAction();
        case MacroActionType_ScrollMouse:
            return processScrollMouseAction();
        case MacroActionType_Text:
            return processTextAction();
    }
    return false;
}

void Macros_StartMacro(uint8_t index)
{
    if (AllMacros[index].macroActionsCount == 0) {
        return;
    }
    MacroPlaying = true;
    currentMacroIndex = index;
    currentMacroActionIndex = 0;
    ValidatedUserConfigBuffer.offset = AllMacros[index].firstMacroActionOffset;
    ParseMacroAction(&ValidatedUserConfigBuffer, &currentMacroAction);
    memset(&MacroMouseReport, 0, sizeof MacroMouseReport);
    memset(&MacroBasicKeyboardReport, 0, sizeof MacroBasicKeyboardReport);
    memset(&MacroMediaKeyboardReport, 0, sizeof MacroMediaKeyboardReport);
    memset(&MacroSystemKeyboardReport, 0, sizeof MacroSystemKeyboardReport);
}

void Macros_ContinueMacro(void)
{
    if (processCurrentMacroAction()) {
        return;
    }
    if (++currentMacroActionIndex == AllMacros[currentMacroIndex].macroActionsCount) {
        MacroPlaying = false;
        return;
    }
    ParseMacroAction(&ValidatedUserConfigBuffer, &currentMacroAction);
}
