#ifndef SRC_MACRO_SHORTCUT_PARSER_H_
#define SRC_MACRO_SHORTCUT_PARSER_H_


// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "macros.h"

// Typedefs:

    // Functions:

    void ShortcutParser_initialize();

    bool MacroShortcutParser_Parse(const char* str, const char* strEnd, macro_sub_action_t type, macro_action_t* outMacroAction, key_action_t* outKeyAction);
    uint8_t MacroShortcutParser_CharacterToScancode(char character);
    bool MacroShortcutParser_CharacterToShift(char character);
    char MacroShortcutParser_ScancodeToCharacter(uint16_t scancode);


#endif /* SRC_MACRO_SHORTCUT_PARSER_H_ */
