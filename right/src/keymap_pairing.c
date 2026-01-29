#include <string.h>
#include "keymap_pairing.h"
#include "keymap.h"
#include "key_action.h"
#include "layer.h"
#include "event_scheduler.h"
#include "lufa/HIDClassCommon.h"
#include "macros/keyid_parser.h"

#define PAIRING_KEYMAP_INDEX 0xFF

#define KEYID_TO_SLOT(keyId) ((keyId) / 64)
#define KEYID_TO_INDEX(keyId) ((keyId) % 64)

static uint8_t savedKeymapIndex = 0;

static void setPairingKey(const char* keyIdStr, uint16_t scancode)
{
    uint8_t keyId = KeyIdParser_KeyIdFromString(keyIdStr);
    if (keyId == 255) {
        return;
    }
    uint8_t slotId = KEYID_TO_SLOT(keyId);
    uint8_t keyIndex = KEYID_TO_INDEX(keyId);
    CurrentKeymap[LayerId_Base][slotId][keyIndex] = (key_action_t){
        .type = KeyActionType_Keystroke,
        .keystroke = { .keystrokeType = KeystrokeType_Basic, .scancode = scancode }
    };
}

void Keymap_ActivatePairingKeymap(void)
{
    if (CurrentKeymapIndex == PAIRING_KEYMAP_INDEX) {
        return;
    }

    savedKeymapIndex = CurrentKeymapIndex;
    CurrentKeymapIndex = PAIRING_KEYMAP_INDEX;

    // Clear entire keymap to KeyActionType_None
    memset(CurrentKeymap, 0, sizeof(CurrentKeymap));

    // Set up number keys at default locations
    setPairingKey("0", HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS);
    setPairingKey("1", HID_KEYBOARD_SC_1_AND_EXCLAMATION);
    setPairingKey("2", HID_KEYBOARD_SC_2_AND_AT);
    setPairingKey("3", HID_KEYBOARD_SC_3_AND_HASHMARK);
    setPairingKey("4", HID_KEYBOARD_SC_4_AND_DOLLAR);
    setPairingKey("5", HID_KEYBOARD_SC_5_AND_PERCENTAGE);
    setPairingKey("6", HID_KEYBOARD_SC_6_AND_CARET);
    setPairingKey("7", HID_KEYBOARD_SC_7_AND_AMPERSAND);
    setPairingKey("8", HID_KEYBOARD_SC_8_AND_ASTERISK);
    setPairingKey("9", HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS);

    // Set up control keys
    setPairingKey("backspace", HID_KEYBOARD_SC_BACKSPACE);
    setPairingKey("escape", HID_KEYBOARD_SC_ESCAPE);
    setPairingKey("capsLock", HID_KEYBOARD_SC_ESCAPE);

    EventVector_Set(EventVector_LedMapUpdateNeeded);
}

void Keymap_DeactivatePairingKeymap(void)
{
    if (CurrentKeymapIndex != PAIRING_KEYMAP_INDEX) {
        return;
    }

    SwitchKeymapById(savedKeymapIndex, true);
}
