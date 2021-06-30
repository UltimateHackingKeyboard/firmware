#include "caret_config.h"
#include "arduino_hid/ConsumerAPI.h"
#include "arduino_hid/SystemAPI.h"

caret_configuration_t caretMediaMode = {
    .axisActions = { //axis array
        { // horizontal axis
            .positiveAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_NEXT }},
            .negativeAction = { .type = KeyActionType_Keystroke, .keystroke = { .keystrokeType = KeystrokeType_Media, .scancode = MEDIA_PLAY_PAUSE }},
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

caret_configuration_t* GetModuleCaretConfiguration(int8_t moduleId, navigation_mode_t mode) {
    switch (mode) {
        case NavigationMode_Caret:
            return &caretCaretMode;
        case NavigationMode_Media:
            return &caretMediaMode;
        default:
            return &caretCaretMode;
    }
}

