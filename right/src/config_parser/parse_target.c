#include "parse_target.h"
#include "config_manager.h"
#include "config_parser/basic_types.h"
#include "config_parser/parse_config.h"
#include "target.h"

static void parseTarget(config_buffer_t* buffer, target_t* target) {
    target->type = ReadUInt8(buffer);

    if (target->type == TargetType_Ble || target->type == TargetType_Dongle) {
        for (uint8_t i = 0; i < BLE_ADDRESS_LENGTH; i++) {
            target->bleAddress[i] = ReadUInt8(buffer);
        }
    }

    if (target->type != TargetType_Empty) {
        uint16_t len;
        target->name.start = ReadString(buffer, &len);
        target->name.end = target->name.start + len;
    }
}

parser_error_t ParseTargets(config_buffer_t *buffer) {
    for (uint8_t targetId = 0; targetId < TARGET_COUNT_MAX; targetId++) {
        target_t dummy;
        parseTarget(buffer, &dummy);
    }

    return ParserError_Success;
}
