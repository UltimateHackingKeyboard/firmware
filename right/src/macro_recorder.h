#ifndef __MACRO_RECORDER_H__
#define __MACRO_RECORDER_H__


// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "key_action.h"
    #include "key_states.h"
    #include "hid/keyboard_report.h"

// Macros:

    #define MAX_RUNTIME_MACROS 32
    #define REPORT_BUFFER_MAX_LENGTH 2048
    #define REPORT_BUFFER_MAX_MACRO_LENGTH (REPORT_BUFFER_MAX_LENGTH/2)
    #define REPORT_BUFFER_SAFETY_MARGIN 11

// Typedefs:


    // New serialization format control bytes
    #define MACRO_CTRL_SCANCODE_FLIP    0x00  // 0 + 7bits of a scancode = flip the scancode bit
    #define MACRO_CTRL_LEFT_MODS        0xC0  // 1100 + 4 bits = left mods
    #define MACRO_CTRL_RIGHT_MODS       0xE0  // 1110 + 4 bits = right mods
    #define MACRO_CTRL_EMPTY            0xF0  // 1111 0000 = empty
    #define MACRO_CTRL_DELAY            0xF1  // 1111 0001 = delay + 2 bytes uint16_t number
    #define MACRO_CTRL_FULL_REPORT      0xF2  // 1111 0010 = full report + 1 byte mods + 1 byte count + count bytes of scancodes that are pressed

    typedef struct {
        uint16_t id;
        uint16_t offset;
        uint16_t length;
    } runtime_macro_header;

// Variables:

    extern bool RuntimeMacroPlaying;
    extern bool RuntimeMacroRecordingBlind;

// Functions:

    void MacroRecorder_RecordBasicReport(hid_keyboard_report_t *report);
    void MacroRecorder_RecordDelay(uint16_t delay);

    void MacroRecorder_UpdateRecordingLed();
    bool MacroRecorder_PlayRuntimeMacroSmart(uint16_t id, hid_keyboard_report_t *report);
    void MacroRecorder_RecordRuntimeMacroSmart(uint16_t id, bool blind);
    void MacroRecorder_StartRecording(uint16_t id, bool blind);
    void MacroRecorder_StopRecording();
    bool MacroRecorder_IsRecording();
    uint16_t MacroRecorder_RecordingId();

#endif
