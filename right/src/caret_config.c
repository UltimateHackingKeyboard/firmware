#include "caret_config.h"
#include "arduino_hid/ConsumerAPI.h"
#include "arduino_hid/SystemAPI.h"
#include "macros.h"

caret_configuration_t caretMediaMode = {
    .axisActions = { //axis array
        { // horizontal axis
            .positiveAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_NEXT }},
            .negativeAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_PREVIOUS }},
        },
        { // vertical axis
            .positiveAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_VOLUME_UP }},
            .negativeAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_VOLUME_DOWN }},
         }
    }
};


caret_configuration_t caretCaretMode = {
    .axisActions = { //axis array
        { // horizontal axis
            .positiveAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Basic, .scancode = HID_KEYBOARD_SC_RIGHT_ARROW }},
            .negativeAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Basic, .scancode = HID_KEYBOARD_SC_LEFT_ARROW }},
        },
        { // vertical axis
            .positiveAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Basic, .scancode = HID_KEYBOARD_SC_UP_ARROW }},
            .negativeAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Basic, .scancode = HID_KEYBOARD_SC_DOWN_ARROW }},
        }
    }
};

caret_configuration_t zoomMacMode = {
    .axisActions = { //axis array
        { // horizontal axis
            .positiveAction = { .type = KeyActionType_None },
            .negativeAction = { .type = KeyActionType_None },
        },
        { // vertical axis
            .positiveAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Basic, .scancode = HID_KEYBOARD_SC_EQUAL_AND_PLUS, .modifiers = HID_KEYBOARD_MODIFIER_LEFTGUI | HID_KEYBOARD_MODIFIER_LEFTSHIFT}},
            .negativeAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Basic, .scancode = HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE, .modifiers = HID_KEYBOARD_MODIFIER_LEFTGUI}},
        }
    }
};

caret_configuration_t zoomPcMode = {
    .axisActions = { //axis array
        { // horizontal axis
            .positiveAction = { .type = KeyActionType_None },
            .negativeAction = { .type = KeyActionType_None },
        },
        { // vertical axis
            .positiveAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Basic, .scancode = HID_KEYBOARD_SC_EQUAL_AND_PLUS, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_LEFTSHIFT}},
            .negativeAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Basic, .scancode = HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE, .modifiers = HID_KEYBOARD_MODIFIER_LEFTCTRL}},
        }
    }
};

caret_configuration_t* GetModuleCaretConfiguration(int8_t moduleId, navigation_mode_t mode) {
    switch (mode) {
        case NavigationMode_Caret:
            return &caretCaretMode;
        case NavigationMode_Media:
            return &caretMediaMode;
        case NavigationMode_ZoomPc:
            return &zoomPcMode;
        case NavigationMode_ZoomMac:
            return &zoomMacMode;
        default:
            Macros_ReportErrorNum("Mode referenced in in valid context:", mode);
            return NULL;
    }
}


void SetModuleCaretConfiguration(navigation_mode_t mode, caret_axis_t axis, bool positive, key_action_t action)
{
    caret_configuration_t* config = &caretMediaMode;

    if(mode == NavigationMode_Caret) {
        config = &caretCaretMode;
    }

    key_action_t* actionSlot = positive ? &config->axisActions[axis].positiveAction : &config->axisActions[axis].negativeAction;

    *actionSlot = action;
}
