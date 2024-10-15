#include "error_reporting.h"
#include "config_parser/basic_types.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>
#include "macros/status_buffer.h"

void ConfigParser_Error(config_buffer_t *buffer, const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char printBuffer[256];
    vsprintf(printBuffer, fmt, myargs);
    Macros_ReportErrorPrintf(NULL, "%d: %s", buffer->offset, printBuffer);
    config_buffer_t myBuffer = *buffer;
    myBuffer.offset = buffer->offset >= 10 ? buffer->offset - 10 : 0;
    uint8_t windowCount = 5;
    for (uint8_t window = 0; window < windowCount; window++)
    {
        uint8_t context[10];
        for (uint8_t i = 0; i < 10; i++)
        {
            context[i] = ReadUInt8(&myBuffer);
        }
        Macros_ReportErrorPrintf(NULL, "%d: %u %u %u %u %u %u %u %u %u %u", myBuffer.offset-10, context[0], context[1], context[2], context[3], context[4], context[5], context[6], context[7], context[8], context[9]);
    }
}

