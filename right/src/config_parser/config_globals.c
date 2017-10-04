#include "config_globals.h"
#include "attributes.h"

static uint8_t hardwareConfig[HARDWARE_CONFIG_SIZE];
static uint8_t ATTR_DATA2 stagingUserConfig[USER_CONFIG_SIZE];
static uint8_t validatedUserConfig[USER_CONFIG_SIZE];

config_buffer_t HardwareConfigBuffer = { hardwareConfig };
config_buffer_t StagingUserConfigBuffer = { stagingUserConfig };
config_buffer_t ValidatedUserConfigBuffer = { validatedUserConfig };

bool ParserRunDry;
