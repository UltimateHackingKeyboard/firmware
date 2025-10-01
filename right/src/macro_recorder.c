#include <string.h>
#include "macro_recorder.h"
#include "event_scheduler.h"
#include "led_display.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "stubs.h"
#ifdef __ZEPHYR__
#include "keyboard/oled/widgets/widget_store.h"
#else
#include "segment_display.h"
#endif
#include "timer.h"

/**
 * Control bytes:
 * 0 + 7bits of a scancode = flip the scancode bit
 * 1100 + 4 bits = left mods
 * 1110 + 4 bits = right mods
 * 1111 0000 = empty
 * 1111 0001 = delay + 2 bytes uint16_t delay value
 * 1111 0010 = full report + 1 byte modifiers + 1 byte scancode count + count bytes of scancodes that are pressed
 */

bool RuntimeMacroPlaying = false;
bool RuntimeMacroRecording = false;
bool RuntimeMacroRecordingBlind = false;

// State variables for new serialization format
static usb_basic_keyboard_report_t MacroRecorder_RecordingState = {0};
static usb_basic_keyboard_report_t MacroRecorder_ReplayingState = {0};

static uint8_t reportBuffer[REPORT_BUFFER_MAX_LENGTH];
static uint16_t reportBufferEnd = 0;

static runtime_macro_header headers[MAX_RUNTIME_MACROS];
static uint16_t headersEnd = 0;

static runtime_macro_header *recordingHeader;
static runtime_macro_header *playbackHeader;
static uint16_t playbackPosition;

static bool delayActive;
static uint32_t delayStart;

static uint32_t ledFlashingPeriod = 500;

static void initHeaderSlot(uint16_t id)
{
    recordingHeader = &headers[headersEnd];
    headersEnd++;
    recordingHeader->id = id;
    recordingHeader->offset = reportBufferEnd;
    recordingHeader->length = 0;
}

static void discardMacro(uint8_t macroIndex)
{
    uint16_t offsetLeft = headers[macroIndex].offset;
    uint16_t offsetRight = headers[macroIndex].offset + headers[macroIndex].length;
    uint16_t length = headers[macroIndex].length;
    uint8_t *leftPtr = &reportBuffer[offsetLeft];
    uint8_t *rightPtr = &reportBuffer[offsetRight];
    memcpy(leftPtr, rightPtr, reportBufferEnd - offsetRight);
    reportBufferEnd = reportBufferEnd - length;
    memcpy(&headers[macroIndex], &headers[macroIndex+1], (headersEnd-macroIndex) * sizeof (*recordingHeader));
    headersEnd = headersEnd-1;
    for (int i = macroIndex; i < headersEnd; i++) {
        headers[i].offset -= length;
    }
    
    // Fix pointers if they were pointing to the discarded macro
    if (recordingHeader == &headers[macroIndex]) {
        recordingHeader = NULL; // Will be set in initHeaderSlot
    } else if (recordingHeader > &headers[macroIndex]) {
        recordingHeader--; // Adjust pointer for the shift
    }
    
    if (playbackHeader == &headers[macroIndex]) {
        playbackHeader = NULL; // Will be set in resolvePlaybackHeader
    } else if (playbackHeader > &headers[macroIndex]) {
        playbackHeader--; // Adjust pointer for the shift
    }
}

static void discardLastMacro()
{
    if (headersEnd > 0) {
        discardMacro(headersEnd - 1);
    }
}

static void resolveRecordingHeader(uint16_t id)
{
    for (int i = 0; i < headersEnd; i++)
    {
        if (headers[i].id == id)
        {
            discardMacro(i);
            break;
        }
    }
    while(headersEnd == MAX_RUNTIME_MACROS) {
        discardMacro(0);
    }
    initHeaderSlot(id);
}

static bool resolvePlaybackHeader(uint16_t id)
{
    for (int i = 0; i < headersEnd; i++)
    {
        if (headers[i].id == id)
        {
            if (&headers[i] == recordingHeader && RuntimeMacroRecording) {
                return false;
            }
            playbackHeader = &headers[i];
            return true;
        }
    }
    //Macros_ReportErrorNum("Macro slot not found ", id);
    return false;
}

