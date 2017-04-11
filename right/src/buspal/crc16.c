#include "bootloader_common.h"
#include "crc16.h"

#if !defined(BOOTLOADER_HOST)
#include "fsl_device_registers.h"
#include "utilities/fsl_rtos_abstraction.h"
#endif // !BOOTLOADER_HOST

#if !defined(BOOTLOADER_HOST) && FSL_FEATURE_SOC_CRC_COUNT && !defined(BL_TARGET_RAM)
#include "fsl_crc.h"

/* Table of base addresses for crc instances. */
static CRC_Type *const g_crcBase[1] = CRC_BASE_PTRS;

void crc16_init(crc16_data_t *crc16Config)
{
    assert(crc16Config);

    crc16Config->currentCrc = 0x0000U;
}

void crc16_update(crc16_data_t *crc16Config, const uint8_t *src, uint32_t lengthInBytes)
{
    assert(crc16Config);
    assert(src);

    crc_config_t crcUserConfigPtr;

    CRC_GetDefaultConfig(&crcUserConfigPtr);

    crcUserConfigPtr.crcBits = kCrcBits16;
    crcUserConfigPtr.seed = crc16Config->currentCrc;
    crcUserConfigPtr.polynomial = 0x1021U;
    crcUserConfigPtr.complementChecksum = false;
    crcUserConfigPtr.reflectIn = false;
    crcUserConfigPtr.reflectOut = false;

    // Init CRC module and then run it
    //! Note: We must init CRC module here, As we may seperate one crc calculation into several times
    //! Note: It is better to use lock to ensure the integrity of current updating operation of crc calculation
    //        in case crc module is shared by multiple crc updating requests at the same time
    if (lengthInBytes)
    {
        lock_acquire();
        CRC_Init(g_crcBase[0], &crcUserConfigPtr);
        CRC_WriteData(g_crcBase[0], src, lengthInBytes);
        crcUserConfigPtr.seed = CRC_Get16bitResult(g_crcBase[0]);
        lock_release();
    }

    crc16Config->currentCrc = crcUserConfigPtr.seed;
}

void crc16_finalize(crc16_data_t *crc16Config, uint16_t *hash)
{
    assert(crc16Config);
    assert(hash);

    *hash = crc16Config->currentCrc;

    // De-init CRC module when we complete a full crc calculation
    CRC_Deinit(g_crcBase[0]);
}
#else
////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////
void crc16_init(crc16_data_t *crc16Config)
{
    assert(crc16Config);

    // initialize running crc and byte count
    crc16Config->currentCrc = 0;
}

void crc16_update(crc16_data_t *crc16Config, const uint8_t *src, uint32_t lengthInBytes)
{
    assert(crc16Config);
    assert(src);

    uint32_t crc = crc16Config->currentCrc;

    uint32_t j;
    for (j = 0; j < lengthInBytes; ++j)
    {
        uint32_t i;
        uint32_t byte = src[j];
        crc ^= byte << 8;
        for (i = 0; i < 8; ++i)
        {
            uint32_t temp = crc << 1;
            if (crc & 0x8000)
            {
                temp ^= 0x1021;
            }
            crc = temp;
        }
    }

    crc16Config->currentCrc = crc;
}

void crc16_finalize(crc16_data_t *crc16Config, uint16_t *hash)
{
    assert(crc16Config);
    assert(hash);

    *hash = crc16Config->currentCrc;
}
#endif
