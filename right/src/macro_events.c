#include "layer.h"
#include "string.h"
#include "str_utils.h"
#include "macro_events.h"
#include "config_parser/parse_macro.h"
#include "macros/core.h"
#include "keymap.h"
#include "led_display.h"
#include "debug.h"
#include "event_scheduler.h"


static macro_index_t anyLayerChangeMacro = MacroIndex_None;
static macro_index_t layerChangeMacro[LayerId_Count];
static macro_index_t keymapLayerChangeMacro[LayerId_Count];
static macro_index_t capsLockChangeMacro = MacroIndex_None;
static macro_index_t scrollLockChangeMacro = MacroIndex_None;
static macro_index_t numLockChangeMacro = MacroIndex_None;

bool MacroEvent_CapsLockStateChanged = false;
bool MacroEvent_NumLockStateChanged = false;
bool MacroEvent_ScrollLockStateChanged = false;

typedef struct {
    const char* name;
    uint8_t macroIndex;
} ATTR_PACKED generic_macro_event_metadata_t;

static generic_macro_event_metadata_t genericMacroEvents[GenericMacroEvent_Count] = {
    [GenericMacroEvent_OnError] = { .name="$onError", .macroIndex=MacroIndex_None},
};

/**
 * Future possible extensions:
 * - generalize change to always handle "in" and "out" events
 * - add onKeymapLayerChange, onKeymapKeyPress, onKeyPress, onLayerChange events. These would be indexed when keymap is changed and kept at hand, so we could run them without any performance impact.
 */


static void registerKeyboardStates();
static void registerGenericEvents();

/**
 * Macro events should be executed in order and wait for each other - first onInit, then `onKmeymapChange any`, finally other `onKeymapChange` ones.
 */
static uint8_t previousEventMacroSlot = 255;

void MacroEvent_OnInit()
{
    const char* name = "$onInit";
    uint8_t idx = FindMacroIndexByName(name, name + strlen(name), false);
    if (idx != 255) {
        previousEventMacroSlot = Macros_StartMacro(idx, NULL, 255, false);
    }

    registerKeyboardStates();
    registerGenericEvents();
}

static void startMacroInSlot(macro_index_t macroIndex, uint8_t* slotId) {
    if (macroIndex != MacroIndex_None) {
        if (*slotId != 255 && MacroState[*slotId].ms.macroPlaying) {
            *slotId = Macros_QueueMacro(macroIndex, NULL, *slotId);
        } else {
            *slotId = Macros_StartMacro(macroIndex, NULL, 255, false);
        }
    }
}