//id is an arbitrary slot identifier
static void recordRuntimeMacroStart(uint16_t id, bool blind)
{
    resolveRecordingHeader(id);
    RuntimeMacroRecording = true;
    RuntimeMacroRecordingBlind = blind;
    // Initialize recording state
    memset(&MacroRecorder_RecordingState, 0, sizeof(MacroRecorder_RecordingState));
    MacroRecorder_UpdateRecordingLed();
}

static void writeByte(uint8_t b)
{
    reportBuffer[reportBufferEnd] = b;
    reportBufferEnd++;
    recordingHeader->length++;
}


static void writeUInt16(uint16_t b)
{
    writeByte(((uint8_t*)&b)[0]);
    writeByte(((uint8_t*)&b)[1]);
}

static void recordRuntimeMacroEnd()
{
    RuntimeMacroRecording = false;
    RuntimeMacroRecordingBlind = false;
    // Reset recording state
    memset(&MacroRecorder_RecordingState, 0, sizeof(MacroRecorder_RecordingState));
    MacroRecorder_UpdateRecordingLed();
}

static uint8_t readByte()
{
    return reportBuffer[playbackPosition++];
}

static uint16_t readUInt16()
{
    uint16_t b;
    ((uint8_t*)&b)[0] = reportBuffer[playbackPosition++];
    ((uint8_t*)&b)[1] = reportBuffer[playbackPosition++];
    return b;
}

// Deserialization functions
static void readEmptyReport(usb_basic_keyboard_report_t *report)
{
    memset(report, 0, sizeof *report);
    MacroRecorder_ReplayingState = *report;
}

static void readFullReport(usb_basic_keyboard_report_t *report)
{
    if (playbackPosition + 2 > playbackHeader->offset + playbackHeader->length) {
        return;
    }
    memset(report, 0, sizeof *report);
    report->modifiers = readByte();
    uint8_t scancodeCount = readByte();
    if (playbackPosition + scancodeCount > playbackHeader->offset + playbackHeader->length) {
        return;
    }
    for (int i = 0; i < scancodeCount; i++) {
        UsbBasicKeyboard_AddScancode(report, readByte());
    }
    MacroRecorder_ReplayingState = *report;
}

static void readScancodeFlip(uint8_t scancode)
{
    if (UsbBasicKeyboard_ContainsScancode(&MacroRecorder_ReplayingState, scancode)) {
        UsbBasicKeyboard_RemoveScancode(&MacroRecorder_ReplayingState, scancode);
    } else {
        UsbBasicKeyboard_AddScancode(&MacroRecorder_ReplayingState, scancode);
    }
}

static void readLeftModifiers(uint8_t modifiers)
{
    MacroRecorder_ReplayingState.modifiers = (MacroRecorder_ReplayingState.modifiers & 0xF0) | (modifiers & 0x0F);
}

static void readRightModifiers(uint8_t modifiers)
{
    MacroRecorder_ReplayingState.modifiers = (MacroRecorder_ReplayingState.modifiers & 0x0F) | ((modifiers & 0x0F) << 4);
}

static void readDelay(usb_basic_keyboard_report_t *report)
{
    if (playbackPosition + 2 > playbackHeader->offset + playbackHeader->length) {
        return;
    }
    uint16_t timeout = readUInt16();
    if (!delayActive) {
        delayActive = true;
        delayStart = Timer_GetCurrentTime();
        playbackPosition -= 3;
    } else {
        if (Timer_GetElapsedTime(&delayStart) < timeout) {
            playbackPosition -= 3;
        } else {
            delayActive = false;
        }
    }
    *report = MacroRecorder_ReplayingState;
}

