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

/**
 * Macro events should be executed in order and wait for each other - first onInit, then `onKmeymapChange any`, finally other `onKeymapChange` ones.
 */
static uint8_t previousEventMacroSlot = 255;

void MacroEvent_OnInit()
{
    const char* s = "$onInit";
    uint8_t idx = FindMacroIndexByName(s, s + strlen(s), false);
    if (idx != 255) {
        previousEventMacroSlot = Macros_StartMacro(idx, NULL, 255, false);
    }
}

void processOnKeymapChange(const char* curAbbrev, const char* curAbbrevEnd)
{
    for (int i = 0; i < AllMacrosCount; i++) {
        const char *thisName, *thisNameEnd;
        FindMacroName(&AllMacros[i], &thisName, &thisNameEnd);

        if (TokenMatches(thisName, thisNameEnd, "$onKeymapChange")) {
            const char* macroArg = NextTok(thisName,thisNameEnd);

            if (TokenMatches2(macroArg, thisNameEnd, curAbbrev, curAbbrevEnd)) {
                if (previousEventMacroSlot != 255 && MacroState[previousEventMacroSlot].ms.macroPlaying) {
                    previousEventMacroSlot = Macros_QueueMacro(i, NULL, previousEventMacroSlot);
                } else {
                    previousEventMacroSlot = Macros_StartMacro(i, NULL, 255, false);
                }
            }
        }
    }

}


void MacroEvent_OnKeymapChange(uint8_t keymapIdx)
{
    keymap_reference_t *keymap = AllKeymaps + keymapIdx;
    const char* curAbbrev = keymap->abbreviation;
    const char* curAbbrevEnd = keymap->abbreviation + keymap->abbreviationLen;

    const char* any = "any";

    processOnKeymapChange(any, any + strlen(any));
    processOnKeymapChange(curAbbrev, curAbbrevEnd);

    previousEventMacroSlot = 255;
}
