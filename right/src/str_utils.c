#include "str_utils.h"
#include "config_parser/parse_keymap.h"
#include "macros.h"
#include "slave_protocol.h"

float ParseFloat(const char *a, const char *aEnd)
{
    bool negate = false;
    if (*a == '-')
    {
        negate = !negate;
        a++;
    }
    float n = 0;
    bool numFound = false;
    while(*a > 47 && *a < 58 && a < aEnd) {
        n = n*10 + ((uint8_t)(*a))-48;
        a++;
        numFound = true;
    }
    if (*a == '.') {
        a++;
    }
    float b = 0.1;
    float d = 0.0f;
    while(*a > 47 && *a < 58 && a < aEnd) {
        d = d + (((uint8_t)(*a))-48)*b;
        b = b/10.0f;
        a++;
    }
    n += d;
    if (negate)
    {
        n = -n;
    }
    if (!numFound) {
        Macros_ReportError("Float expected", NULL, NULL);
    }
    return n;
}

int32_t ParseInt32_2(const char *a, const char *aEnd, const char* *parsedTill)
{
    bool negate = false;
    if (*a == '-')
    {
        negate = !negate;
        a++;
    }
    int32_t n = 0;
    bool numFound = false;
    while(*a > 47 && *a < 58 && a < aEnd) {
        n = n*10 + ((uint8_t)(*a))-48;
        a++;
        numFound = true;
    }
    if (negate)
    {
        n = -n;
    }
    if (!numFound) {
        Macros_ReportError("Integer expected", NULL, NULL);
    }
    if (parsedTill != NULL) {
        *parsedTill = a;
    }
    return n;
}

int32_t ParseInt32(const char *a, const char *aEnd)
{
    return ParseInt32_2(a, aEnd, NULL);
}

bool StrLessOrEqual(const char* a, const char* aEnd, const char* b, const char* bEnd)
{
    while(true) {
        if ((*a == '\0' || a==aEnd) && (*b == '\0' || b==bEnd)) {
            return true;
        }
        else if (*a == '\0' || a==aEnd) {
            return true;
        }
        else if (*b == '\0' || b==bEnd) {
            return false;
        }
        else if (*a < *b) {
            return true;
        }
        else if (*b < *a) {
            return false;
        }
        else {
            a++;
            b++;
        }
    }
}


bool StrEqual(const char* a, const char* aEnd, const char* b, const char* bEnd)
{
    while(true) {
        if ((*a == '\0' || a==aEnd) && (*b == '\0' || b==bEnd)) {
            return true;
        }
        else if (*a == '\0' || a==aEnd) {
            return false;
        }
        else if (*b == '\0' || b==bEnd) {
            return false;
        }
        else if (*a != *b) {
            return false;
        }
        else {
            a++;
            b++;
        }
    }
}

const char* FindChar(char c, const char* str, const char* strEnd)
{
    while(str < strEnd) {
        if (*str == c) {
            return str;
        }
        str++;
    }
    return strEnd;
}


bool TokenMatches(const char *a, const char *aEnd, const char *b)
{
    while(a < aEnd && *b) {
        if (*a <= 32 || a == aEnd || *b <= 32) {
            return (*a <= 32 || a == aEnd) && *b <= 32;
        }
        if (*a++ != *b++) {
            return false;
        }
    }
    return (*a <= 32 || a == aEnd || *a == '.') && *b <= 32;
}

bool TokenMatches2(const char *a, const char *aEnd, const char *b, const char *bEnd)
{
    while(a < aEnd && b < bEnd) {
        if (*a <= 32 || a == aEnd || *b <= 32 || b == bEnd) {
            return (*a <= 32 || a == aEnd) && *b <= 32;
        }
        if (*a++ != *b++) {
            return false;
        }
    }
    return (*a <= 32 || a == aEnd || *a == '.') && (*b <= 32 || b == bEnd);
}

uint8_t TokLen(const char *a, const char *aEnd)
{
    uint8_t l = 0;
    while(*a > 32 && a < aEnd) {
        l++;
        a++;
    }
    return l;
}

const char* TokEnd(const char* cmd, const char *cmdEnd)
{
    while(*cmd > 32 && cmd < cmdEnd)    {
        cmd++;
    }
    return cmd;
}

const char* NextTok(const char* cmd, const char *cmdEnd)
{
    while(*cmd > 32 && cmd < cmdEnd)    {
        cmd++;
    }
    while(*cmd <= 32 && cmd < cmdEnd) {
        cmd++;
    }
    if (cmd < cmdEnd - 1 && cmd[0] == '/' && cmd[1] == '/') {
        return cmdEnd;
    }
    return cmd;
}

const char* NextCmd(const char* cmd, const char *cmdEnd)
{
    while(*cmd != '\n' && *cmd != '\r' && cmd < cmdEnd)    {
        cmd++;
    }
    while(*cmd <= 32 && cmd < cmdEnd) {
        cmd++;
    }
    return cmd;
}

module_id_t ParseModuleId(const char* arg1, const char* cmdEnd)
{
    if (TokenMatches(arg1, cmdEnd, "keycluster")) {
        return ModuleId_KeyClusterLeft;
    }
    else if (TokenMatches(arg1, cmdEnd, "trackball")) {
        return ModuleId_TrackballRight;
    }
    else if (TokenMatches(arg1, cmdEnd, "trackpoint")) {
        return ModuleId_TrackpointRight;
    }
    else if (TokenMatches(arg1, cmdEnd, "touchpad")) {
        return ModuleId_TouchpadRight;
    }
    Macros_ReportError("Module not recognized: ", arg1, cmdEnd);
    return 0;
}

navigation_mode_t ParseNavigationModeId(const char* arg1, const char* cmdEnd)
{
    if (TokenMatches(arg1, cmdEnd, "cursor")) {
        return NavigationMode_Cursor;
    }
    else if (TokenMatches(arg1, cmdEnd, "scroll")) {
        return NavigationMode_Scroll;
    }
    else if (TokenMatches(arg1, cmdEnd, "caret")) {
        return NavigationMode_Caret;
    }
    else if (TokenMatches(arg1, cmdEnd, "media")) {
        return NavigationMode_Media;
    }
    else if (TokenMatches(arg1, cmdEnd, "zoom")) {
        return NavigationMode_Zoom;
    }
    else if (TokenMatches(arg1, cmdEnd, "zoomPc")) {
        return NavigationMode_ZoomPc;
    }
    else if (TokenMatches(arg1, cmdEnd, "zoomMac")) {
        return NavigationMode_ZoomMac;
    }
    else if (TokenMatches(arg1, cmdEnd, "none")) {
        return NavigationMode_None;
    }
    Macros_ReportError("Mode not recognized: ", arg1, cmdEnd);
    return 0;
}

uint8_t CountCommands(const char* text, uint16_t textLen)
{
    uint8_t count = 1;
    const char* textEnd = text + textLen;

    while ( *text <= 32 && text < textEnd) {
        text++;
    }

    while (true) {
        text = NextCmd(text, textEnd);
        if (text == textEnd) {
            return count;
        }
        if (*text > 32) {
            count++;
        }
    }
}