static void playReport(usb_basic_keyboard_report_t *report)
{
    if (playbackPosition >= playbackHeader->offset + playbackHeader->length) {
        // End of macro reached
        return;
    }

    uint8_t controlByte = readByte();

    if (controlByte == MACRO_CTRL_EMPTY) {
        readEmptyReport(report);
    } else if (controlByte == MACRO_CTRL_DELAY) {
        readDelay(report);
    } else if (controlByte == MACRO_CTRL_FULL_REPORT) {
        readFullReport(report);
    } else if ((controlByte & 0xF0) == 0xC0) {
        // Left modifiers
        readLeftModifiers(controlByte);
        *report = MacroRecorder_ReplayingState;
    } else if ((controlByte & 0xF0) == 0xE0) {
        // Right modifiers
        readRightModifiers(controlByte);
        *report = MacroRecorder_ReplayingState;
    } else {
        // Scancode flip (0 + 7 bits)
        readScancodeFlip(controlByte);
        *report = MacroRecorder_ReplayingState;
    }
}

static bool playRuntimeMacroBegin(uint16_t id)
{
    if (!resolvePlaybackHeader(id)) {
        return false;
    }
    playbackPosition = playbackHeader->offset;
    RuntimeMacroPlaying = true;
    // Initialize replaying state
    memset(&MacroRecorder_ReplayingState, 0, sizeof(MacroRecorder_ReplayingState));
    return true;
}

static bool playRuntimeMacroContinue(usb_basic_keyboard_report_t* report)
{
    if (!RuntimeMacroPlaying) {
        return false;
    }
    playReport(report);
    RuntimeMacroPlaying = playbackPosition < playbackHeader->offset + playbackHeader->length;
    return RuntimeMacroPlaying;
}

void writeReportScancodes(usb_basic_keyboard_report_t *report)
{
    UsbBasicKeyboard_ForeachScancode(report, &writeByte);
}

// Helper function to make space in buffer
static bool makeSpaceIfNeeded(uint16_t expected_length)
{
    while (reportBufferEnd + expected_length > REPORT_BUFFER_MAX_LENGTH || 
           recordingHeader->length + expected_length > REPORT_BUFFER_MAX_MACRO_LENGTH) {
        if (headersEnd > 1) {
            discardMacro(0);
        } else {
            recordRuntimeMacroEnd();
            return false; // No space available, recording stopped
        }
    }
    return true; // Space available
}

// Macros
#define RETURN_IF_FULL(expected_length) \
    if (!makeSpaceIfNeeded(expected_length)) { \
        return; \
    }


// Serialization functions
static void writeEmptyReport(void)
{
    writeByte(MACRO_CTRL_EMPTY);
    memset(&MacroRecorder_RecordingState, 0, sizeof(MacroRecorder_RecordingState));
}

static void writeFullReport(usb_basic_keyboard_report_t *report)
{
    writeByte(MACRO_CTRL_FULL_REPORT);
    writeByte(report->modifiers);
    writeByte(UsbBasicKeyboard_ScancodeCount(report));
    writeReportScancodes(report);
    MacroRecorder_RecordingState = *report;
}

static void writeScancodeFlip(uint8_t diffResult)
{
    uint8_t scancode = diffResult & 0x7F;
    writeByte(scancode & 0x7F); // 0 + 7 bits of scancode
    if (UsbBasicKeyboard_ContainsScancode(&MacroRecorder_RecordingState, scancode)) {
        UsbBasicKeyboard_RemoveScancode(&MacroRecorder_RecordingState, scancode);
    } else {
        UsbBasicKeyboard_AddScancode(&MacroRecorder_RecordingState, scancode);
    }
}

static void writeLeftModifiers(uint8_t diffResult)
{
    uint8_t leftMods = diffResult & 0x0F;
    writeByte(MACRO_CTRL_LEFT_MODS | leftMods);
    MacroRecorder_RecordingState.modifiers = (MacroRecorder_RecordingState.modifiers & 0xF0) | leftMods;
}

static void writeRightModifiers(uint8_t diffResult)
{
    uint8_t rightMods = diffResult & 0x0F;
    writeByte(MACRO_CTRL_RIGHT_MODS | rightMods);
    MacroRecorder_RecordingState.modifiers = (MacroRecorder_RecordingState.modifiers & 0x0F) | (rightMods << 4);
}

static void writeDelay(uint16_t delay)
{
    writeByte(MACRO_CTRL_DELAY);
    writeUInt16(delay);
}


