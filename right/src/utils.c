#include <string.h>
#include "utils.h"
#include "key_action.h"
#include "key_states.h"

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

key_coordinates_t Utils_KeyIdToKeyCoordinates(uint16_t keyId)
{
    return (key_coordinates_t) { .slotId = keyId / 64, .inSlotId = keyId % 64, };
}

//TODO: Should probably be realized by the above KeyStateToKeyId
void Utils_DecodeId(uint16_t keyid, uint8_t* outSlotId, uint8_t* outSlotIdx)
{
    //we want to guarantee that slot ids will remain the same even if slot size
    //changes in the future, therefore hardcoded 64 is indeed correct
    *outSlotId = keyid/64;
    *outSlotIdx = keyid%64;
}

static void Utils_SetStatusScancodeCharacter(uint8_t scancode)
{
    Macros_SetStatusChar(MacroShortcutParser_ScancodeToCharacter(scancode));
}

static bool reportModifiers(uint8_t modifiers)
{
    bool modifierFound = false;
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t modifier = 1 << i;
        if (modifiers & modifier) {
            modifierFound = true;
            switch (modifier) {
                case HID_KEYBOARD_MODIFIER_LEFTCTRL:
                    Macros_SetStatusString("LC", NULL);
                    break;
                case HID_KEYBOARD_MODIFIER_LEFTSHIFT:
                    Macros_SetStatusString("LS", NULL);
                    break;
                case HID_KEYBOARD_MODIFIER_LEFTALT:
                    Macros_SetStatusString("LA", NULL);
                    break;
                case HID_KEYBOARD_MODIFIER_LEFTGUI:
                    Macros_SetStatusString("LG", NULL);
                    break;
                case HID_KEYBOARD_MODIFIER_RIGHTCTRL:
                    Macros_SetStatusString("RC", NULL);
                    break;
                case HID_KEYBOARD_MODIFIER_RIGHTSHIFT:
                    Macros_SetStatusString("RS", NULL);
                    break;
                case HID_KEYBOARD_MODIFIER_RIGHTALT:
                    Macros_SetStatusString("RA", NULL);
                    break;
                case HID_KEYBOARD_MODIFIER_RIGHTGUI:
                    Macros_SetStatusString("RG", NULL);
      break;
            }
        }
    }
    return modifierFound;
}

void Utils_PrintReport(const char* prefix, usb_basic_keyboard_report_t* report)
{
    Macros_SetStatusString(prefix, NULL);

    Macros_SetStatusString(" ", NULL);

    bool modifierFound = reportModifiers(report->modifiers);

    if (modifierFound) {
        Macros_SetStatusString(" ", NULL);
    }

    UsbBasicKeyboard_ForeachScancode(report, &Utils_SetStatusScancodeCharacter);

    Macros_SetStatusString("\n", NULL);
}

void Utils_SafeStrCopy(char* target, const char* src, uint8_t max) {
    uint8_t stringlength = MIN(strlen(src)+1, (max));
    memcpy(target, src, stringlength);
    target[stringlength-1] = '\0';
}

const char* Utils_KeyAbbreviation(key_state_t* keyState)
{
    //maps according to static enUs mapping
    uint8_t keyId = Utils_KeyStateToKeyId(keyState);
    return MacroKeyIdParser_KeyIdToAbbreviation(keyId);
}
