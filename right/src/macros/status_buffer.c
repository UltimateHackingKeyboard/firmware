#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "macros/core.h"
#include "macros/scancode_commands.h"
#include "macros/status_buffer.h"
#include "macros/status_buffer.h"
#include "macros/string_reader.h"
#include "segment_display.h"
#include "config_parser/parse_macro.h"
#include "config_parser/config_globals.h"
#include <math.h>
#include <stdarg.h>
#include "macro_events.h"
#include "wormhole.h"
#include "trace.h"

#ifdef __ZEPHYR__
#include "keyboard/oled/widgets/widgets.h"
#include "flash.h"
#else
#include "eeprom.h"
#endif

static bool printing;
static bool containsWormholeData = false;
static uint16_t consumeStatusCharReadingPos = 0;
bool Macros_ConsumeStatusCharDirtyFlag = false;
bool Macros_StatusBufferError = false;

#define Buf StateWormhole.statusBuffer

static void updateErrorIndicator(bool newValue) {
#if DEVICE_HAS_OLED
    if (Macros_StatusBufferError != newValue) {
        Macros_StatusBufferError = newValue;
        Widget_Refresh(&StatusWidget);
    }
#endif
}

static void indicateError() {
    updateErrorIndicator(true);
    SegmentDisplay_SetText(3, "ERR", SegmentDisplaySlot_Error);
}

static void indicateWarn() {
    updateErrorIndicator(true);
    SegmentDisplay_SetText(3, "WRN", SegmentDisplaySlot_Warn);
}

static void indicateOut() {
    updateErrorIndicator(true);
    SegmentDisplay_SetText(3, "OUT", SegmentDisplaySlot_Error);
}

static void clearIndication() {
    updateErrorIndicator(false);
    SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Error);
    SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Warn);
}

macro_result_t Macros_ProcessClearStatusCommand(bool force)
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    if (force || !containsWormholeData) {
        Buf.len = 0;
        consumeStatusCharReadingPos = 0;
        Macros_ConsumeStatusCharDirtyFlag = false;
        clearIndication();
    }
    return MacroResult_Finished;
}

char Macros_ConsumeStatusChar()
{
    char res;

    if (consumeStatusCharReadingPos < Buf.len) {
        res = Buf.data[consumeStatusCharReadingPos++];
    } else {
        res = '\0';
        consumeStatusCharReadingPos = 0;
        Macros_ConsumeStatusCharDirtyFlag = false;
    }

    return res;
}

//textEnd is allowed to be null if text is null-terminated
static void setStatusStringInterpolated(parser_context_t* ctx)
{
    if (printing) {
        return;
    }
    uint16_t stringOffset = 0, textIndex = 0, textSubIndex = 0;

    char c;
    while (
            (c = Macros_ConsumeCharOfString(ctx, &stringOffset, &textIndex, &textSubIndex)) != '\0'
            && Buf.len < STATUS_BUFFER_MAX_LENGTH
          ) {
        Buf.data[Buf.len] = c;
        Buf.len++;
        Macros_ConsumeStatusCharDirtyFlag = true;
    }
}

static void setStatusChar(char n)
{
    if (printing) {
        return;
    }

    if (n && Buf.len == STATUS_BUFFER_MAX_LENGTH && DEBUG_ROLL_STATUS_BUFFER) {
        memcpy(Buf.data, &Buf.data[STATUS_BUFFER_MAX_LENGTH/2], STATUS_BUFFER_MAX_LENGTH/2);
        int16_t shiftBy = STATUS_BUFFER_MAX_LENGTH - STATUS_BUFFER_MAX_LENGTH/2;
        Buf.len = STATUS_BUFFER_MAX_LENGTH/2;
        consumeStatusCharReadingPos = consumeStatusCharReadingPos > shiftBy ? consumeStatusCharReadingPos - shiftBy : 0;
    }

    if (n && Buf.len < STATUS_BUFFER_MAX_LENGTH) {
        Buf.data[Buf.len] = n;
        Buf.len++;
        Macros_ConsumeStatusCharDirtyFlag = true;
    }
}

