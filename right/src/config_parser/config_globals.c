#include "config_globals.h"

static uint8_t hardwareConfig[HARDWARE_CONFIG_SIZE];
config_buffer_t HardwareConfigBuffer = {hardwareConfig};

static uint8_t userConfig1[USER_CONFIG_SIZE];
static uint8_t __attribute__((section (".m_data_2"))) userConfig2[USER_CONFIG_SIZE];
config_buffer_t UserConfigBuffer = { userConfig1 };
config_buffer_t StagingUserConfigBuffer = { userConfig2 };

bool ParserRunDry;
