#ifndef SRC_MACRO_SHORTCUT_PARSER_H_
#define SRC_MACRO_SHORTCUT_PARSER_H_


// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Typedefs:

    typedef enum {
        scType_basic,
        scType_system,
        scType_media,
        scType_mouseBtn
    } shortcut_type_t;

    typedef struct {
        const char* id;
        uint8_t scancode;
        shortcut_type_t type;
    } lookup_record_t;

    // Functions:

    macro_action_t MacroShortcutParser_Parse(const char* str, const char* strEnd);
    uint8_t MacroShortcutParser_CharacterToScancode(char character);
    bool MacroShortcutParser_CharacterToShift(char character);
    char MacroShortcutParser_ScancodeToCharacter(uint16_t scancode);


#endif /* SRC_MACRO_SHORTCUT_PARSER_H_ */
