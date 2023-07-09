#ifndef __PARSE_CONFIG_H__
#define __PARSE_CONFIG_H__

// Includes:

    #include "basic_types.h"

// Typedefs:

    typedef enum {
        ParserError_Success                             =  0,
        ParserError_InvalidSerializedKeystrokeType      =  1,
        ParserError_InvalidSerializedMouseAction        =  2,
        ParserError_InvalidSerializedKeyActionType      =  3,
        ParserError_InvalidLayerCount                   =  4,
        ParserError_InvalidModuleCount                  =  5,
        ParserError_InvalidActionCount                  =  6,
        ParserError_InvalidSerializedMacroActionType    =  7,
        ParserError_InvalidSerializedSwitchKeymapAction =  8,
        ParserError_InvalidModuleConfigurationCount     =  9,
        ParserError_InvalidKeymapCount                  = 10,
        ParserError_InvalidAbbreviationLen              = 11,
        ParserError_InvalidMacroCount                   = 12,
        ParserError_InvalidSerializedPlayMacroAction    = 13,
        ParserError_InvalidMouseKineticProperty         = 14,
        ParserError_InvalidLayerId                      = 15,
    } parser_error_t;

    extern uint16_t DataModelMajorVersion;
    extern uint16_t DataModelMinorVersion;
    extern uint16_t DataModelPatchVersion;

    extern bool PerKeyRgbPresent;

// Functions:

    parser_error_t ParseConfig(config_buffer_t *buffer);

#endif
