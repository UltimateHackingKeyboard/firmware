#ifndef __CONFIG_GLOBALS_H__
#define __CONFIG_GLOBALS_H__

// Includes:

    #include "fsl_common.h"
    #include "attributes.h"
    #include "basic_types.h"

// Macros:

    #define HARDWARE_CONFIG_SIGNATURE_LENGTH 3

// Typedefs:

    typedef enum {
        ConfigBufferId_HardwareConfig,
        ConfigBufferId_StagingUserConfig,
        ConfigBufferId_ValidatedUserConfig,
    } config_buffer_id_t;

    typedef struct {
        uint8_t signatureLength;
        char signature[HARDWARE_CONFIG_SIGNATURE_LENGTH];
        uint8_t majorVersion;
        uint8_t minorVersion;
        uint8_t patchVersion;
        uint8_t brandId;
        uint8_t deviceId;
        uint32_t uniqueId;
        bool isVendorModeOn;
        bool isIso;
    } ATTR_PACKED hardware_config_t;

// Variables:

    extern bool ParserRunDry;
    extern uint16_t ValidatedUserConfigLength;
    extern config_buffer_t HardwareConfigBuffer;
    extern config_buffer_t StagingUserConfigBuffer;
    extern config_buffer_t ValidatedUserConfigBuffer;
    extern hardware_config_t *HardwareConfig;

// Functions:

    bool IsConfigBufferIdValid(config_buffer_id_t configBufferId);
    config_buffer_t* ConfigBufferIdToConfigBuffer(config_buffer_id_t configBufferId);
    uint16_t ConfigBufferIdToBufferSize(config_buffer_id_t configBufferId);

#endif
