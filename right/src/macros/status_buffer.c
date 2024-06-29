#include <stdio.h>
#include <string.h>
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

#ifdef __ZEPHYR__
#include "flash.h"
#else
#include "eeprom.h"
#endif

static uint16_t consumeStatusCharReadingPos = 0;
bool Macros_ConsumeStatusCharDirtyFlag = false;

static char statusBuffer[STATUS_BUFFER_MAX_LENGTH];
static uint16_t statusBufferLen;
static bool statusBufferPrinting;

macro_result_t Macros_ProcessClearStatusCommand()
{
    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    statusBufferLen = 0;
    consumeStatusCharReadingPos = 0;
    Macros_ConsumeStatusCharDirtyFlag = false;
    SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Error);
    SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Warn);
    return MacroResult_Finished;
}


char Macros_ConsumeStatusChar()
{
    char res;

    if (consumeStatusCharReadingPos < statusBufferLen) {
        res = statusBuffer[consumeStatusCharReadingPos++];
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
    if (statusBufferPrinting) {
        return;
    }
    uint16_t stringOffset = 0, textIndex = 0, textSubIndex = 0;

    char c;
    while (
            (c = Macros_ConsumeCharOfString(ctx, &stringOffset, &textIndex, &textSubIndex)) != '\0'
            && statusBufferLen < STATUS_BUFFER_MAX_LENGTH
          ) {
        statusBuffer[statusBufferLen] = c;
        statusBufferLen++;
        Macros_ConsumeStatusCharDirtyFlag = true;
    }
}

static void setStatusChar(char n)
{
    if (statusBufferPrinting) {
        return;
    }

    if (n && statusBufferLen == STATUS_BUFFER_MAX_LENGTH) {
        memcpy(statusBuffer, &statusBuffer[STATUS_BUFFER_MAX_LENGTH/2], STATUS_BUFFER_MAX_LENGTH/2);
        statusBufferLen = STATUS_BUFFER_MAX_LENGTH/2;
    }

    if (n && statusBufferLen < STATUS_BUFFER_MAX_LENGTH) {
        statusBuffer[statusBufferLen] = n;
        statusBufferLen++;
        Macros_ConsumeStatusCharDirtyFlag = true;
    }
}

void Macros_SetStatusString(const char* text, const char *textEnd)
{
    if (statusBufferPrinting) {
        return;
    }

    while(*text && statusBufferLen < STATUS_BUFFER_MAX_LENGTH && (text < textEnd || textEnd == NULL)) {
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
    uint16_t l = statusBufferLen;
    Macros_SetStatusNumSpaced(line, false);
    l = statusBufferLen - l;
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
    SegmentDisplay_SetText(3, "ERR", SegmentDisplaySlot_Error);
    reportErrorHeader("Error", findPosition(arg));
    reportError(err, arg, argEnd);
}

void Macros_ReportWarn(const char* err, const char* arg, const char *argEnd)
{
    if (!Macros_ValidationInProgress) {
        return;
    }
    SegmentDisplay_SetText(3, "WRN", SegmentDisplaySlot_Warn);
    reportErrorHeader("Warning", findPosition(arg));
    reportError(err, arg, argEnd);
}

void Macros_ReportPrintf(const char* pos, const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);

    SegmentDisplay_SetText(3, "OUT", SegmentDisplaySlot_Error);
    if (pos != NULL) {
        reportErrorHeader("Out", findPosition(pos));
        reportError(buffer, pos, pos);
    } else {
        Macros_SetStatusString(buffer, NULL);
        Macros_SetStatusString("\n", NULL);
    }
}

void Macros_ReportErrorPrintf(const char* pos, const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    sprintf(buffer, fmt, myargs);
    Macros_ReportError(buffer, pos, pos);

}

void Macros_ReportErrorFloat(const char* err, float num, const char* pos)
{
    Macros_ParserError = true;
    SegmentDisplay_SetText(3, "ERR", SegmentDisplaySlot_Error);
    reportErrorHeader("Error", findPosition(pos));
    Macros_SetStatusString(err, NULL);
    Macros_SetStatusFloat(num);
    reportError("", pos, pos);
}

void Macros_ReportErrorNum(const char* err, int32_t num, const char* pos)
{
    Macros_ParserError = true;
    SegmentDisplay_SetText(3, "ERR", SegmentDisplaySlot_Error);
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
    statusBufferPrinting = true;
    macro_result_t res = Macros_DispatchText(statusBuffer, statusBufferLen, true);
    if (res == MacroResult_Finished) {
        Macros_ProcessClearStatusCommand();
        statusBufferPrinting = false;
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

void Macros_ClearStatus(void)
{
    SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Error);
    Macros_ProcessClearStatusCommand();
}
