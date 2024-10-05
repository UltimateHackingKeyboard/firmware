#include "parse_host_connection.h"
#include "config_manager.h"
#include "config_parser/basic_types.h"
#include "config_parser/parse_config.h"
#include "host_connection.h"

static void parseHostConnection(config_buffer_t* buffer, host_connection_t* host_connection) {
    host_connection->type = ReadUInt8(buffer);

    if (host_connection->type == HostConnectionType_Ble || host_connection->type == HostConnectionType_Dongle) {
        for (uint8_t i = 0; i < BLE_ADDRESS_LENGTH; i++) {
            host_connection->bleAddress[i] = ReadUInt8(buffer);
        }
    }

    if (host_connection->type != HostConnectionType_Empty) {
        uint16_t len;
        host_connection->name.start = ReadString(buffer, &len);
        host_connection->name.end = host_connection->name.start + len;
    }
}

parser_error_t ParseHostConnections(config_buffer_t *buffer) {
    for (uint8_t host_connectionId = 0; host_connectionId < HOST_CONNECTION_COUNT_MAX; host_connectionId++) {
        host_connection_t dummy;
        parseHostConnection(buffer, &dummy);
    }

    return ParserError_Success;
}
