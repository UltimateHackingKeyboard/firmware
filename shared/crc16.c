#include "crc16.h"

void crc16_init(crc16_data_t *crc16Config)
{
    crc16Config->currentCrc = 0;
}

void crc16_update(crc16_data_t *crc16Config, const uint8_t *src, uint32_t lengthInBytes)
{
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
    *hash = crc16Config->currentCrc;
}

crc16_data_t crc16data;

void CRC16_UpdateMessageChecksum(i2c_message_t *message)
{
    uint16_t hash;
    crc16_init(&crc16data);
    crc16_update(&crc16data, message->data, message->length);
    crc16_finalize(&crc16data, &hash);
    message->crc = hash;
}

bool CRC16_IsMessageValid(i2c_message_t *message)
{
    uint16_t hash;
    crc16_init(&crc16data);
    crc16_update(&crc16data, message->data, message->length);
    crc16_finalize(&crc16data, &hash);
    return message->crc == hash;
}
