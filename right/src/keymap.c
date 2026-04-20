#include <string.h>
#include "arduino_hid/ConsumerAPI.h"
#include "arduino_hid/SystemAPI.h"
#include "keymap.h"
#include "layer.h"
#include "layer_switcher.h"
#include "led_display.h"
#include "ledmap.h"
#include "config_parser/parse_keymap.h"
#include "config_parser/config_globals.h"
#include "macros/core.h"
#include "macro_events.h"
#include "segment_display.h"
#include "layer_stack.h"
#include "debug.h"
#include "slave_drivers/uhk_module_driver.h"
#include "macros/status_buffer.h"

#ifdef __ZEPHYR__
#include "keyboard/oled/widgets/widget_store.h"
#include "keyboard/oled/widgets/text_widget.h"
#include "device.h"
#endif

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

void SwitchKeymapById(uint8_t index, bool resetLayerStack)
{
    if (!ValidatedUserConfigBuffer.isValid) {
        EventVector_Unset(EventVector_KeymapReloadNeeded);
        return;
    }
    parse_config_t parseConfig = (parse_config_t) {
        .mode = ParserRunDry ? ParseKeymapMode_DryRun : ParseKeymapMode_FullRun
    };
    CurrentKeymapIndex = index;
    config_buffer_t buffer = ValidatedUserConfigBuffer;
    buffer.offset = AllKeymaps[index].offset;
    ParseKeymap(&buffer, index, AllKeymapsCount, AllMacrosCount, parseConfig);
#if DEVICE_HAS_OLED
    Widget_Refresh(&KeymapWidget);
    Widget_Refresh(&KeymapLayerWidget);
#endif
    SegmentDisplay_UpdateKeymapText();
    if (DEVICE_IS_MASTER) {
        MacroEvent_RegisterLayerMacros();
        MacroEvent_OnKeymapChange(index);
        MacroEvent_OnLayerChange(ActiveLayer);
    }
    if (resetLayerStack) {
        LayerStack_Reset();
    }
    EventVector_Set(EventVector_LedMapUpdateNeeded);
    EventVector_Unset(EventVector_KeymapReloadNeeded);
}

uint8_t FindKeymapByAbbreviation(uint8_t length, const char *abbrev) {
    for (uint8_t i=0; i<AllKeymapsCount; i++) {
        keymap_reference_t *keymap = AllKeymaps + i;
        if (keymap->abbreviationLen == length && memcmp(keymap->abbreviation, abbrev, length) == 0) {
            return i;
        }
    }
    return 0xFF;
}

bool SwitchKeymapByAbbreviation(uint8_t length, const char *abbrev, bool resetLayerStack)
{
    uint8_t keymapId = FindKeymapByAbbreviation(length, abbrev);

    if (keymapId != 0xFF) {
        SwitchKeymapById(keymapId, resetLayerStack);
        return true;
    } else {
        return false;
    }
}

void OverlayKeymap(uint8_t srcKeymap)
{
    config_buffer_t buffer = ValidatedUserConfigBuffer;
    parse_config_t parseConfig = (parse_config_t) {
        .mode = ParseKeymapMode_OverlayKeymap,
    };
    buffer.offset = AllKeymaps[srcKeymap].offset;
    ParseKeymap(&buffer, srcKeymap, AllKeymapsCount, AllMacrosCount, parseConfig);
    EventVector_Set(EventVector_LedMapUpdateNeeded);
}


void DryParseKeymap(uint8_t srcKeymap)
{
    config_buffer_t buffer = ValidatedUserConfigBuffer;
    parse_config_t parseConfig = (parse_config_t) {
        .mode = ParseKeymapMode_DryRun,
    };
    buffer.offset = AllKeymaps[srcKeymap].offset;
    ParseKeymap(&buffer, srcKeymap, AllKeymapsCount, AllMacrosCount, parseConfig);
}

void OverlayLayer(layer_id_t dstLayer, uint8_t srcKeymap, layer_id_t srcLayer)
{
    config_buffer_t buffer = ValidatedUserConfigBuffer;
    parse_config_t parseConfig = (parse_config_t) {
        .mode = ParseKeymapMode_OverlayLayer,
        .srcLayer = srcLayer,
        .dstLayer = dstLayer,
    };
    buffer.offset = AllKeymaps[srcKeymap].offset;
    ParseKeymap(&buffer, srcKeymap, AllKeymapsCount, AllMacrosCount, parseConfig);
    EventVector_Set(EventVector_LedMapUpdateNeeded);
}

