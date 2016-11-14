#include "keyboard_layout.h"

static uint8_t keyMasks[LAYOUT_KEY_COUNT];

static inline __attribute__((always_inline)) uint8_t getKeycode(KEYBOARD_LAYOUT(layout), uint8_t keyId, uint8_t modifierState)
{
	if (keyId<LAYOUT_KEY_COUNT) {
		if (keyMasks[keyId]!=0 && keyMasks[keyId]!=modifierState){
			//Mask out key presses after releasing modifier keys
			return 0;
		}

		uint8_t k = layout[keyId][modifierState];
		keyMasks[keyId] = modifierState;

		if (k==0) {
			k = layout[keyId][0];
		}

		return k;
	} else {
		return 0;
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

void fillKeyboardReport(usb_keyboard_report_t *report, const uint8_t *leftKeyStates, const uint8_t *rightKeyStates, KEYBOARD_LAYOUT(layout)){
	int scancodeIdx = 0;

	clearKeymasks(leftKeyStates, rightKeyStates);
	uint8_t modifierState=getModifierState(leftKeyStates, rightKeyStates);

	for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
		if (scancodeIdx>=USB_KEYBOARD_MAX_KEYS) {
			break;
		}

		if (rightKeyStates[keyId]) {
			uint8_t code=getKeycode(layout, keyId, modifierState);
			if (code) {
				report->scancodes[scancodeIdx++] = code;
			}
		}
	}

	for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
		if (scancodeIdx>=USB_KEYBOARD_MAX_KEYS) {
			break;
		}

		if (leftKeyStates[keyId]) {
			uint8_t code=getKeycode(layout, LAYOUT_LEFT_OFFSET+keyId, modifierState);
			if (code) {
				report->scancodes[scancodeIdx++] = code;
			}
		}
	}
}
