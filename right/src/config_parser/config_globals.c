#include <stdint.h>
#include <stddef.h>
#include "config_globals.h"
#include "attributes.h"
#include "eeprom.h"

static uint8_t hardwareConfig[HARDWARE_CONFIG_SIZE];
static uint8_t ATTR_DATA2 stagingUserConfig[USER_CONFIG_SIZE];
static uint8_t validatedUserConfig[USER_CONFIG_SIZE];

uint16_t ValidatedUserConfigLength;
config_buffer_t HardwareConfigBuffer = { .buffer = hardwareConfig, .offset = 0 };
config_buffer_t StagingUserConfigBuffer = { .buffer = stagingUserConfig, .offset = 0 };
config_buffer_t ValidatedUserConfigBuffer = { .buffer = validatedUserConfig, .offset = 0 };

hardware_config_t *HardwareConfig = (hardware_config_t*)hardwareConfig;

bool ParserRunDry;

bool IsConfigBufferIdValid(config_buffer_id_t configBufferId)
{
    return ConfigBufferId_HardwareConfig <= configBufferId && configBufferId <= ConfigBufferId_ValidatedUserConfig;
}

config_buffer_t* ConfigBufferIdToConfigBuffer(config_buffer_id_t configBufferId)
{
    switch (configBufferId) {
        case ConfigBufferId_HardwareConfig:
            return &HardwareConfigBuffer;
        case ConfigBufferId_StagingUserConfig:
            return &StagingUserConfigBuffer;
        case ConfigBufferId_ValidatedUserConfig:
            return &ValidatedUserConfigBuffer;
        default:
            return NULL;
    }
}

uint16_t ConfigBufferIdToBufferSize(config_buffer_id_t configBufferId)
{
    switch (configBufferId) {
        case ConfigBufferId_HardwareConfig:
            return HARDWARE_CONFIG_SIZE;
        case ConfigBufferId_StagingUserConfig:
        case ConfigBufferId_ValidatedUserConfig:
            return USER_CONFIG_SIZE;
        default:
            return 0;
    }
}
