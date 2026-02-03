#ifndef SRC_STR_UTILS_H_
#define SRC_STR_UTILS_H_

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "layer.h"
    #include "module.h"
    #include "secondary_role_driver.h"
    #include "macros/typedefs.h"

    #ifdef __ZEPHYR__
        #include "device.h"
    #else
        #include "device.h"
    #endif

// Macros:

    #define EXPAND_SEGMENT(SEGMENT) SegmentLen(SEGMENT), SEGMENT.start
    #define EXPAND_REF(REF) REF.len, (const char*)ValidatedUserConfigBuffer.buffer + REF.offset
    #define CHAR_IS_VALID(C) ((C >= 32 && C < 127) || (C == '\n') || (C == '\r') || (C == '\t'))

    #define PARSER_CONTEXT_STACK_SIZE 4

    #define CTX_COPY(DST, SRC) \
        parser_context_t DST = SRC; \
        DST.nestingBound = DST.nestingLevel; \

// Typedefs:


    typedef struct macro_state_t macro_state_t;

    typedef struct {
        const macro_state_t* macroState;
        const char* begin;
        const char* at;
        const char* end;
        uint8_t nestingLevel;
        uint8_t nestingBound; // This context can't be popped below this bound, because it is a copy.
    } parser_context_t;

    typedef struct {
        uint16_t offset;
        uint8_t len;
    } ATTR_PACKED string_ref_t;

    typedef struct {
        const char* start;
        const char* end;
    } ATTR_PACKED string_segment_t;


// Functions:

    uint8_t SegmentLen(string_segment_t str);
    bool IsEnd(parser_context_t* ctx);
    bool SegmentEqual(string_segment_t str1, string_segment_t str2);
    bool StrLessOrEqual(const char* a, const char* aEnd, const char* b, const char* bEnd);
    bool StrEqual(const char* a, const char* aEnd, const char* b, const char* bEnd);
    const char* FindChar(char c, const char* str, const char* strEnd);
    bool StrContains(const char* str, const char* strEnd, const char* needle);
    bool ConsumeToken(parser_context_t* ctx, const char *b);
    void ConsumeAnyToken(parser_context_t* ctx);
    struct command_entry* ConsumeGperfToken(parser_context_t* ctx);
    void ConsumeCommentsAsWhite(bool consume);
    bool ConsumeTokenByRef(parser_context_t* ctx, string_ref_t ref);
    bool ConsumeIdentifierByRef(parser_context_t* ctx, string_ref_t ref);
    void ConsumeAnyIdentifier(parser_context_t* ctx);
    void ConsumeAnyChar(parser_context_t* ctx);
    void UnconsumeWhite(parser_context_t* ctx);
    const char* ConsumedToken(parser_context_t* ctx);
    bool IsIdentifierChar(char c);
    const char* IdentifierEnd(parser_context_t* ctx);
    const char* OneWord(parser_context_t* ctx);
    void ConsumeWhite(parser_context_t* ctx);
    bool TokenMatches(const char *a, const char *aEnd, const char *b);
    bool TokenMatches2(const char *a, const char *aEnd, const char *b, const char *bEnd);
    uint8_t TokLen(const char *a, const char *aEnd);
    const char* NextTok(const char* cmd, const char *cmdEnd);
    const char* NextCmd(const char* cmd, const char *cmdEnd);
    const char* CmdEnd(const char* cmd, const char *cmdEnd);
    void ConsumeUntilDot(parser_context_t* ctx);
    void ConsumeWhiteAt(parser_context_t* ctx, const char* at);
    const char* SkipWhite(const char* cmd, const char *cmdEnd);
    uint8_t CountCommands(const char* text, uint16_t textLen);
    const char* TokEnd(const char* cmd, const char *cmdEnd);
    module_id_t ConsumeModuleId(parser_context_t* ctx);
    secondary_role_state_t ConsumeSecondaryRoleTimeoutAction(parser_context_t* ctx);
    secondary_role_timeout_type_t ConsumeSecondaryRoleTimeoutType(parser_context_t* ctx);
    secondary_role_triggering_event_t ConsumeSecondaryRoleTriggeringEvent(parser_context_t* ctx);
    secondary_role_triggering_event_t ConsumeSecondaryRoleTriggerByPress(parser_context_t* ctx, secondary_role_triggering_event_t current);
    secondary_role_triggering_event_t ConsumeSecondaryRoleTriggerByRelease(parser_context_t* ctx, secondary_role_triggering_event_t current);
    secondary_role_strategy_t ConsumeSecondaryRoleStrategy(parser_context_t* ctx);
    navigation_mode_t ConsumeNavigationModeId(parser_context_t* ctx);
    const char* DeviceModelName(device_id_t device);

    bool PushParserContext(parser_context_t* ctx, const char* begin, const char* at, const char* end);
    bool PopParserContext(parser_context_t* ctx);
    parser_context_t CreateStringRefContext(string_ref_t ref);
    const parser_context_t* ViewContext(uint8_t level);

#ifdef __ZEPHYR__
    const char* Utils_DeviceIdToString(device_id_t deviceId);
#endif


#endif /* SRC_STR_UTILS_H_ */
