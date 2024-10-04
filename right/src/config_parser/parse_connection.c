#include "parse_connection.h"
#include "config_manager.h"
#include "config_parser/basic_types.h"
#include "config_parser/parse_config.h"
#include "connection.h"

static void parseConnection(config_buffer_t* buffer, connection_t* connection) {
    connection->type = ReadUInt8(buffer);

    if (connection->type == ConnectionType_Ble || connection->type == ConnectionType_Dongle) {
        for (uint8_t i = 0; i < BLE_ADDRESS_LENGTH; i++) {
            connection->bleAddress[i] = ReadUInt8(buffer);
        }
    }

    if (connection->type != ConnectionType_Empty) {
        uint16_t len;
        connection->name.start = ReadString(buffer, &len);
        connection->name.end = connection->name.start + len;
    }
}

parser_error_t ParseConnections(config_buffer_t *buffer) {
    for (uint8_t connectionId = 0; connectionId < CONNECTION_COUNT_MAX; connectionId++) {
        connection_t dummy;
        parseConnection(buffer, &dummy);
    }

    return ParserError_Success;
}