static void processOnKeymapChange(const char* curAbbrev, const char* curAbbrevEnd)
{
    for (int i = 0; i < AllMacrosCount; i++) {
        const char *thisName, *thisNameEnd;
        FindMacroName(&AllMacros[i], &thisName, &thisNameEnd);

        if (TokenMatches(thisName, thisNameEnd, "$onKeymapChange")) {
            const char* macroArg = NextTok(thisName,thisNameEnd);

            if (TokenMatches2(macroArg, thisNameEnd, curAbbrev, curAbbrevEnd)) {
                startMacroInSlot(i, &previousEventMacroSlot);
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
}

void MacroEvent_RegisterLayerMacros()
{
    keymap_reference_t *keymap = AllKeymaps + CurrentKeymapIndex;
    const char* curAbbrev = keymap->abbreviation;
    const char* curAbbrevEnd = keymap->abbreviation + keymap->abbreviationLen;

    anyLayerChangeMacro = MacroIndex_None;
    memset(layerChangeMacro, MacroIndex_None, sizeof layerChangeMacro);
    memset(keymapLayerChangeMacro, MacroIndex_None, sizeof layerChangeMacro);

    bool oldParserStatus = Macros_ParserError;
    Macros_ParserError = false;

    for (int i = 0; i < AllMacrosCount; i++) {
        const char *thisName, *thisNameEnd;
        FindMacroName(&AllMacros[i], &thisName, &thisNameEnd);

        if (TokenMatches(thisName, thisNameEnd, "$onKeymapLayerChange")) {
            const char* macroArg = NextTok(thisName,thisNameEnd);
            const char* macroArg2 = NextTok(macroArg,thisNameEnd);
            const layer_id_t layerId = Macros_ConsumeLayerId(&(parser_context_t){.at = macroArg2, .end = thisNameEnd});

            if (TokenMatches2(macroArg, thisNameEnd, curAbbrev, curAbbrevEnd) && !Macros_ParserError) {
                keymapLayerChangeMacro[layerId] = i;
            }
        }

        if (TokenMatches(thisName, thisNameEnd, "$onLayerChange")) {
            const char* macroArg = NextTok(thisName,thisNameEnd);
            if (TokenMatches(macroArg, thisNameEnd, "any") && !Macros_ParserError) {
                anyLayerChangeMacro = i;
            } else {
                const layer_id_t layerId = Macros_ConsumeLayerId(&(parser_context_t){.at = macroArg, .end = thisNameEnd});
                if (!Macros_ParserError) {
                    layerChangeMacro[layerId] = i;
                }
            }
        }
        Macros_ParserError = false;
    }

    Macros_ParserError = oldParserStatus;
}

void MacroEvent_OnLayerChange(layer_id_t layerId)
{
    macro_index_t macrosToTry[3] = {
        anyLayerChangeMacro,
        layerChangeMacro[layerId],
        keymapLayerChangeMacro[layerId],
    };

    for (uint8_t i = 0; i < sizeof macrosToTry; i++) {
        startMacroInSlot(macrosToTry[i], &previousEventMacroSlot);
    }

    previousEventMacroSlot = 255;
}

static void registerKeyboardStates()
{
    for (int i = 0; i < AllMacrosCount; i++) {
        const char *thisName, *thisNameEnd;
        FindMacroName(&AllMacros[i], &thisName, &thisNameEnd);

        if (TokenMatches(thisName, thisNameEnd, "$onCapsLockStateChange")) {
            capsLockChangeMacro = i;
        }
        if (TokenMatches(thisName, thisNameEnd, "$onNumLockStateChange")) {
            numLockChangeMacro = i;
        }
        if (TokenMatches(thisName, thisNameEnd, "$onScrollLockStateChange")) {
            scrollLockChangeMacro = i;
        }
    }
}

static void registerGenericEvents()
{
    for (int i = 0; i < AllMacrosCount; i++) {
        const char *thisName, *thisNameEnd;
        FindMacroName(&AllMacros[i], &thisName, &thisNameEnd);

        for (uint8_t genericEventId = 0; genericEventId < GenericMacroEvent_Count; genericEventId++) {
            if (TokenMatches(thisName, thisNameEnd, genericMacroEvents[genericEventId].name)) {
                genericMacroEvents[genericEventId].macroIndex = i;
            }
        }
    }
}

void MacroEvent_TriggerGenericEvent(generic_macro_event_t eventId)
{
    uint8_t macroIndex = genericMacroEvents[eventId].macroIndex;
    if (macroIndex != MacroIndex_None) {
        startMacroInSlot(macroIndex, &previousEventMacroSlot);
    }
    previousEventMacroSlot = 255;
}

void MacroEvent_OnError() {
    static uint32_t last = 0;

    if (CurrentTime - last > 1000) {
        last = CurrentTime;
        MacroEvent_TriggerGenericEvent(GenericMacroEvent_OnError);
    }
}

void MacroEvent_ProcessStateKeyEvents()
{
    EventVector_Unset(EventVector_KeyboardLedState);

    if (MacroEvent_CapsLockStateChanged && capsLockChangeMacro != MacroIndex_None) {
        MacroEvent_CapsLockStateChanged = false;
        startMacroInSlot(capsLockChangeMacro, &previousEventMacroSlot);
    }
    if (MacroEvent_NumLockStateChanged && numLockChangeMacro != MacroIndex_None) {
        MacroEvent_NumLockStateChanged = false;
        startMacroInSlot(numLockChangeMacro, &previousEventMacroSlot);
    }
    if (MacroEvent_ScrollLockStateChanged && scrollLockChangeMacro != MacroIndex_None) {
        MacroEvent_ScrollLockStateChanged = false;
        startMacroInSlot(scrollLockChangeMacro, &previousEventMacroSlot);
    }
    previousEventMacroSlot = 255;
}
