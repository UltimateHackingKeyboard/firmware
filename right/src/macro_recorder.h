#ifndef __MACRO_RECORDER_H__
#define __MACRO_RECORDER_H__


// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "key_action.h"
    #include "usb_device_config.h"
    #include "key_states.h"

// Macros:

    #define MAX_RUNTIME_MACROS 32
    #define REPORT_BUFFER_MAX_LENGTH 2048
    #define REPORT_BUFFER_MAX_MACRO_LENGTH (REPORT_BUFFER_MAX_LENGTH/4)
    #define REPORT_BUFFER_MIN_GAP (REPORT_BUFFER_MAX_LENGTH/4)
    #define REPORT_BUFFER_SAFETY_MARGIN 11

// Typedefs:

    typedef enum {
        BasicKeyboardEmpty,
        BasicKeyboardSimple,
        BasicKeyboard,
        Delay
    } macro_report_type_t;

    typedef struct {
        uint16_t id;
        uint16_t offset;
        uint16_t length;
    } runtime_macro_header;

// Variables:

    extern bool RuntimeMacroPlaying;
    extern bool RuntimeMacroRecordingBlind;

// Functions:

    void MacroRecorder_RecordBasicReport(usb_basic_keyboard_report_t *report);
    void MacroRecorder_RecordDelay(uint16_t delay);

    bool MacroRecorder_PlayRuntimeMacroSmart(uint16_t id, usb_basic_keyboard_report_t *report);
    void MacroRecorder_RecordRuntimeMacroSmart(uint16_t id, bool blind);
    void MacroRecorder_StartRecording(uint16_t id, bool blind);
    void MacroRecorder_StopRecording();
    bool MacroRecorder_IsRecording();
    uint16_t MacroRecorder_RecordingId();

#endif
