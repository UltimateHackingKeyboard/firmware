#include "arduino_hid/ConsumerAPI.h"
#include "arduino_hid/SystemAPI.h"
#include "keymap.h"
#include "led_display.h"
#include "config_parser/parse_keymap.h"
#include "config_parser/config_globals.h"
#include "macros.h"

keymap_reference_t AllKeymaps[MAX_KEYMAP_NUM] = {
    {
        .abbreviation = "FTY",
        .offset = 0,
        .abbreviationLen = 3
    }
};

uint8_t AllKeymapsCount;
uint8_t DefaultKeymapIndex;
uint8_t CurrentKeymapIndex = 0;

void SwitchKeymapById(uint8_t index)
{
    CurrentKeymapIndex = index;
    ValidatedUserConfigBuffer.offset = AllKeymaps[index].offset;
    ParseKeymap(&ValidatedUserConfigBuffer, index, AllKeymapsCount, AllMacrosCount);
    LedDisplay_UpdateText();
}

bool SwitchKeymapByAbbreviation(uint8_t length, char *abbrev)
{
    for (uint8_t i=0; i<AllKeymapsCount; i++) {
        keymap_reference_t *keymap = AllKeymaps + i;
        if (keymap->abbreviationLen == length && memcmp(keymap->abbreviation, abbrev, length) == 0) {
            SwitchKeymapById(i);
            return true;
        }
    }
    return false;
}

// The factory keymap is initialized before it gets overwritten by the default keymap of the EEPROM.
key_action_t CurrentKeymap[LAYER_COUNT][SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
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
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ENTER }},
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
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SPACE }},
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mod }},
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }},
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
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_TAB }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Q }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_W }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_E }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_R }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_T }},

            // Row 3
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mouse }},
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
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }},
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SPACE }},
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mod }},
            { .type = KeyActionType_None },
        },

        // Left module
        {
            // Row 1
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSPACE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ENTER }},
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_LeftClick },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_MiddleClick },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_RightClick },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_6_AND_CARET }},

            // Row 2
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_TAB }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Q }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_W }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_E }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_R }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_T }},

            // Row 3
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mouse }},
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
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }},
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SPACE }},
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mod }},
            { .type = KeyActionType_None },
        },

        // Right module
        {
            // Row 1
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_RightClick },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_LeftClick },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_2_AND_AT }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_3_AND_HASHMARK }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_4_AND_DOLLAR }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_5_AND_PERCENTAGE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_6_AND_CARET }},

            // Row 2
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_TAB }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Q }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_W }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_E }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_R }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_T }},

            // Row 3
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mouse }},
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
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }},
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SPACE }},
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mod }},
            { .type = KeyActionType_None },
        }
    },

    // Mod layer
    {
        // Right keyboard half
        {
            // Row 1
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F7 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F8 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F9 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F10 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F11 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F12 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }},

            // Row 2
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_HOME }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_UP_ARROW }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_END }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PRINT_SCREEN }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SCROLL_LOCK }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAUSE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_UP }},

            // Row 3
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ARROW }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DOWN_ARROW }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ARROW }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_INSERT }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_DOWN }},

            // Row 4
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ESCAPE }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_APPLICATION }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }},
            { .type = KeyActionType_None },

            // Row 5
            { .type = KeyActionType_None },
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mod }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }},
        },

        // Left keyboard half
        {
            // Row 1
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ESCAPE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F1 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F2 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F3 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F4 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F5 }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F6 }},

            // Row 2
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ESCAPE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_UP, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}, // [<] tab prev
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_T, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}, // [+] tab new
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_DOWN, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}, // [>] tab next
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 3
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_CAPS_LOCK }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ARROW, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_LEFTALT }}, // workspace prev
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_TAB, .modifiers = HID_KEYBOARD_MODIFIER_LEFTALT }}, // window switch
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ARROW, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_LEFTALT }}, // workspace next
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 4
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_W, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}, // [x] tab close
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 5
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mod }},
            { .type = KeyActionType_None },
        }
    },

    // Fn layer
    {
        // Right keyboard half
        {
            // Row 1
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_System, .scancode = SYSTEM_WAKE_UP }},
            { .type = KeyActionType_None },

            // Row 2
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_PLAY_PAUSE }},
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_VOLUME_UP }},
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_STOP }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_System, .scancode = SYSTEM_SLEEP }},
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_System, .scancode = SYSTEM_POWER_DOWN }},
            { .type = KeyActionType_None },

            // Row 3
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_PREVIOUS }},
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_VOLUME_DOWN }},
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_NEXT }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 4
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_VOLUME_MUTE }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }},
            { .type = KeyActionType_None },

            // Row 5
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }},
        },

        // Left keyboard half
        {
            // Row 1
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 2
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = HID_CONSUMER_AC_CANCEL }}, // HID_CONSUMER_AC_STOP
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_BROWSER_REFRESH }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 3
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_BROWSER_BACK }},
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_EXPLORER }},
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_BROWSER_FORWARD }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 4
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_SCREENSAVER }},
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = HID_CONSUMER_AC_SEARCH }},
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_CALCULATOR }},
            { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = HID_CONSUMER_EJECT }},
            { .type = KeyActionType_None },

            // Row 5
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }},
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
        }
    },

    // Mouse layer
    {
        // Right keyboard half
        {
            // Row 1
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 2
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_Button_4 },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_MoveUp },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_Button_5 },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_Button_6 },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_ScrollUp },

            // Row 3
            { .type = KeyActionType_Mouse, .mouseAction= SerializedMouseAction_MoveLeft },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_MoveDown },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_MoveRight },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_ScrollDown },

            // Row 4
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }},
            { .type = KeyActionType_None },

            // Row 5
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }},
        },

        // Left keyboard half
        {
            // Row 1
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 2
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 3
            { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mouse }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_RightClick },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_MiddleClick },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_LeftClick },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 4
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },
            { .type = KeyActionType_None },

            // Row 5
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }},
            { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }},
            { .type = KeyActionType_None },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_Decelerate },
            { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_Accelerate },
            { .type = KeyActionType_None },
        }
    },
};
