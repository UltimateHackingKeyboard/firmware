#include "macro_recorder.h"
#include "led_display.h"
#include "macros.h"
#include "segment_display.h"
#include "timer.h"

/**
 * Coding should be refactored as follows:
 * - first byte header
 *   - 3 bits -> report type/coding style:
 *     delay
 *     basic
 *     basicFollowedByEmpty
 *   - 2 bits:
 *     - no mods
 *     - 1 bit = left shift
 *     - 1 bit = right shift
 *     - both = full mod mask follow
 *   - 3 bits = number of scancodes, 7 = full size follows
 * - further bytes: mod mask and scancodes as specified by header
 */

bool RuntimeMacroPlaying = false;
bool RuntimeMacroRecording = false;
bool RuntimeMacroRecordingBlind = false;

static uint8_t reportBuffer[REPORT_BUFFER_MAX_LENGTH];
static uint16_t reportBufferLength = 0;

static runtime_macro_header headers[MAX_RUNTIME_MACROS];
static uint16_t headersLen = 0;

static runtime_macro_header *recordingHeader;
static runtime_macro_header *playbackHeader;
static uint16_t playbackPosition;

static bool delayActive;
static uint32_t delayStart;

static void initHeaderSlot(uint16_t id)
{
    recordingHeader = &headers[headersLen];
    headersLen++;
    recordingHeader->id = id;
    recordingHeader->offset = reportBufferLength;
    recordingHeader->length = 0;
}

static void discardHeaderSlot(uint8_t headerSlot)
{
    uint16_t offsetLeft = headers[headerSlot].offset;
    uint16_t offsetRight = headers[headerSlot].offset + headers[headerSlot].length;
    uint16_t length = headers[headerSlot].length;
    uint8_t *leftPtr = &reportBuffer[offsetLeft];
    uint8_t *rightPtr = &reportBuffer[offsetRight];
    memcpy(leftPtr, rightPtr, reportBufferLength - offsetRight);
    reportBufferLength = reportBufferLength - length;
    memcpy(&headers[headerSlot], &headers[headerSlot+1], (headersLen-headerSlot) * sizeof (*recordingHeader));
    headersLen = headersLen-1;
    for (int i = headerSlot; i < headersLen; i++) {
        headers[i].offset -= length;
    }
}

static void discardLastHeaderSlot()
{
    uint8_t headerSlot = headersLen - 1;
    uint16_t length = headers[headerSlot].length;
    reportBufferLength = reportBufferLength - length;
    headersLen = headersLen-1;
}

static void resolveRecordingHeader(uint16_t id)
{
    for (int i = 0; i < headersLen; i++)
    {
        if (headers[i].id == id)
        {
            discardHeaderSlot(i);
            break;
        }
    }
    while(headersLen == MAX_RUNTIME_MACROS || reportBufferLength > REPORT_BUFFER_MAX_LENGTH - REPORT_BUFFER_MIN_GAP) {
        discardHeaderSlot(0);
    }
    initHeaderSlot(id);
}

static bool resolvePlaybackHeader(uint16_t id)
{
    for (int i = 0; i < headersLen; i++)
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
    SegmentDisplay_SetText(3, "REC", SegmentDisplaySlot_Recording);
}

static void writeByte(uint8_t b)
{
    reportBuffer[reportBufferLength] = b;
    reportBufferLength++;
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
    SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Recording);
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

static void playReport(usb_basic_keyboard_report_t *report)
{
    macro_report_type_t type = readByte();
    switch(type) {
    case BasicKeyboardEmpty:
        memset(report, 0, sizeof *report);
        break;
    case BasicKeyboardSimple:
        memset(report, 0, sizeof *report);
        UsbBasicKeyboard_AddScancode(report, readByte());
        break;
    case BasicKeyboard:
        memset(report, 0, sizeof *report);
        {
            uint8_t size = readByte();
            report->modifiers = readByte();
            for (int i = 0; i < size; i++) {
                UsbBasicKeyboard_AddScancode(report, readByte());
            }
        }
        break;
    case Delay:
        {
            uint16_t timeout = readUInt16();
            if (!delayActive) {
                delayActive = true;
                delayStart = CurrentTime;
                playbackPosition -= 3;
            } else {
                if (Timer_GetElapsedTime(&delayStart) < timeout) {
                    playbackPosition -= 3;
                }
                else {
                    delayActive = false;
                }
            }
        }
        break;
    default:
        Macros_ReportErrorNum("PlayReport decode failed at ", type);
    }
}

static bool playRuntimeMacroBegin(uint16_t id)
{
    if (!resolvePlaybackHeader(id)) {
        return false;
    }
    playbackPosition = playbackHeader->offset;
    RuntimeMacroPlaying = true;
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

void MacroRecorder_RecordBasicReport(usb_basic_keyboard_report_t *report)
{
    if (!RuntimeMacroRecording) {
        return;
    }

    uint8_t sc = UsbBasicKeyboard_ScancodeCount(report);

    if (
            (reportBufferLength + 3 + sc > REPORT_BUFFER_MAX_LENGTH) ||
            (recordingHeader->length + 1 > REPORT_BUFFER_MAX_MACRO_LENGTH)
    ) {
        recordRuntimeMacroEnd();
        discardLastHeaderSlot();
        return;
    }
    if ((report->modifiers == 0) && (sc == 0)) {
        writeByte(BasicKeyboardEmpty);
        return;
    }
    if ((report->modifiers == 0) && (sc == 1)) {
        writeByte(BasicKeyboardSimple);
        writeReportScancodes(report);
        return;
    }

    writeByte(BasicKeyboard);
    writeByte(sc);
    writeByte(report->modifiers);
    writeReportScancodes(report);
}

void MacroRecorder_RecordDelay(uint16_t delay)
{
    if (!RuntimeMacroRecording) {
        return;
    }
    writeByte(Delay);
    writeUInt16(delay);
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
