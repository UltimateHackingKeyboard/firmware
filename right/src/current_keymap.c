#include "action.h"
#include "arduino_hid/ConsumerAPI.h"
#include "arduino_hid/SystemAPI.h"

key_action_t CurrentKeymap[LAYER_COUNT][SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
    // Base layer
    {
        // Right keyboard half
        {
            // Row 1
//            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_7_AND_AMPERSAND }},
//            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_8_AND_ASTERISK }},
//            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS }},
            { .type = KEY_ACTION_TEST, .test = { .testAction = TestAction_DisableUsb }},
            { .type = KEY_ACTION_TEST, .test = { .testAction = TestAction_DisableI2c }},
            { .type = KEY_ACTION_TEST, .test = { .testAction = TestAction_DisableKeyMatrixScan }},

            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_EQUAL_AND_PLUS }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSPACE }},

            // Row 2
//            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_U }},
//            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_I }},
            { .type = KEY_ACTION_TEST, .test = { .testAction = TestAction_DisableLedSdb }},
            { .type = KEY_ACTION_TEST, .test = { .testAction = TestAction_DisableLedFetPwm }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_O }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_P }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSLASH_AND_PIPE }},
//            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_Y }},
            { .type = KEY_ACTION_TEST, .test = { .testAction = TestAction_DisableLedDriverPwm }},

            // Row 3
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_J }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_K }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_L }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_SEMICOLON_AND_COLON }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_ENTER }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_H }},

            // Row 4
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_N }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_M }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }},
            { .type = KEY_ACTION_NONE },

            // Row 5
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_SPACE }},
            { .type = KEY_ACTION_SWITCH_LAYER, .switchLayer = { .layer = LAYER_ID_MOD }},
            { .type = KEY_ACTION_SWITCH_LAYER, .switchLayer = { .layer = LAYER_ID_FN }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }},
        },

        // Left keyboard half
        {
            // Row 1
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_1_AND_EXCLAMATION }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_2_AND_AT }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_3_AND_HASHMARK }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_4_AND_DOLLAR }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_5_AND_PERCENTAGE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_6_AND_CARET }},

            // Row 2
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_TAB }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_Q }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_W }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_E }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_R }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_T }},

            // Row 3
            { .type = KEY_ACTION_SWITCH_LAYER, .switchLayer = { .layer = LAYER_ID_MOUSE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_A }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_S }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_D }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_G }},

            // Row 4
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_NON_US_BACKSLASH_AND_PIPE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_Z }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_X }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_C }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_V }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_B }},

            // Row 5
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }},
            { .type = KEY_ACTION_SWITCH_LAYER, .switchLayer = { .layer = LAYER_ID_FN }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_SPACE }},
            { .type = KEY_ACTION_SWITCH_LAYER, .switchLayer = { .layer = LAYER_ID_MOD }},
            { .type = KEY_ACTION_NONE },
        }
    },

    // Mod layer
    {
        // Right keyboard half
        {
            // Row 1
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F7 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F8 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F9 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F10 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F11 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F12 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }},

            // Row 2
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_HOME }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_UP_ARROW }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_END }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_PRINT_SCREEN }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_SCROLL_LOCK }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_PAUSE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_UP }},

            // Row 3
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ARROW }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_DOWN_ARROW }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ARROW }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_INSERT }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_DOWN }},

            // Row 4
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }},
            { .type = KEY_ACTION_NONE },

            // Row 5
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_SWITCH_LAYER, .switchLayer = { .layer = LAYER_ID_MOD }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }},
        },

        // Left keyboard half
        {
            // Row 1
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_ESCAPE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F1 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F2 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F3 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F4 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F5 }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_F6 }},

            // Row 2
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_UP, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}, // [<] tab prev
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_T, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}, // [+] tab new
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_DOWN, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}, // [>] tab next
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 3
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ARROW, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_LEFTALT }}, // workspace prev
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_TAB, .modifiers = HID_KEYBOARD_MODIFIER_LEFTALT }}, // window switch
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ARROW, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_LEFTALT }}, // workspace next
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 4
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_W, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}, // [x] tab close
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 5
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_SWITCH_LAYER, .switchLayer = { .layer = LAYER_ID_MOD }},
            { .type = KEY_ACTION_NONE },
        }
    },

    // Fn layer
    {
        // Right keyboard half
        {
            // Row 1
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_SYSTEM, .scancode = SYSTEM_WAKE_UP }},
            { .type = KEY_ACTION_NONE },

            // Row 2
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = MEDIA_PLAY_PAUSE }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = MEDIA_VOLUME_UP }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = MEDIA_STOP }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_SYSTEM, .scancode = SYSTEM_SLEEP }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_SYSTEM, .scancode = SYSTEM_POWER_DOWN }},
            { .type = KEY_ACTION_NONE },

            // Row 3
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = MEDIA_PREVIOUS }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = MEDIA_VOLUME_DOWN }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = MEDIA_NEXT }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 4
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = MEDIA_VOLUME_MUTE }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }},
            { .type = KEY_ACTION_NONE },

            // Row 5
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_SWITCH_LAYER, .switchLayer = { .layer = LAYER_ID_FN }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }},
        },

        // Left keyboard half
        {
            // Row 1
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 2
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = HID_CONSUMER_AC_CANCEL }}, // HID_CONSUMER_AC_STOP
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = CONSUMER_BROWSER_REFRESH }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 3
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = CONSUMER_BROWSER_BACK }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = CONSUMER_EXPLORER }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = CONSUMER_BROWSER_FORWARD }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 4
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = CONSUMER_SCREENSAVER }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = HID_CONSUMER_AC_SEARCH }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = CONSUMER_CALCULATOR }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .keystrokeType = KEYSTROKE_MEDIA, .scancode = HID_CONSUMER_EJECT }},
            { .type = KEY_ACTION_NONE },

            // Row 5
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }},
            { .type = KEY_ACTION_SWITCH_LAYER, .switchLayer = { .layer = LAYER_ID_FN }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
        }
    },

    // Mouse layer
    {
        // Right keyboard half
        {
            // Row 1
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 2
            { .type = KEY_ACTION_MOUSE, .mouse = { .buttonActions = MOUSE_BUTTON_4 }},
            { .type = KEY_ACTION_MOUSE, .mouse = { .moveActions = MOUSE_MOVE_UP }},
            { .type = KEY_ACTION_MOUSE, .mouse = { .buttonActions = MOUSE_BUTTON_5 }},
            { .type = KEY_ACTION_MOUSE, .mouse = { .buttonActions = MOUSE_BUTTON_6 }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_MOUSE, .mouse = { .scrollActions = MOUSE_SCROLL_UP }},

            // Row 3
            { .type = KEY_ACTION_MOUSE, .mouse = { .moveActions = MOUSE_MOVE_LEFT }},
            { .type = KEY_ACTION_MOUSE, .mouse = { .moveActions = MOUSE_MOVE_DOWN }},
            { .type = KEY_ACTION_MOUSE, .mouse = { .moveActions = MOUSE_MOVE_RIGHT }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_MOUSE, .mouse = { .scrollActions = MOUSE_SCROLL_DOWN }},

            // Row 4
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }},
            { .type = KEY_ACTION_NONE },

            // Row 5
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }},
        },

        // Left keyboard half
        {
            // Row 1
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 2
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 3
            { .type = KEY_ACTION_SWITCH_LAYER, .switchLayer = { .layer = LAYER_ID_MOUSE }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_MOUSE, .mouse = { .buttonActions = MOUSE_BUTTON_RIGHT }},
            { .type = KEY_ACTION_MOUSE, .mouse = { .buttonActions = MOUSE_BUTTON_MIDDLE }},
            { .type = KEY_ACTION_MOUSE, .mouse = { .buttonActions = MOUSE_BUTTON_LEFT }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 4
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_NONE },

            // Row 5
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }},
            { .type = KEY_ACTION_KEYSTROKE, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }},
            { .type = KEY_ACTION_NONE },
            { .type = KEY_ACTION_MOUSE, .mouse = { .moveActions = MOUSE_DECELERATE }},
            { .type = KEY_ACTION_MOUSE, .mouse = { .moveActions = MOUSE_ACCELERATE }},
            { .type = KEY_ACTION_NONE },
        }
    },
};
