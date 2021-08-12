#include "str_utils.h"
#include "macro_events.h"
#include "config_parser/parse_macro.h"
#include "macros.h"
#include "keymap.h"
#include "led_display.h"
#include "debug.h"

/**
 * Future possible extensions:
 * - generalize change to always handle "in" and "out" events
 * - add onKeymapLayerChange, onKeymapKeyPress, onKeyPress, onLayerChange events. These would be indexed when keymap is changed and kept at hand, so we could run them without any performance impact.
 */

void MacroEvent_OnInit()
{
    const char* s = "$onInit";
    uint8_t idx = FindMacroIndexByName(s, s + strlen(s), false);
    if (idx != 255) {
        Macros_StartMacro(idx, NULL, 255);
    }
}

void MacroEvent_OnKeymapChange(uint8_t keymapIdx)
{
    keymap_reference_t *keymap = AllKeymaps + keymapIdx;
    const char* curAbbrev = keymap->abbreviation;
    const char* curAbbrevEnd = keymap->abbreviation + keymap->abbreviationLen;

    for (int i = 0; i < AllMacrosCount; i++) {
        const char *thisName, *thisNameEnd;
        FindMacroName(&AllMacros[i], &thisName, &thisNameEnd);

        if (TokenMatches(thisName, thisNameEnd, "$onKeymapChange")) {
            const char* macroArg = NextTok(thisName,thisNameEnd);

            if (TokenMatches2(macroArg, thisNameEnd, curAbbrev, curAbbrevEnd)) {
                Macros_StartMacro(i, NULL, 255);
            }
        }
    }
}
