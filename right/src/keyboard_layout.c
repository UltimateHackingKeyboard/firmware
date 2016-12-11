#include "keyboard_layout.h"
#include "led_driver.h"

static uint8_t keyMasks[LAYOUT_KEY_COUNT];

static uint8_t modifierState = 0;

static inline __attribute__((always_inline)) uhk_key_t getKeycode(KEYBOARD_LAYOUT(layout), uint8_t keyId)
{
    if (keyId<LAYOUT_KEY_COUNT) {
        if (keyMasks[keyId]!=0 && keyMasks[keyId]!=modifierState){
            //Mask out key presses after releasing modifier keys
      return (uhk_key_t) { .raw = 0 };
        }

        uhk_key_t k = layout[keyId][modifierState];
        keyMasks[keyId] = modifierState;

        if (k.raw==0) {
            k = layout[keyId][0];
        }

        return k;
    } else {
        return (uhk_key_t) { .raw = 0 };
    }
}

static void clearKeymasks(const uint8_t *leftKeyStates, const uint8_t *rightKeyStates){
    int i;
    for (i=0; i<KEY_STATE_COUNT; ++i){
        if (rightKeyStates[i]==0){
            keyMasks[i] = 0;
        }

        if (leftKeyStates[i]==0){
            keyMasks[LAYOUT_LEFT_OFFSET+i] = 0;
        }
    }
}

static uint8_t LEDVal = 0;
static int8_t LEDstep = 10;

bool pressKey(uhk_key_t key, int scancodeIdx, usb_keyboard_report_t *report) {
  if (key.type != UHK_KEY_SIMPLE)
    return false;

  if (!key.simple.key)
    return false;

  for (uint8_t i = 0; i < 8; i++) {
    if (key.simple.mods & (1 << i) ||
        key.simple.key == HID_KEYBOARD_SC_LEFT_CONTROL + i) {
      report->modifiers |= (1 << i);
    }
  }
  report->scancodes[scancodeIdx] = key.simple.key;
  return true;
}

bool layerOn(uhk_key_t key) {
  modifierState |= (1 << (key.layer.target - 1));
  return false;
}

bool layerOff(uhk_key_t key) {
  modifierState &= ~(1 << (key.layer.target - 1));
  return false;
}

bool handleKey(uhk_key_t key, int scancodeIdx, usb_keyboard_report_t *report, uint8_t keyState) {
  switch (key.type) {
  case UHK_KEY_SIMPLE:
    if (keyState)
      return pressKey (key, scancodeIdx, report);
    break;
  case UHK_KEY_LAYER:
    if (keyState)
      return layerOn (key);
    return layerOff (key);
    break;
  default:
    break;
  }
  return false;
}

void fillKeyboardReport(usb_keyboard_report_t *report, const uint8_t *leftKeyStates, const uint8_t *rightKeyStates, KEYBOARD_LAYOUT(layout)) {
    int scancodeIdx = 0;

    clearKeymasks(leftKeyStates, rightKeyStates);

    for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
        if (scancodeIdx>=USB_KEYBOARD_MAX_KEYS) {
            break;
        }

    uhk_key_t code=getKeycode(layout, keyId);

    if (handleKey(code, scancodeIdx, report, rightKeyStates[keyId])) {
      scancodeIdx++;
    }
    }

    for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
        if (scancodeIdx>=USB_KEYBOARD_MAX_KEYS) {
            break;
        }

    uhk_key_t code=getKeycode(layout, LAYOUT_LEFT_OFFSET+keyId);

    if (handleKey(code, scancodeIdx, report, leftKeyStates[keyId])) {
      scancodeIdx++;
    }
    }
}