void MacroRecorder_RecordBasicReport(usb_basic_keyboard_report_t *report)
{
    if (!RuntimeMacroRecording) {
        return;
    }

    usb_basic_keyboard_report_t emptyReport = {0};

    // Check if this is the first report (recording header length is 0)
    if (recordingHeader->length == 0) {
        // First report - write full report
        RETURN_IF_FULL(3 + UsbBasicKeyboard_ScancodeCount(report));
        writeFullReport(report);
        return;
    }

    // Check if report is empty
    if (memcmp(report, &emptyReport, sizeof(usb_basic_keyboard_report_t)) == 0) {
        // Report is empty - check if we need to write empty control byte
        if (memcmp(&MacroRecorder_RecordingState, &emptyReport, sizeof(usb_basic_keyboard_report_t)) != 0) {
            // Recording state is not empty - write empty control byte
            RETURN_IF_FULL(1);
            writeEmptyReport();
        }
        return;
    }

    // Write incremental changes
    uint8_t diffResult;
    while (UsbBasicKeyboard_FindFirstDifference(report, &MacroRecorder_RecordingState, &diffResult)) {
        if (diffResult == 0xFF) {
            // Scancode out of range, fall back to full report
            RETURN_IF_FULL(3 + UsbBasicKeyboard_ScancodeCount(report));
            writeFullReport(report);
            return;
        }

        RETURN_IF_FULL(1);

        // Write the appropriate control byte based on difference type
        if ((diffResult & 0xF0) == 0xC0) {
            // Left modifiers
            writeLeftModifiers(diffResult);
        } else if ((diffResult & 0xF0) == 0xE0) {
            // Right modifiers
            writeRightModifiers(diffResult);
        } else {
            // Scancode flip
            writeScancodeFlip(diffResult);
        }
    }
}

void MacroRecorder_RecordDelay(uint16_t delay)
{
    if (!RuntimeMacroRecording) {
        return;
    }

    // Check if we have any recorded data yet (recording header length is 0)
    if (recordingHeader->length == 0) {
        // No data recorded yet - write empty report first, then delay
        RETURN_IF_FULL(4); // 1 byte empty + 1 byte control + 2 bytes delay
        writeEmptyReport();
    }

    RETURN_IF_FULL(3); // 1 byte control + 2 bytes delay
    writeDelay(delay);
}

bool MacroRecorder_PlayRuntimeMacroSmart(uint16_t id, usb_basic_keyboard_report_t* report)
{
    if (!RuntimeMacroPlaying) {
        if (!playRuntimeMacroBegin(id)) {
            return false;
        }
    }
    return playRuntimeMacroContinue(report);
}

void MacroRecorder_RecordRuntimeMacroSmart(uint16_t id, bool blind)
{
    if (RuntimeMacroPlaying) {
        return;
    }
    if (!RuntimeMacroRecording) {
        recordRuntimeMacroStart(id, blind);
    }
    else {
        recordRuntimeMacroEnd();
    }
}

void MacroRecorder_UpdateRecordingLed()
{
    static bool ledOn = false;

    if (!RuntimeMacroRecording) {
        ledOn = false;
        WIDGET_REFRESH(&StatusWidget);
        LedDisplay_SetIcon(LedDisplayIcon_Adaptive, false);
        EventScheduler_Unschedule(EventSchedulerEvent_MacroRecorderFlashing);
        return;
    }

    ledOn = !ledOn;
    WIDGET_REFRESH(&StatusWidget);
    LedDisplay_SetIcon(LedDisplayIcon_Adaptive, ledOn);
    EventScheduler_Schedule(Timer_GetCurrentTime() + ledFlashingPeriod, EventSchedulerEvent_MacroRecorderFlashing, "macro recorder flashing");
}

void MacroRecorder_StartRecording(uint16_t id, bool blind)
{
    if (RuntimeMacroPlaying) {
        return;
    }
    recordRuntimeMacroEnd();
    recordRuntimeMacroStart(id, blind);
}

void MacroRecorder_StopRecording()
{
    recordRuntimeMacroEnd();
}

bool MacroRecorder_IsRecording()
{
    return RuntimeMacroRecording;
}

uint16_t MacroRecorder_RecordingId()
{
    return recordingHeader->id;
}
