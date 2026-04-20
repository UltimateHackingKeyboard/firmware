#include <string.h>
#include "test_switches.h"
#include "led_display.h"
#include "key_action.h"
#include "keymap.h"
#include "segment_display.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "ledmap.h"
#include "led_manager.h"
#include "event_scheduler.h"

#ifdef __ZEPHYR__
#include "state_sync.h"
#endif

bool TestSwitches = false;

static const key_definition_t TestKeymap[1][2][MAX_KEY_COUNT_PER_MODULE] = {
    // Base layer
    {
        // Right keyboard half
        {
            // Row 1
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_7_AND_AMPERSAND } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_8_AND_ASTERISK } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_EQUAL_AND_PLUS } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSPACE } }},

            // Row 2
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_U } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_I } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_O } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_P } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSLASH_AND_PIPE } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Y } }},

            // Row 3
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_J } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_K } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_L } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SEMICOLON_AND_COLON } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_PLUS } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_H } }},

            // Row 4
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_N } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_M } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_INTERNATIONAL1 } }},
            { .action = { .type = KeyActionType_None }},

            // Row 5
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_6_AND_RIGHT_ARROW } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_ASTERISK } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_7_AND_HOME } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_9_AND_PAGE_UP } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_0_AND_INSERT } }},
        },

        // Left keyboard half
        {
            // Row 1
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_1_AND_EXCLAMATION } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_2_AND_AT } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_3_AND_HASHMARK } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_4_AND_DOLLAR } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_5_AND_PERCENTAGE } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_6_AND_CARET } }},

            // Row 2
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ESCAPE } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Q } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_W } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_E } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_R } }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_T } }},

            // Row 3
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_MINUS } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_A } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_S } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_D } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F } }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_G } }},

            // Row 4
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_INTERNATIONAL4 } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Z } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_X } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_C } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_V } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_B } }},

            // Row 5
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_1_AND_END } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_3_AND_PAGE_DOWN } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_4_AND_LEFT_ARROW } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_SLASH } }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_KEYPAD_5 } }},
            { .action = { .type = KeyActionType_None }},
        }
    }
};

void TestSwitches_Activate(void)
{
    TestSwitches = true;
    memcpy(&CurrentKeymap, &TestKeymap, sizeof TestKeymap);
    SegmentDisplay_SetText(3, "TES", SegmentDisplaySlot_Keymap);

#ifndef __ZEPHYR__
    LedSlaveDriver_EnableAllLeds();
#endif

    Ledmap_ActivateTestLedMode(true);

#if DEVICE_IS_UHK80_RIGHT
    StateSync_UpdateProperty(StateSyncPropertyId_SwitchTestMode, NULL);
#endif
}

void TestSwitches_Deactivate(void)
{
    TestSwitches = false;
    Ledmap_ActivateTestLedMode(false);

#if DEVICE_IS_UHK80_RIGHT
    StateSync_UpdateProperty(StateSyncPropertyId_SwitchTestMode, NULL);
#endif

#if DEVICE_IS_MASTER
    EventVector_Set(EventVector_KeymapReloadNeeded);
#endif
}
