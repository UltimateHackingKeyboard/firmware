#include "config_parser/parse_host_connection.h"
#include "config_parser/config_globals.h"
#include "config_parser/parse_config.h"
#include "config_parser/error_reporting.h"
#include "config_globals.h"
#include "config_manager.h"
#include "config_parser/basic_types.h"
#include "host_connection.h"
#include "parse_config.h"

#ifdef __ZEPHYR__
static parser_error_t parseHostConnection(config_buffer_t* buffer, host_connection_t* hostConnection) {
    hostConnection->type = ReadUInt8(buffer);

    // check validity of the type
    if (hostConnection->type >= HostConnectionType_Count) {
        hostConnection->type = HostConnectionType_Empty;
        ConfigParser_Error(buffer, "Invalid host type: %d\n", hostConnection->type);
        return ParserError_InvalidHostType;
    }

    if (hostConnection->type == HostConnectionType_Ble || hostConnection->type == HostConnectionType_Dongle) {
        hostConnection->bleAddress.type = 1;
        for (uint8_t i = 0; i < BLE_ADDRESS_LENGTH; i++) {
            hostConnection->bleAddress.a.val[i] = ReadUInt8(buffer);
        }
    }

    if (hostConnection->type != HostConnectionType_Empty) {
        if (VERSION_AT_LEAST(DataModelVersion, 8, 2, 0)) {
            hostConnection->switchOver = ReadUInt8(buffer);
        }

        uint16_t len;
        hostConnection->name.start = ReadString(buffer, &len);
        hostConnection->name.end = hostConnection->name.start + len;
    }

    return ParserError_Success;
}
#endif

parser_error_t ParseHostConnections(config_buffer_t *buffer) {
#ifdef __ZEPHYR__
    int errorCode;

    for (uint8_t hostConnectionId = 0; hostConnectionId < HOST_CONNECTION_COUNT_MAX; hostConnectionId++) {
        host_connection_t dummy;

        host_connection_t* hostConnection = ParserRunDry ? &dummy : &HostConnections[hostConnectionId];

        RETURN_ON_ERROR(
            parseHostConnection(buffer, hostConnection);
        );
    }
#endif

    return ParserError_Success;
}
