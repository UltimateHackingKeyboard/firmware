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
        ParserError_InvalidSerializedConnectionAction   = 20,
        ParserError_InvalidHostConnectionId             = 21,
        ParserError_InvalidSerializedOtherAction        = 22,
        ParserError_ConfigVersionTooNew                = 23,
        ParserError_InvalidSecondaryRoleTriggeringEvent = 24,
        ParserError_InvalidSecondaryRoleTimeoutType     = 25,
    } parser_error_t;

    typedef enum {
        SerializedSecondaryRoleActionType_Primary,
        SerializedSecondaryRoleActionType_Secondary,
        SerializedSecondaryRoleActionType_NoOp,
    } serialized_secondary_role_action_type_t;

    typedef enum {
        SerializedSecondaryRoleTriggeringEvent_Press,
        SerializedSecondaryRoleTriggeringEvent_Release,
        SerializedSecondaryRoleTriggeringEvent_None,
    } serialized_secondary_role_triggering_event_t;

    typedef enum {
        SerializedSecondaryRoleTimeoutType_Active,
        SerializedSecondaryRoleTimeoutType_Passive,
    } serialized_secondary_role_timeout_type_t;

    typedef enum {
        SerializedChargingMode_Full = 0,
        SerializedChargingMode_StationaryMode = 1,
    } serialized_charging_mode_t;


    extern version_t DataModelVersion;

    extern bool PerKeyRgbPresent;

// Functions:

    parser_error_t ParseConfig(config_buffer_t *buffer);

#endif