void ReplaceLayer(layer_id_t dstLayer, uint8_t srcKeymap, layer_id_t srcLayer)
{
    config_buffer_t buffer = ValidatedUserConfigBuffer;
    parse_config_t parseConfig = (parse_config_t) {
        .mode = ParseKeymapMode_ReplaceLayer,
        .srcLayer = srcLayer,
        .dstLayer = dstLayer,
    };
    buffer.offset = AllKeymaps[srcKeymap].offset;
    ParseKeymap(&buffer, srcKeymap, AllKeymapsCount, AllMacrosCount, parseConfig);
    EventVector_Set(EventVector_LedMapUpdateNeeded);
}

void ReplaceKeymap(uint8_t srcKeymap)
{
    config_buffer_t buffer = ValidatedUserConfigBuffer;
    parse_config_t parseConfig = (parse_config_t) {
        .mode = ParseKeymapMode_ReplaceKeymap,
    };
    buffer.offset = AllKeymaps[srcKeymap].offset;
    ParseKeymap(&buffer, srcKeymap, AllKeymapsCount, AllMacrosCount, parseConfig);
    EventVector_Set(EventVector_LedMapUpdateNeeded);
}

string_segment_t GetKeymapName(uint8_t keymapId)
{
    const char* name;
    uint16_t len;
    config_buffer_t buffer = ValidatedUserConfigBuffer;
    buffer.offset = AllKeymaps[keymapId].offset;
    ParseKeymapName(&buffer, &name, &len);
    return (string_segment_t){ .start = name, .end = name + len };
}

// The factory keymap is initialized before it gets overwritten by the default keymap of the EEPROM.
#ifndef __ZEPHYR__
ATTR_DATA2
#endif
key_definition_t CurrentKeymap[LayerId_Count][SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
    // Base layer
    {
        // Right keyboard half
        {
            // Row 1
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_7_AND_AMPERSAND }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_8_AND_ASTERISK }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_EQUAL_AND_PLUS }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSPACE }}},

            // Row 2
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Y }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_U }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_I }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_O }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_P }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSLASH_AND_PIPE }}},

            // Row 3
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_H }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_J }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_K }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_L }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SEMICOLON_AND_COLON }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ENTER }}},

            // Row 4
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_N }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_M }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }}},

            // Row 5
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SPACE }}},
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn , .mode = SwitchLayerMode_Hold }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }}},
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mod, .mode = SwitchLayerMode_Hold }}},
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn2, .mode = SwitchLayerMode_HoldAndDoubleTapToggle }}},

#if DEVICE_IS_UHK80
            // Row 0
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F7 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F8 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F9 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F10 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F11 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F12 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PRINT_SCREEN }}},

            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SCROLL_LOCK }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAUSE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_INSERT }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_HOME }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_UP }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_END }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_DOWN }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ARROW, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_UP_ARROW}}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ARROW, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ARROW}}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DOWN_ARROW}}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ARROW}}},
#endif

        },

        // Left keyboard half
        {
            // Row 1
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_1_AND_EXCLAMATION }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_2_AND_AT }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_3_AND_HASHMARK }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_4_AND_DOLLAR }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_5_AND_PERCENTAGE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_6_AND_CARET }}},

            // Row 2
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_TAB }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Q }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_W }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_E }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_R }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_T }}},

            // Row 3
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mouse, .mode = SwitchLayerMode_HoldAndDoubleTapToggle }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_A }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_S }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_D }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_G }}},

            // Row 4
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_NON_US_BACKSLASH_AND_PIPE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_Z }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_X }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_C }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_V }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_B }}},

            // Row 5
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }}},
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn, .mode = SwitchLayerMode_Hold }}},
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mod, .mode = SwitchLayerMode_Hold }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SPACE }}},
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn2, .mode = SwitchLayerMode_HoldAndDoubleTapToggle }}},

