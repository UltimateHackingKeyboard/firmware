#include "default_layout.h"

KEYBOARD_LAYOUT(defaultKeyboardLayout)={
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_7_AND_AMPERSAND }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F7 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_8_AND_ASTERISK }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F8 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F9 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F10 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F11 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_EQUAL_AND_PLUS }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F12 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_BACKSPACE }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_DELETE }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_U }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_I }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_UP_ARROW }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_O }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_P }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_PRINT_SCREEN }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_SCROLL_LOCK }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_PAUSE }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_BACKSLASH_AND_PIPE }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_Y }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_PAGE_UP }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_J }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_LEFT_ARROW }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_K }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_DOWN_ARROW }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_L }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_RIGHT_ARROW }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_SEMICOLON_AND_COLON }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_ENTER }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_H }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_PAGE_DOWN }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_N }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_MEDIA_MUTE }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_M }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_RIGHT_SHIFT }},
		{Key_NoKey}, //??
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_SPACE }},
		{Key_NoKey}, //??
		{{.type = UHK_KEY_TEST, .value = HID_KEYBOARD_SC_H }}, //Fn
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_RIGHT_ALT }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_RIGHT_GUI }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_RIGHT_CONTROL }},

		//Left half
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_ESCAPE }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_1_AND_EXCLAMATION }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F1 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_2_AND_AT }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F2 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_3_AND_HASHMARK }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F3 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_4_AND_DOLLAR }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F4 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_5_AND_PERCENTAGE }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F5 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_6_AND_CARET }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F6 }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_TAB }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_Q }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_W }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_UP_ARROW }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_E }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_R }},
		{Key_NoKey}, //??
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_T }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_HOME }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_CAPS_LOCK }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_A }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_LEFT_ARROW }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_S }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_DOWN_ARROW }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_D }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_RIGHT_ARROW }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_F }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_DELETE }},
		{Key_NoKey}, //??
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_G }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_END }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_LEFT_SHIFT }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_NON_US_BACKSLASH_AND_PIPE }}, //no idea if this is the right one},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_Z }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_MEDIA_BACKWARD }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_X }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_MEDIA_PLAY }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_C }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_MEDIA_FORWARD }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_V }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_MEDIA_VOLUME_DOWN }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_B }, {.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_MEDIA_VOLUME_UP }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_LEFT_CONTROL }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_LEFT_GUI }},
		{{.type = UHK_KEY_SIMPLE, .key = HID_KEYBOARD_SC_LEFT_ALT }},
		{Key_NoKey}, //Fn
		{Key_NoKey}, //??
		{Key_NoKey}, //Mod
		{Key_NoKey} //??
};

