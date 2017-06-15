#ifndef __PARSE_KEYMAP_H__
#define __PARSE_KEYMAP_H__

// Includes:

    #include <stdint.h>

// Typedefs:

    typedef enum {
        SerializedKeystrokeType_Basic,
        SerializedKeystrokeType_ShortMedia,
        SerializedKeystrokeType_LongMedia,
        SerializedKeystrokeType_System,
    } serialized_keystroke_type_t;

    typedef enum {
        SerializedKeyActionType_None = 0,
        SerializedKeyActionType_KeyStroke = 1,
        SerializedKeyActionType_LastKeyStroke = 31,
        SerializedKeyActionType_SwitchLayer,
        SerializedKeyActionType_SwitchKeymap,
        SerializedKeyActionType_Mouse,
        SerializedKeyActionType_PlayMacro
    } serialized_key_action_type_t;

    typedef struct {
        const uint8_t *buffer;
        uint16_t offset;
    } serialized_buffer_t;

// Functions:

    void ParseLayer(const uint8_t *data, uint8_t targetLayer);

#endif
