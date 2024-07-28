#include "attributes.h"
#include "versioning.h"

// weak definitions of generated variables to be able to build without final artifacts available
const version_t firmwareVersion ATTR_WEAK = {};
const version_t deviceProtocolVersion ATTR_WEAK = {};
const version_t moduleProtocolVersion ATTR_WEAK = {};
const version_t userConfigVersion ATTR_WEAK = {};
const version_t hardwareConfigVersion ATTR_WEAK = {};
const version_t smartMacrosVersion ATTR_WEAK = {};

const char gitRepo[] ATTR_WEAK = "";
const char gitTag[] ATTR_WEAK = "";

#ifdef DEVICE_COUNT
const char *const DeviceMD5Checksums[DEVICE_COUNT + 1] ATTR_WEAK = {
    [1] = "000000000000000000000000000000000",
    [2] = "000000000000000000000000000000000",
    [3] = "000000000000000000000000000000000",
    [4] = "000000000000000000000000000000000",
    [5] = "000000000000000000000000000000000",
};
#endif

const char *const ModuleMD5Checksums[ModuleId_AllCount] ATTR_WEAK = {
    [1] = "000000000000000000000000000000000",
    [2] = "000000000000000000000000000000000",
    [3] = "000000000000000000000000000000000",
    [4] = "000000000000000000000000000000000",
};
