#include "config_parser/parse_host_connection.h"
#include "config_parser/config_globals.h"
#include "config_parser/parse_config.h"
#include "config_parser/error_reporting.h"
#include "config_globals.h"
#include "config_manager.h"
#include "config_parser/basic_types.h"
#include "host_connection.h"
#include "parse_config.h"
#include <string.h>

#ifdef __ZEPHYR__
#include "bt_conn.h"
#endif

static parser_error_t parseHostConnection(config_buffer_t* buffer, host_connection_t* hostConnection) {
    host_connection_type_t hostType = ReadUInt8(buffer);

    // check validity of the type
    if (hostType >= HostConnectionType_Count) {
        hostType = HostConnectionType_Empty;
        ConfigParser_Error(buffer, "Invalid host type: %d\n", hostType);
        return ParserError_InvalidHostType;
    }

    // Handle overwrite logic - don't overwrite unregistered slots by empty slots
    if (hostType == HostConnectionType_Empty) {
        if (hostConnection->type == HostConnectionType_UnregisteredBtHid) {
            return ParserError_Success;
        } else {
            hostConnection->type = HostConnectionType_Empty;
            return ParserError_Success;
        }
    }

    // Parse the connection
    {
        // Set type
        hostConnection->type = (host_connection_type_t)hostType;

        // Set address
        if (hostConnection->type == HostConnectionType_BtHid || hostConnection->type == HostConnectionType_Dongle) {
            for (uint8_t i = 0; i < BLE_ADDRESS_LENGTH; i++) {
                hostConnection->bleAddress.a.val[i] = ReadUInt8(buffer);
            }
            hostConnection->bleAddress.type = hostConnection->bleAddress.a.val[0] & 0x01;
        }

        // Set name and switchover
        if (hostConnection->type != HostConnectionType_Empty) {
            if (VERSION_AT_LEAST(DataModelVersion, 8, 3, 0)) {
                hostConnection->switchover = ReadUInt8(buffer);
            }

            uint16_t len;
            hostConnection->name.start = ReadString(buffer, &len);
            hostConnection->name.end = hostConnection->name.start + len;
        }
    }

    return ParserError_Success;
}

static void deduplicateUnregisteredConnections(void) {
#ifdef __ZEPHYR__
    if (ParserRunDry) {
        return;
    }

    for (uint8_t i = 0; i < SERIALIZED_HOST_CONNECTION_COUNT_MAX; i++) {
        host_connection_t* conn = &HostConnections[i];

        if (conn->type != HostConnectionType_UnregisteredBtHid) {
            continue;
        }

        for (uint8_t j = 0; j < SERIALIZED_HOST_CONNECTION_COUNT_MAX; j++) {
            if (i == j) {
                continue;
            }

            host_connection_t* other = &HostConnections[j];

            if (other->type != HostConnectionType_BtHid &&
                other->type != HostConnectionType_Dongle &&
                other->type != HostConnectionType_UnregisteredBtHid) {
                continue;
            }

            if (BtAddrEq(&conn->bleAddress, &other->bleAddress)) {
                conn->type = HostConnectionType_Empty;
                memset(&conn->bleAddress, 0, sizeof(bt_addr_le_t));
                conn->name = (string_segment_t){ .start = NULL, .end = NULL };
                conn->switchover = false;
                break;
            }
        }
    }
#endif
}

parser_error_t ParseHostConnections(config_buffer_t *buffer) {
    int errorCode;

    for (uint8_t hostConnectionId = 0; hostConnectionId < SERIALIZED_HOST_CONNECTION_COUNT_MAX; hostConnectionId++) {
        host_connection_t dummy = { .type = HostConnectionType_Empty };

        host_connection_t* hostConnection = &dummy;

#ifdef __ZEPHYR__
        hostConnection = ParserRunDry ? &dummy : &HostConnections[hostConnectionId];
#endif

        RETURN_ON_ERROR(
            parseHostConnection(buffer, hostConnection);
        );
    }

    deduplicateUnregisteredConnections();

    return ParserError_Success;
}
