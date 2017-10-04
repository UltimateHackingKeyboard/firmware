#include "macros.h"
#include "config_parser/parse_macro.h"
#include "config_parser/config_globals.h"

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
            return 0;
        case 'a' ... 'z':
            return 0;
        case '1' ... '9':
            return 0;
        case ')':
        case '0':
            return 0;
        case '!':
            return 0;
        case '@':
            return 0;
        case '#':
            return 0;
        case '$':
            return 0;
        case '%':
            return 0;
        case '^':
            return 0;
        case '&':
            return 0;
        case '*':
            return 0;
        case '(':
            return 0;
        case '`':
        case '~':
            return 0;
        case '[':
        case '{':
            return 0;
        case ']':
        case '}':
            return 0;
        case ';':
        case ':':
            return 0;
        case '\'':
        case '\"':
            return 0;
        case '+':
        case '=':
            return 0;
        case '\\':
        case '|':
            return 0;
        case '.':
        case '>':
            return 0;
        case ',':
        case '<':
            return 0;
        case '/':
        case '\?':
            return 0;
        case '-':
        case '_':
            return 0;
    }
    return 0;
}

void addBasicScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    for (uint8_t i = 0; i < USB_BASIC_KEYBOARD_MAX_KEYS; i++) {
        if (MacroBasicKeyboardReport.scancodes[i] == scancode) {
            return;
        }
    }
    for (uint8_t i = 0; i < USB_BASIC_KEYBOARD_MAX_KEYS; i++) {
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
    for (uint8_t i = 0; i < USB_BASIC_KEYBOARD_MAX_KEYS; i++) {
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

bool processKeyMacroAction(void)
{
    static bool pressStarted;

    switch (currentMacroAction.key.action) {
        case MacroSubAction_Press:
            if (!pressStarted) {
                pressStarted = true;
                addModifiers(currentMacroAction.key.modifierMask);
                switch (currentMacroAction.key.type) {
                    case KeystrokeType_Basic:
                        addBasicScancode(currentMacroAction.key.scancode);
                        break;
                    case KeystrokeType_Media:
                        // addMediaScancode(currentMacroAction.key.scancode);
                        break;
                    case KeystrokeType_System:
                        addSystemScancode(currentMacroAction.key.scancode);
                        break;
                }
                return true;
            }
            pressStarted = false;
            deleteModifiers(currentMacroAction.key.modifierMask);
            switch (currentMacroAction.key.type) {
                case KeystrokeType_Basic:
                    deleteBasicScancode(currentMacroAction.key.scancode);
                    break;
                case KeystrokeType_Media:
                    // deleteMediaScancode(currentMacroAction.key.scancode);
                    break;
                case KeystrokeType_System:
                    deleteSystemScancode(currentMacroAction.key.scancode);
                    break;
            }
            break;
        case MacroSubAction_Release:
            deleteModifiers(currentMacroAction.key.modifierMask);
            switch (currentMacroAction.key.type) {
                case KeystrokeType_Basic:
                    deleteBasicScancode(currentMacroAction.key.scancode);
                    break;
                case KeystrokeType_Media:
                    // deleteMediaScancode(currentMacroAction.key.scancode);
                    break;
                case KeystrokeType_System:
                    deleteSystemScancode(currentMacroAction.key.scancode);
                    break;
            }
            break;
        case MacroSubAction_Hold:
            addModifiers(currentMacroAction.key.modifierMask);
            switch (currentMacroAction.key.type) {
                case KeystrokeType_Basic:
                    addBasicScancode(currentMacroAction.key.scancode);
                    break;
                case KeystrokeType_Media:
                    // addMediaScancode(currentMacroAction.key.scancode);
                    break;
                case KeystrokeType_System:
                    addSystemScancode(currentMacroAction.key.scancode);
                    break;
            }
            break;
    }
    return false;
}

bool processCurrentMacroAction(void)
{
    switch (currentMacroAction.type) {
        case MacroActionType_Delay:
            return false;
        case MacroActionType_Key:
            return processKeyMacroAction();
        case MacroActionType_MouseButton:
            return false;
        case MacroActionType_MoveMouse:
            return false;
        case MacroActionType_ScrollMouse:
            return false;
        case MacroActionType_Text:
            return false;
    }
    return false;
}

void Macros_StartMacro(uint8_t index)
{
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