#if DEVICE_IS_UHK80
            // Row 0
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ESCAPE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F1 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F2 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F3 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F4 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F5 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F6 }}},
#endif
        },

        // Left module
        {
            // Row 1
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSPACE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ENTER }}},
        },

        // Right module
        {
            // Row 1
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_LeftClick }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_RightClick }},
        }
    },

    // Mod layer
    {
        // Right keyboard half
        {
            // Row 1
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F7 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F8 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F9 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F10 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F11 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F12 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }}},

            // Row 2
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_UP }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_HOME }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_UP_ARROW }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_END }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PRINT_SCREEN }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_SCROLL_LOCK }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAUSE }}},

            // Row 3
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_DOWN }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ARROW }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DOWN_ARROW }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ARROW }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_INSERT }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 4
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ESCAPE }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_APPLICATION }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }}},

            // Row 5
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }}},
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mod, .mode = SwitchLayerMode_Hold }}},
        },

        // Left keyboard half
        {
            // Row 1
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ESCAPE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F1 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F2 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F3 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F4 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F5 }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_F6 }}},

            // Row 2
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ESCAPE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_UP, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}}, // [<] tab prev
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_T, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}}, // [+] tab new
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_PAGE_DOWN, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}}, // [>] tab next
            { .action = { .type = KeyActionType_None }},

            // Row 3
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_CAPS_LOCK }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ARROW, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_LEFTALT }}}, // workspace prev
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_TAB, .modifiers = HID_KEYBOARD_MODIFIER_LEFTALT }}}, // window switch
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ARROW, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_LEFTALT }}}, // workspace next
            { .action = { .type = KeyActionType_None }},

            // Row 4
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_W, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL }}}, // [x] tab close
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 5
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mod, .mode = SwitchLayerMode_Hold }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
        },

        // Left module
        {
            // Row 1
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSPACE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ENTER }}},
        },

        // Right module
        {
            // Row 1
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_LeftClick }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_RightClick }},
        }
    },

    // Fn layer
    {
        // Right keyboard half
        {
            // Row 1
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_System, .scancode = SYSTEM_WAKE_UP }}},
            { .action = { .type = KeyActionType_None }},

            // Row 2
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_PLAY_PAUSE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_VOLUME_UP }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_STOP }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_System, .scancode = SYSTEM_SLEEP }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_System, .scancode = SYSTEM_POWER_DOWN }}},

            // Row 3
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_PREVIOUS }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_VOLUME_DOWN }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_NEXT }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 4
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_VOLUME_MUTE }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }}},

            // Row 5
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn, .mode = SwitchLayerMode_Hold }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }}},
            { .action = { .type = KeyActionType_None }},
        },

        // Left keyboard half
        {
            // Row 1
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 2
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = HID_CONSUMER_AC_CANCEL }}}, // HID_CONSUMER_AC_STOP
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_BROWSER_REFRESH }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 3
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_BROWSER_BACK }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_EXPLORER }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_BROWSER_FORWARD }}},
            { .action = { .type = KeyActionType_None }},

            // Row 4
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_SCREENSAVER }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = HID_CONSUMER_AC_SEARCH }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = CONSUMER_CALCULATOR }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = HID_CONSUMER_EJECT }}},
            { .action = { .type = KeyActionType_None }},

            // Row 5
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }}},
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Fn, .mode = SwitchLayerMode_Hold }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
        },

        // Left module
        {
            // Row 1
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSPACE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ENTER }}},
        },

        // Right module
        {
            // Row 1
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_LeftClick }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_RightClick }},
        }
    },

    // Mouse layer
    {
        // Right keyboard half
        {
            // Row 1
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 2
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_ScrollUp }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_Button_4 }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_MoveUp }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_Button_5 }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_Button_6 }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 3
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_ScrollDown }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction= SerializedMouseAction_MoveLeft }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_MoveDown }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_MoveRight }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 4
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_SHIFT }}},

            // Row 5
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_ALT }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_GUI }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_RIGHT_CONTROL }}},
            { .action = { .type = KeyActionType_None }},
        },

        // Left keyboard half
        {
            // Row 1
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 2
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 3
            { .action = { .type = KeyActionType_SwitchLayer, .switchLayer = { .layer = LayerId_Mouse, .mode = SwitchLayerMode_HoldAndDoubleTapToggle }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_RightClick }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_MiddleClick }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_LeftClick }},
            { .action = { .type = KeyActionType_None }},

            // Row 4
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_SHIFT }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_None }},

            // Row 5
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_CONTROL }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_GUI }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_LEFT_ALT }}},
            { .action = { .type = KeyActionType_None }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_Accelerate }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_Decelerate }},
            { .action = { .type = KeyActionType_None }},
        },

        // Left module
        {
            // Row 1
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSPACE }}},
            { .action = { .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ENTER }}},
        },

        // Right module
        {
            // Row 1
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_LeftClick }},
            { .action = { .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_RightClick }},
        }

    },
};

