#include "config_globals.h"
#include "attributes.h"

static uint8_t hardwareConfig[HARDWARE_CONFIG_SIZE];
config_buffer_t HardwareConfigBuffer = {hardwareConfig};

static uint8_t userConfig1[USER_CONFIG_SIZE];
static uint8_t ATTR_DATA2 userConfig2[USER_CONFIG_SIZE];
config_buffer_t ValidatedUserConfigBuffer = { userConfig1 };
config_buffer_t StagingUserConfigBuffer = { userConfig2 };

bool ParserRunDry;
