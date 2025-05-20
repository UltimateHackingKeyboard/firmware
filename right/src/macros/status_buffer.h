#ifndef __MACROS_STATUS_BUFFER_H__
#define __MACROS_STATUS_BUFFER_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "attributes.h"
    #include "macros/typedefs.h"
    #include "str_utils.h"

// Macros:

// Typedefs:

    #define Macros_Printf(...) Macros_PrintfWithPos(NULL, __VA_ARGS__)

    typedef struct {
        char data[STATUS_BUFFER_MAX_LENGTH];
        uint16_t len;
    } ATTR_PACKED macro_status_buffer_t;

// Variables:
//
    extern bool Macros_ConsumeStatusCharDirtyFlag;
    extern bool Macros_StatusBufferError;
    extern bool LastRunWasCrash;

// Functions:

    void Macros_ClearStatus(bool force);

    void Macros_ReportError(const char* err, const char* arg, const char *argEnd);
    void Macros_ReportErrorPrintf(const char* pos, const char *fmt, ...);
    void Macros_ReportErrorNum(const char* err, int32_t num, const char* pos);
    void Macros_ReportErrorFloat(const char* err, float num, const char* pos);
    void Macros_ReportWarn(const char* err, const char* arg, const char *argEnd);
    void Macros_PrintfWithPos(const char* pos, const char *fmt, ...);

    void Macros_SetStatusString(const char* text, const char *textEnd);
    void Macros_SetStatusStringInterpolated(const char* text, const char *textEnd);
    void Macros_SetStatusBool(bool b);
    void Macros_SetStatusFloat(float n);
    void Macros_SetStatusNum(int32_t n);
    void Macros_SetStatusNumSpaced(int32_t n, bool space);
    void Macros_SetStatusChar(char n);

    macro_result_t Macros_ProcessClearStatusCommand(bool force);
    macro_result_t Macros_ProcessSetStatusCommand(parser_context_t* ctx, bool addEndline);
    macro_result_t Macros_ProcessPrintStatusCommand();

    void MacroStatusBuffer_InitFromWormhole();
    void MacroStatusBuffer_InitNormal();

    void NotifyPrintf(const char *fmt, ...);

#endif
