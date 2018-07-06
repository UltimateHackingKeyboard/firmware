#include "test_mode.h"
#include "led_display.h"
#include "key_action.h"
#include "keymap.h"

static const key_action_t TestKeymap[1][2][MAX_KEY_COUNT_PER_MODULE] = {
    // Base layer
    {
        // Right keyboard half
        {
            // Row 1
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_7_AND_AMPERSAND }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_8_AND_ASTERISK }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_EQUAL_AND_PLUS }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSPACE }},

            // Row 2
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_U }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_I }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_O }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_P }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSLASH_AND_PIPE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Y }},

            // Row 3
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_J }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_K }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_L }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SEMICOLON_AND_COLON }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_PLUS }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_H }},

            // Row 4
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_N }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_M }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }},
            { .type = KeyActionType_None },

            // Row 5
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_6_AND_RIGHT_ARROW }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_ASTERISK }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_7_AND_HOME }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_9_AND_PAGE_UP }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_0_AND_INSERT }},
        },

        // Left keyboard half
        {
            // Row 1
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_1_AND_EXCLAMATION }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_2_AND_AT }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_3_AND_HASHMARK }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_4_AND_DOLLAR }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_5_AND_PERCENTAGE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_6_AND_CARET }},

            // Row 2
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ESCAPE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Q }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_W }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_E }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_R }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_T }},

            // Row 3
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_MINUS }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_A }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_S }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_D }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_G }},

            // Row 4
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_NON_US_BACKSLASH_AND_PIPE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Z }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_X }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_C }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_V }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_B }},

            // Row 5
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_1_AND_END }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_3_AND_PAGE_DOWN }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_4_AND_LEFT_ARROW }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_SLASH }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_5 }},
            { .type = KeyActionType_None },
        }
    }
};

void TestMode_Activate(void)
{
    memcpy(&CurrentKeymap, &TestKeymap, sizeof TestKeymap);
    LedDisplay_SetText(3, "TES");
}