void Macros_SetStatusString(const char* text, const char *textEnd)
{
    if (printing) {
        return;
    }

    while(*text && Buf.len < STATUS_BUFFER_MAX_LENGTH && (text < textEnd || textEnd == NULL)) {
        setStatusChar(*text);
        text++;
    }
}

void Macros_SetStatusBool(bool b)
{
    Macros_SetStatusString(b ? "1" : "0", NULL);
}

void Macros_SetStatusNumSpaced(int32_t n, bool spaced)
{
    char buff[2];
    buff[0] = ' ';
    buff[1] = '\0';
    if (spaced) {
        Macros_SetStatusString(" ", NULL);
    }
    if (n < 0) {
        n = -n;
        buff[0] = '-';
        Macros_SetStatusString(buff, NULL);
    }
    int32_t orig = n;
    for (uint32_t div = 1000000000; div > 0; div /= 10) {
        buff[0] = (char)(((uint8_t)(n/div)) + '0');
        n = n%div;
        if (n!=orig || div == 1) {
          Macros_SetStatusString(buff, NULL);
        }
    }
}

void Macros_SetStatusFloat(float n)
{
    float intPart = 0;
    float fraPart = modff(n, &intPart);
    Macros_SetStatusNumSpaced(intPart, true);
    Macros_SetStatusString(".", NULL);
    Macros_SetStatusNumSpaced((int32_t)(fraPart * 1000 / 1), false);
}

void Macros_SetStatusNum(int32_t n)
{
    Macros_SetStatusNumSpaced(n, true);
}

void Macros_SetStatusChar(char n)
{
    setStatusChar(n);
}

static uint16_t findCurrentCommandLine()
{
    if (S != NULL) {
        uint16_t lineCount = 1;
        for (const char* c = S->ms.currentMacroAction.cmd.text; c < S->ms.currentMacroAction.cmd.text + S->ls->ms.commandBegin; c++) {
            if (*c == '\n') {
                lineCount++;
            }
        }
        return lineCount;
    }
    return 0;
}

static uint16_t findPosition(const char* arg)
{
    const char* startOfLine = S->ms.currentMacroAction.cmd.text + S->ls->ms.commandBegin;
    uint16_t pos;
    if (arg == NULL) {
        pos = 1;
    } else {
        pos = arg - startOfLine + 1;
    }
    return pos;
}

static void reportErrorHeader(const char* status, uint16_t pos)
{
    if (S != NULL) {
        const char *name, *nameEnd;
        uint16_t lineCount = findCurrentCommandLine(status);
        FindMacroName(&AllMacros[S->ms.currentMacroIndex], &name, &nameEnd);
        Macros_SetStatusString(status, NULL);
        Macros_SetStatusString(" at ", NULL);
        Macros_SetStatusString(name, nameEnd);
        Macros_SetStatusString(" ", NULL);
        Macros_SetStatusNumSpaced(S->ms.currentMacroActionIndex+1, false);
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNumSpaced(lineCount, false);
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNumSpaced(pos, false);
        Macros_SetStatusString(": ", NULL);
    }
}

static void reportCommandLocation(uint16_t line, uint16_t pos, const char* begin, const char* end, bool reportPosition)
{
    Macros_SetStatusString("> ", NULL);
    uint16_t l = Buf.len;
    Macros_SetStatusNumSpaced(line, false);
    l = Buf.len - l;
    Macros_SetStatusString(" | ", NULL);
    Macros_SetStatusString(begin, end);
    Macros_SetStatusString("\n", NULL);
    if (reportPosition) {
        Macros_SetStatusString("> ", NULL);
        for (uint8_t i = 0; i < l; i++) {
            Macros_SetStatusString(" ", NULL);
        }
        Macros_SetStatusString(" | ", NULL);
        for (uint8_t i = 0; i < pos; i++) {
            Macros_SetStatusString(" ", NULL);
        }
        Macros_SetStatusString("^", NULL);
        Macros_SetStatusString("\n", NULL);
    }
}

