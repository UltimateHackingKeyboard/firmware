#include "str_utils.h"
#include "debug.h"
#include "config_parser/config_globals.h"
#include "config_parser/parse_keymap.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "macros/vars.h"
#include "module.h"
#include "slave_protocol.h"

static bool consumeCommentsAsWhite = true;

static bool isIdentifierChar(char c);

float ParseFloat(const char *a, const char *aEnd)
{
    parser_context_t ctx = { .at = a, .end = aEnd };
    return Macros_ConsumeFloat(&ctx);
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


static void consumeWhite(parser_context_t* ctx)
{
    while (*ctx->at <= 32 && ctx->at < ctx->end) {
        ctx->at++;
    }
    if (ctx->at[0] == '/' && ctx->at[1] == '/' && consumeCommentsAsWhite) {
        while (*ctx->at != '\n' && *ctx->at != '\r' && ctx->at < ctx->end) {
            ctx->at++;
        }
    }
}


void ConsumeCommentsAsWhite(bool consume)
{
    consumeCommentsAsWhite = consume;
}

void ConsumeWhite(parser_context_t* ctx)
{
    consumeWhite(ctx);
}

void ConsumeWhiteAt(parser_context_t* ctx, const char* at)
{
    ctx->at = at;
    consumeWhite(ctx);
}

void UnconsumeWhite(parser_context_t* ctx)
{
    while (ctx->at > ctx->begin && *(ctx->at-1) <= 32) {
        ctx->at--;
    }
}

const char* OneWord(parser_context_t* ctx)
{
    static char buffer[20];
    uint8_t len = MIN(19, TokEnd(ctx->at, ctx->end) - ctx->at);
    memcpy(&buffer, ctx->at, len);
    buffer[len] = '\0';
    return buffer;
}

const char* ConsumedToken(parser_context_t* ctx)
{
    const char* at = ctx->at;

    at--;

    while (*at <= 32) {
        at--;
    }
    while (*at > 32 && *at != '.') {
        at--;
    }

    return at;
}

bool ConsumeToken(parser_context_t* ctx, const char *b)
{
    const char* at = ctx->at;
    while(at < ctx->end && *b) {
        if (*at <= 32 || at == ctx->end || *b <= 32) {
            bool res = (*at <= 32 || at == ctx->end) && *b <= 32;
            if (res) {
                ctx->at = at;
                consumeWhite(ctx);
            }
            return res;
        }
        if (*at++ != *b++) {
            return false;
        }
    }
    bool res = (*at <= 32 || at == ctx->end || !isIdentifierChar(*at) || !isIdentifierChar(*(at-1))) && *b <= 32;
    if (res) {
        ctx->at = at;
        consumeWhite(ctx);
    }
    return res;
}

bool ConsumeTokenByRef(parser_context_t* ctx, string_ref_t ref)
{
    const char* at = ctx->at;
    const char* b = (const char*)(ValidatedUserConfigBuffer.buffer + ref.offset);
    const char* bEnd = (const char*)(ValidatedUserConfigBuffer.buffer + ref.offset + ref.len);
    while(at < ctx->end && b < bEnd) {
        if (*at <= 32 || at == ctx->end || *b <= 32 || b == bEnd) {
            bool res = (*at <= 32 || at == ctx->end) && *b <= 32;
            if (res) {
                ctx->at = at;
                consumeWhite(ctx);
            }
            return res;
        }
        if (*at++ != *b++) {
            return false;
        }
    }
    bool res = (*at <= 32 || at == ctx->end || *at == '.') && (*b <= 32 || b == bEnd);
    if (res) {
        ctx->at = at;
        consumeWhite(ctx);
    }
    return res;
}

static bool isIdentifierChar(char c)
{
    switch (c) {
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '0' ... '9':
        case '_':
            return true;
        default:
            return false;
    }
}


bool IsIdentifierChar(char c)
{
    return isIdentifierChar(c);
}


bool ConsumeIdentifierByRef(parser_context_t* ctx, string_ref_t ref)
{
    const char* at = ctx->at;
    const char* b = (const char*)(ValidatedUserConfigBuffer.buffer + ref.offset);
    const char* bEnd = (const char*)(ValidatedUserConfigBuffer.buffer + ref.offset + ref.len);
    while(at < ctx->end && b < bEnd) {
        if (!isIdentifierChar(*at) || at == ctx->end || !isIdentifierChar(*b) || b == bEnd) {
            bool res = (!isIdentifierChar(*at) || at == ctx->end) && (!isIdentifierChar(*b) || b == bEnd);
            if (res) {
                ctx->at = at;
                consumeWhite(ctx);
            }
            return res;
        }
        if (*at++ != *b++) {
            return false;
        }
    }

    bool res = (!isIdentifierChar(*at) || at == ctx->end) && (!isIdentifierChar(*b) || b == bEnd);
    if (res) {
        ctx->at = at;
        consumeWhite(ctx);
    }
    return res;
}

const char* IdentifierEnd(parser_context_t* ctx)
{
    const char* at = ctx->at;
    while (isIdentifierChar(*at) && at < ctx->end) {
        at++;
    }
    return at;
}

void ConsumeAnyIdentifier(parser_context_t* ctx)
{
    ctx->at = IdentifierEnd(ctx);
    consumeWhite(ctx);
}

void ConsumeUntilDot(parser_context_t* ctx)
{
    while(*ctx->at > 32 && *ctx->at != '.' && ctx->at < ctx->end)    {
        ctx->at++;
    }
    if (*ctx->at != '.') {
        Macros_ReportError("'.' expected", ctx->at, ctx->at);
    }
    ctx->at++;
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


const char* CmdEnd(const char* cmd, const char *cmdEnd)
{
    while(*cmd != '\n' && *cmd != '\r' && cmd < cmdEnd)    {
        cmd++;
    }
    return cmd;
}

const char* SkipWhite(const char* cmd, const char *cmdEnd)
{
    while(*cmd <= 32 && cmd < cmdEnd) {
        cmd++;
    }
    return cmd;
}

module_id_t ConsumeModuleId(parser_context_t* ctx)
{
    if (ConsumeToken(ctx, "keycluster")) {
        return ModuleId_KeyClusterLeft;
    }
    else if (ConsumeToken(ctx, "trackball")) {
        return ModuleId_TrackballRight;
    }
    else if (ConsumeToken(ctx, "trackpoint")) {
        return ModuleId_TrackpointRight;
    }
    else if (ConsumeToken(ctx, "touchpad")) {
        return ModuleId_TouchpadRight;
    }
    Macros_ReportError("Module not recognized: ", ctx->at, ctx->end);
    return 0;
}

module_id_t ParseModuleId(const char* arg1, const char* cmdEnd)
{
    parser_context_t ctx = { .at = arg1, .end = cmdEnd };
    return ConsumeModuleId(&ctx);
}

navigation_mode_t ConsumeNavigationModeId(parser_context_t* ctx)
{
    if (ConsumeToken(ctx, "cursor")) {
        return NavigationMode_Cursor;
    }
    else if (ConsumeToken(ctx, "scroll")) {
        return NavigationMode_Scroll;
    }
    else if (ConsumeToken(ctx, "caret")) {
        return NavigationMode_Caret;
    }
    else if (ConsumeToken(ctx, "media")) {
        return NavigationMode_Media;
    }
    else if (ConsumeToken(ctx, "zoom")) {
        return NavigationMode_Zoom;
    }
    else if (ConsumeToken(ctx, "zoomPc")) {
        return NavigationMode_ZoomPc;
    }
    else if (ConsumeToken(ctx, "zoomMac")) {
        return NavigationMode_ZoomMac;
    }
    else if (ConsumeToken(ctx, "none")) {
        return NavigationMode_None;
    }
    Macros_ReportError("Mode not recognized: ", ctx->at, ctx->end);
    return 0;
}


navigation_mode_t ParseNavigationModeId(const char* arg1, const char* cmdEnd)
{
    parser_context_t ctx = { .at = arg1, .end = cmdEnd };
    return ConsumeNavigationModeId(&ctx);
}

secondary_role_state_t ConsumeSecondaryRoleTimeoutAction(parser_context_t* ctx)
{
    if (ConsumeToken(ctx, "primary")) {
        return SecondaryRoleState_Primary;
    }
    else if (ConsumeToken(ctx, "secondary")) {
        return SecondaryRoleState_Secondary;
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
        return SecondaryRoleState_DontKnowYet;
    }
}

secondary_role_strategy_t ConsumeSecondaryRoleStrategy(parser_context_t* ctx)
{
    if (ConsumeToken(ctx, "simple")) {
        return SecondaryRoleStrategy_Simple;
    }
    else if (ConsumeToken(ctx, "advanced")) {
        return SecondaryRoleStrategy_Advanced;
    }
    else {
        Macros_ReportError("Parameter not recognized:", ctx->at, ctx->end);
        return SecondaryRoleStrategy_Simple;
    }
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


