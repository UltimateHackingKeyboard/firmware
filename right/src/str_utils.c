#include "str_utils.h"
#include "config_parser/parse_keymap.h"
#include "macros.h"

int32_t ParseInt32(const char *a, const char *aEnd)
{
    bool negate = false;
    if(*a == '-')
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
    if(negate)
    {
        n = -n;
    }
    if(!numFound) {
        Macros_ReportError("Integer expected", NULL, NULL);
    }
    return n;
}

bool StrLessOrEqual(const char* a, const char* aEnd, const char* b, const char* bEnd)
{
    while(true) {
        if((*a == '\0' || a==aEnd) && (*b == '\0' || b==bEnd)) {
            return true;
        }
        else if(*a == '\0' || a==aEnd) {
            return true;
        }
        else if(*b == '\0' || b==bEnd) {
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
        if((*a == '\0' || a==aEnd) && (*b == '\0' || b==bEnd)) {
            return true;
        }
        else if(*a == '\0' || a==aEnd) {
            return false;
        }
        else if(*b == '\0' || b==bEnd) {
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
        if(*str == c) {
            return str;
        }
        str++;
    }
    return strEnd;
}


bool TokenMatches(const char *a, const char *aEnd, const char *b)
{
    while(a < aEnd && *b) {
        if(*a <= 32 || a == aEnd || *b <= 32) {
            return (*a <= 32 || a == aEnd) && *b <= 32;
        }
        if(*a++ != *b++){
            return false;
        }
    }
    return (*a <= 32 || a == aEnd) && *b <= 32;
}

bool TokenMatches2(const char *a, const char *aEnd, const char *b, const char *bEnd)
{
    while(a < aEnd && b < bEnd) {
        if(*a <= 32 || a == aEnd || *b <= 32 || b == bEnd) {
            return (*a <= 32 || a == aEnd) && *b <= 32;
        }
        if(*a++ != *b++){
            return false;
        }
    }
    return (*a <= 32 || a == aEnd) && (*b <= 32 || b == bEnd);
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
    if(cmd < cmdEnd - 1 && cmd[0] == '/' && cmd[1] == '/') {
        return cmdEnd;
    }
    return cmd;
}
