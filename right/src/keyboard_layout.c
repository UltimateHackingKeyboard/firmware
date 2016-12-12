#include "keyboard_layout.h"
#include "led_driver.h"
#include "layer.h"

static uint8_t keyMasks[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

uint8_t prevKeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

static inline __attribute__((always_inline)) uhk_key_t getKeycode(uint8_t slotId, uint8_t keyId)
{
    if (keyId < MAX_KEY_COUNT_PER_MODULE) {
        if (keyMasks[slotId][keyId]!=0 && keyMasks[slotId][keyId]!=ActiveLayer) {
            // Mask out key presses after releasing modifier keys
            return (uhk_key_t){.type = UHK_KEY_NONE};
        }

        uhk_key_t k = CurrentKeymap[ActiveLayer][slotId][keyId];
        keyMasks[slotId][keyId] = ActiveLayer;

        return k;
    } else {
        return (uhk_key_t){.type = UHK_KEY_NONE};
    }
}

static void clearKeymasks(const uint8_t *leftKeyStates, const uint8_t *rightKeyStates){
    int i;
    for (i=0; i < MAX_KEY_COUNT_PER_MODULE; i++){
        if (rightKeyStates[i]==0){
            keyMasks[SLOT_ID_RIGHT_KEYBOARD_HALF][i] = 0;
        }

        if (leftKeyStates[i]==0) {
            keyMasks[SLOT_ID_LEFT_KEYBOARD_HALF][i] = 0;
        }
    }
}

bool pressKey(uhk_key_t key, int scancodeIdx, usb_keyboard_report_t *report) {
    if (key.type != UHK_KEY_SIMPLE) {
        return false;
    }

    if (!key.simple.key) {
        return false;
    }

    for (uint8_t i = 0; i < 8; i++) {
        if (key.simple.mods & (1 << i) || key.simple.key == HID_KEYBOARD_SC_LEFT_CONTROL + i) {
            report->modifiers |= (1 << i);
        }
    }

    report->scancodes[scancodeIdx] = key.simple.key;
    return true;
}

bool key_toggled_on(const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId) {
    return (!prevKeyStates[keyId]) && currKeyStates[keyId];
}

bool key_is_pressed(const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId) {
    return currKeyStates[keyId];
}

bool key_toggled_off(const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId) {
    return (!currKeyStates[keyId]) && prevKeyStates[keyId];
}

bool handleKey(uhk_key_t key, int scancodeIdx, usb_keyboard_report_t *report, const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId) {
    switch (key.type) {
    case UHK_KEY_SIMPLE:
        if (key_is_pressed(prevKeyStates, currKeyStates, keyId)) {
            return pressKey(key, scancodeIdx, report);
        }
        break;
    case UHK_KEY_LAYER:
        if (key_toggled_on(prevKeyStates, currKeyStates, keyId)) {
            Layer_MoveTo(key.layer.target);
        }
        if (key_toggled_off(prevKeyStates, currKeyStates, keyId)) {
            Layer_MoveToBase();
        }
        return false;
        break;
    default:
        break;
    }
    return false;
}

void fillKeyboardReport(usb_keyboard_report_t *report, const uint8_t *leftKeyStates, const uint8_t *rightKeyStates) {
    int scancodeIdx = 0;

    clearKeymasks(leftKeyStates, rightKeyStates);

    for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
        if (scancodeIdx >= USB_KEYBOARD_MAX_KEYS) {
            break;
        }

        uhk_key_t code = getKeycode(SLOT_ID_RIGHT_KEYBOARD_HALF, keyId);

        if (handleKey(code, scancodeIdx, report, prevKeyStates[SLOT_ID_RIGHT_KEYBOARD_HALF], rightKeyStates, keyId)) {
            scancodeIdx++;
        }
    }

    for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
        if (scancodeIdx >= USB_KEYBOARD_MAX_KEYS) {
            break;
        }

        uhk_key_t code = getKeycode(SLOT_ID_LEFT_KEYBOARD_HALF, keyId);

        if (handleKey(code, scancodeIdx, report, prevKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], leftKeyStates, keyId)) {
            scancodeIdx++;
        }
    }

    memcpy(prevKeyStates[SLOT_ID_RIGHT_KEYBOARD_HALF], rightKeyStates, KEY_STATE_COUNT);
    memcpy(prevKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], leftKeyStates, KEY_STATE_COUNT);
}
