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

void addScancode(uint16_t scancode, macro_sub_action_t type)
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

void deleteScancode(uint16_t scancode, macro_sub_action_t type)
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
    static bool pressStarted;

    switch (currentMacroAction.key.action) {
        case MacroSubAction_Tap:
            if (!pressStarted) {
                pressStarted = true;
                addModifiers(currentMacroAction.key.modifierMask);
                addScancode(currentMacroAction.key.scancode, currentMacroAction.key.type);
                return true;
            }
            pressStarted = false;
            deleteModifiers(currentMacroAction.key.modifierMask);
            deleteScancode(currentMacroAction.key.scancode, currentMacroAction.key.type);
            break;
        case MacroSubAction_Release:
            deleteModifiers(currentMacroAction.key.modifierMask);
            deleteScancode(currentMacroAction.key.scancode, currentMacroAction.key.type);
            break;
        case MacroSubAction_Press:
            addModifiers(currentMacroAction.key.modifierMask);
            addScancode(currentMacroAction.key.scancode, currentMacroAction.key.type);
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
        Timer_SetCurrentTime(&delayStart);
        inDelay = true;
    }
    return inDelay;
}

bool processMouseButtonAction(void)
{
    static bool pressStarted;

    switch (currentMacroAction.key.action) {
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
    MacroMouseReport.x = currentMacroAction.moveMouse.x;
    MacroMouseReport.y = currentMacroAction.moveMouse.y;
    return false;
}

bool processScrollMouseAction(void)
{
    MacroMouseReport.wheelX = currentMacroAction.scrollMouse.x;
    MacroMouseReport.wheelY = currentMacroAction.scrollMouse.y;
    return false;
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
