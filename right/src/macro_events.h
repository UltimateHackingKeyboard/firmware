#ifndef __MACRO_EVENTS_H__
#define __MACRO_EVENTS_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "key_action.h"
    #include "usb_device_config.h"
    #include "key_states.h"
    #include "layer.h"

// Macros:


// Typedefs:

    typedef enum {
        GenericMacroEvent_OnError = 0,
        GenericMacroEvent_Count,
    } generic_macro_event_t;

// Variables:

    extern bool MacroEvent_CapsLockStateChanged;
    extern bool MacroEvent_NumLockStateChanged;
    extern bool MacroEvent_ScrollLockStateChanged;

// Functions:

    void MacroEvent_OnInit();
    void MacroEvent_OnKeymapChange(uint8_t keymapIdx);
    void MacroEvent_OnLayerChange(layer_id_t layerId);
    void MacroEvent_OnError();
    void MacroEvent_RegisterLayerMacros();
    void MacroEvent_ProcessStateKeyEvents();
    void MacroEvent_TriggerGenericEvent(generic_macro_event_t eventId);

#endif
