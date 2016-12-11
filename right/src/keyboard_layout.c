#include "keyboard_layout.h"
#include "led_driver.h"

static uint8_t keyMasks[LAYOUT_KEY_COUNT];

static inline __attribute__((always_inline)) uhk_key_t getKeycode(KEYBOARD_LAYOUT(layout), uint8_t keyId, uint8_t modifierState)
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


static uint8_t getModifierState(const uint8_t *leftKeyStates, const uint8_t *rightKeyStates){
	uint8_t mod = 0;
	if (leftKeyStates[KEYID_LEFT_MOD]) {
		mod |= MODIFIER_MOD_PRESSED;
	}
	if (leftKeyStates[KEYID_LEFT_FN] | rightKeyStates[KEYID_RIGHT_FN]) {
		mod |= MODIFIER_FN_PRESSED;
	}

	return mod;
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

void handleKey(uhk_key_t key, int scancodeIdx, usb_keyboard_report_t *report) {
  switch (key.type) {
  case UHK_KEY_SIMPLE:
    if (key.key) {
      if (key.key >= HID_KEYBOARD_SC_LEFT_CONTROL && key.key <= HID_KEYBOARD_SC_RIGHT_GUI) {
        uint8_t k = key.key - HID_KEYBOARD_SC_LEFT_CONTROL;
        report->modifiers |= (1 << k);
      } else {
        report->scancodes[scancodeIdx++] = key.key;
      }
    }
    break;
  default:
    break;
  }
}

void fillKeyboardReport(usb_keyboard_report_t *report, const uint8_t *leftKeyStates, const uint8_t *rightKeyStates, KEYBOARD_LAYOUT(layout)){
	int scancodeIdx = 0;

	clearKeymasks(leftKeyStates, rightKeyStates);
	uint8_t modifierState=getModifierState(leftKeyStates, rightKeyStates);

	for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
		if (scancodeIdx>=USB_KEYBOARD_MAX_KEYS) {
			break;
		}

		if (rightKeyStates[keyId]) {
			uhk_key_t code=getKeycode(layout, keyId, modifierState);

      handleKey(code, scancodeIdx, report);
		}
	}

	for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
		if (scancodeIdx>=USB_KEYBOARD_MAX_KEYS) {
			break;
		}

		if (leftKeyStates[keyId]) {
			uhk_key_t code=getKeycode(layout, LAYOUT_LEFT_OFFSET+keyId, modifierState);

      handleKey(code, scancodeIdx, report);
		}
	}
}