static void reportError(
    const char* err,
    const char* arg,
    const char* argEnd
) {
    Macros_SetStatusString(err, NULL);

    if (S != NULL) {
        bool argIsCommand = ValidatedUserConfigBuffer.buffer <= (uint8_t*)arg && (uint8_t*)arg < ValidatedUserConfigBuffer.buffer + USER_CONFIG_SIZE;
        if (arg != NULL && arg != argEnd) {
            Macros_SetStatusString(" ", NULL);
            Macros_SetStatusString(arg, TokEnd(arg, argEnd));
        }
        Macros_SetStatusString("\n", NULL);
        const char* startOfLine = S->ms.currentMacroAction.cmd.text + S->ls->ms.commandBegin;
        const char* endOfLine = S->ms.currentMacroAction.cmd.text + S->ls->ms.commandEnd;
        uint16_t line = findCurrentCommandLine();
        reportCommandLocation(line, arg-startOfLine, startOfLine, endOfLine, argIsCommand);
    } else {
        if (arg != NULL && arg != argEnd) {
            Macros_SetStatusString(" ", NULL);
            Macros_SetStatusString(arg, argEnd);
        }
        Macros_SetStatusString("\n", NULL);
    }
}

void Macros_ReportError(const char* err, const char* arg, const char *argEnd)
{
    //if this line of code already caused an error, don't throw another one
    Macros_ParserError = true;
    indicateError();
    reportErrorHeader("Error", findPosition(arg));
    reportError(err, arg, argEnd);
    MacroEvent_OnError();
}

void Macros_ReportWarn(const char* err, const char* arg, const char *argEnd)
{
    if (!Macros_ValidationInProgress) {
        return;
    }
    indicateWarn();
    reportErrorHeader("Warning", findPosition(arg));
    reportError(err, arg, argEnd);
}

void Macros_PrintfWithPos(const char* pos, const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);

    indicateOut();
    if (pos != NULL) {
        reportErrorHeader("Out", findPosition(pos));
        reportError(buffer, pos, pos);
    } else {
        Macros_SetStatusString(buffer, NULL);
    }
}

void Macros_ReportErrorPrintf(const char* pos, const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);
    Macros_ReportError(buffer, pos, pos);
}

void Macros_ReportErrorFloat(const char* err, float num, const char* pos)
{
    Macros_ParserError = true;
    indicateError();
    reportErrorHeader("Error", findPosition(pos));
    Macros_SetStatusString(err, NULL);
    Macros_SetStatusFloat(num);
    reportError("", pos, pos);
}

void Macros_ReportErrorNum(const char* err, int32_t num, const char* pos)
{
    Macros_ParserError = true;
    indicateError();
    reportErrorHeader("Error", findPosition(pos));
    Macros_SetStatusString(err, NULL);
    Macros_SetStatusNum(num);
    reportError("", pos, pos);
}

macro_result_t Macros_ProcessPrintStatusCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    printing = true;
    macro_result_t res = Macros_DispatchText(Buf.data, Buf.len, true);
    if (res == MacroResult_Finished) {
        Macros_ProcessClearStatusCommand(true);
        printing = false;
    }
    return res;
}

macro_result_t Macros_ProcessSetStatusCommand(parser_context_t* ctx, bool addEndline)
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }
    setStatusStringInterpolated(ctx);
    if (addEndline) {
        Macros_SetStatusString("\n", NULL);
    }
    return MacroResult_Finished;
}

void Macros_ClearStatus(bool force)
{
    Macros_ProcessClearStatusCommand(force);
}

void MacroStatusBuffer_InitFromWormhole() {
    containsWormholeData = StateWormhole.persistStatusBuffer;

    if (containsWormholeData) {
        indicateError();
    } else {
        Macros_ProcessClearStatusCommand(true);
    }
}

void MacroStatusBuffer_InitNormal() {
    Macros_ProcessClearStatusCommand(true);
}
