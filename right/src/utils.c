#include <string.h>
#include "utils.h"
#include "key_action.h"
#include "key_states.h"
#include "macros/keyid_parser.h"
#include "timer.h"

#ifdef __ZEPHYR__
#include "device.h"
#else
#include "keymap.h"
#endif

#include "layer.h"
#include "macros/core.h"
#include "macros/keyid_parser.h"
#include "macros/status_buffer.h"
#include "macros/shortcut_parser.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "led_display.h"

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if !DEVICE_IS_UHK_DONGLE
//this is noop at the moment, prepared for time when MAX_KEY_COUNT_PER_MODULE changes
//the purpose is to preserve current keyids
static uint16_t recodeId(uint16_t newFormat, uint16_t fromBase, uint16_t toBase)
{
    return toBase * (newFormat / fromBase) + (newFormat % fromBase);
}
#endif

uint16_t Utils_KeyStateToKeyId(key_state_t* key)
{
#if DEVICE_IS_UHK_DONGLE
    return 0;
#else
    if (key == NULL) {
        return 0;
    }
    uint32_t ptr1 = (uint32_t)(key_state_t*)key;
    uint32_t ptr2 = (uint32_t)(key_state_t*)&(KeyStates[0][0]);
    uint32_t res = (ptr1 - ptr2) / sizeof(key_state_t);
    return recodeId(res, MAX_KEY_COUNT_PER_MODULE, 64);
#endif
}

key_state_t* Utils_KeyIdToKeyState(uint16_t keyid)
{
#if DEVICE_IS_UHK_DONGLE
    return NULL;
#else
    return &(((key_state_t*)KeyStates)[recodeId(keyid, 64, MAX_KEY_COUNT_PER_MODULE)]);
#endif
}

const char* Utils_KeyStateToKeyAbbreviation(key_state_t* key)
{
    if (key == NULL) {
        return NULL;
    }
    uint16_t keyId = Utils_KeyStateToKeyId(key);
    return MacroKeyIdParser_KeyIdToAbbreviation(keyId);
}

key_coordinates_t Utils_KeyIdToKeyCoordinates(uint16_t keyId)
{
    return (key_coordinates_t) { .slotId = keyId / 64, .inSlotId = keyId % 64, };
}

uint16_t Utils_KeyCoordinatesToKeyId(uint8_t slotId, uint8_t keyIdx)
{
    uint16_t keyId = slotId * 64 + keyIdx;
    return keyId;
}

//TODO: Should probably be realized by the above KeyStateToKeyId
void Utils_DecodeId(uint16_t keyid, uint8_t* outSlotId, uint8_t* outSlotIdx)
{
    //we want to guarantee that slot ids will remain the same even if slot size
    //changes in the future, therefore hardcoded 64 is indeed correct
    *outSlotId = keyid/64;
    *outSlotIdx = keyid%64;
}

static char usbReportString[32];

const char* Utils_GetUsbReportString(const usb_basic_keyboard_report_t* report)
{
    char *p = usbReportString;
    char *end = usbReportString + sizeof(usbReportString) - 1;

    static const struct { uint8_t mask; const char id[3]; } mods[] = {
        { HID_KEYBOARD_MODIFIER_LEFTCTRL,   "LC" },
        { HID_KEYBOARD_MODIFIER_LEFTSHIFT,  "LS" },
        { HID_KEYBOARD_MODIFIER_LEFTALT,    "LA" },
        { HID_KEYBOARD_MODIFIER_LEFTGUI,    "LG" },
        { HID_KEYBOARD_MODIFIER_RIGHTCTRL,  "RC" },
        { HID_KEYBOARD_MODIFIER_RIGHTSHIFT, "RS" },
        { HID_KEYBOARD_MODIFIER_RIGHTALT,   "RA" },
        { HID_KEYBOARD_MODIFIER_RIGHTGUI,   "RG" },
    };

    for (uint8_t i = 0; i < sizeof(mods)/sizeof(mods[0]) && p < end - 2; i++) {
        if (report->modifiers & mods[i].mask) {
            *p++ = mods[i].id[0];
            *p++ = mods[i].id[1];
        }
    }

    bool hasMods = (p > usbReportString);
    bool hasScancodes = UsbBasicKeyboard_ScancodeCount(report) > 0;

    if (hasMods && hasScancodes && p < end) {
        *p++ = '-';
    }

    bool first = !hasMods;
    for (uint8_t sc = 4; sc < 232 && p < end - 4; sc++) {
        // Skip modifier scancodes (0xE0-0xE7) - they're already printed as modifiers above
        if (UsbBasicKeyboard_IsModifier(sc)) {
            continue;
        }
        if (UsbBasicKeyboard_ContainsScancode(report, sc)) {
            if (!first && p < end) *p++ = ' ';
            first = false;
            char c = MacroShortcutParser_ScancodeToCharacter(sc);
            if (c == DEFAULT_SCANCODE_ABBREVIATION) {
                // Print as 3-digit zero-padded number
                *p++ = '0' + (sc / 100);
                *p++ = '0' + ((sc / 10) % 10);
                *p++ = '0' + (sc % 10);
            } else {
                *p++ = c;
            }
        }
    }

    *p = '\0';
    return usbReportString;
}

void Utils_PrintReport(const char* prefix, usb_basic_keyboard_report_t* report)
{
    Macros_SetStatusString(prefix, NULL);
    Macros_SetStatusString(" ", NULL);
    Macros_SetStatusString(Utils_GetUsbReportString(report), NULL);
    Macros_SetStatusString("\n", NULL);
}

uint8_t Utils_SafeStrCopy(char* target, const char* src, uint8_t max) {
    uint8_t stringlength = MIN(strlen(src)+1, (max));
    memcpy(target, src, stringlength);
    target[stringlength-1] = '\0';
    return stringlength-1;
}

const char* Utils_KeyAbbreviation(key_state_t* keyState)
{
    //maps according to static enUs mapping
    uint8_t keyId = Utils_KeyStateToKeyId(keyState);
    return MacroKeyIdParser_KeyIdToAbbreviation(keyId);
}

// Try resending a report for 512ms. Give up if it doesn't succeed by then.
bool ShouldResendReport(bool statusOk, uint8_t* counter) {

    if (statusOk) {
        *counter = 0;
        return false;
    }

    // keep this low, since the actual delay this causes with a full queue is
    // queueLength * maxDelay
    const uint16_t maxDelay = 128; //ms
    const uint8_t granularity = 16; //ms
    uint8_t minimizedTime = Timer_GetCurrentTime() / granularity;

    if (*counter == 0) {
        *counter = minimizedTime;
        return true;
    } else if ((uint8_t)(minimizedTime - *counter) < (maxDelay / granularity)) {
        return true;
    } else {
        *counter = 0;
        return false;
    }
}
