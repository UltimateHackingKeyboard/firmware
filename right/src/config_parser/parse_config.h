#ifndef __PARSE_CONFIG_H__
#define __PARSE_CONFIG_H__

// Includes:

    #include "basic_types.h"

// Typedefs:

    typedef enum {
        ParserError_Success,
        ParserError_InvalidSerializedKeystrokeType,
        ParserError_InvalidSerializedMouseAction,
        ParserError_InvalidSerializedKeyActionType,
        ParserError_InvalidLayerCount,
        ParserError_InvalidModuleCount,
        ParserError_InvalidActionCount,
        ParserError_InvalidSerializedMacroActionType,
        ParserError_InvalidSerializedSwitchKeymapAction,
        ParserError_InvalidModuleConfigurationCount,
        ParserError_InvalidKeymapCount,
        ParserError_InvalidAbbreviationLen,
        ParserError_InvalidMacroCount,
        ParserError_InvalidSerializedPlayMacroAction,
    } parser_error_t;

// Functions:

    parser_error_t ParseConfig(config_buffer_t *buffer);

#endif
