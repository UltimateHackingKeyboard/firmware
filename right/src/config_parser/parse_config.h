#ifndef __PARSE_CONFIG_H__
#define __PARSE_CONFIG_H__

// Includes:

    #include "basic_types.h"
    #include "versioning.h"

// Macros:

#define RETURN_ON_ERROR(code) \
        errorCode = code; \
        if (errorCode != ParserError_Success) { \
            return errorCode; \
        } \

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
        ParserError_InvalidNavigationMode               = 16,
        ParserError_InvalidModuleProperty               = 17,
        ParserError_InvalidSecondaryRoleActionType      = 18,
        ParserError_InvalidHostType                     = 19,
    } parser_error_t;

    typedef enum {
        SerializedSecondaryRoleActionType_Primary,
        SerializedSecondaryRoleActionType_Secondary,
    } serialized_secondary_role_action_type_t;

    extern version_t DataModelVersion;

    extern bool PerKeyRgbPresent;
    extern bool ConfigParser_ConfigVersionIsEmpty;

// Functions:

    parser_error_t ParseConfig(config_buffer_t *buffer);

#endif
