#ifndef __CRC16_H__
#define __CRC16_H__

#include <stdint.h>
#include <stdbool.h>
#include "slave_protocol.h"

#define CRC16_HASH_LENGTH 2 // bytes

typedef struct Crc16Data {
    uint16_t currentCrc;
} crc16_data_t;

//! @brief Initializes the parameters of the crc function, must be called first.
//! @param crc16Config Instantiation of the data structure of type crc16_data_t.
void crc16_init(crc16_data_t *crc16Config);

//! @brief A "running" crc calculator that updates the crc value after each call.
//! @param crc16Config Instantiation of the data structure of type crc16_data_t.
//! @param src Pointer to the source buffer of data.
//! @param lengthInBytes The length, given in bytes (not words or long-words).
void crc16_update(crc16_data_t *crc16Config, const uint8_t *src, uint32_t lengthInBytes);

//! @brief Calculates the final crc value, padding with zeros if necessary, must be called last.
//! @param crc16Config Instantiation of the data structure of type crc16_data_t.
//! @param hash Pointer to the value returned for the final calculated crc value.
void crc16_finalize(crc16_data_t *crc16Config, uint16_t *hash);

void CRC16_UpdateMessageChecksum(i2c_message_t *message);
bool CRC16_IsMessageValid(i2c_message_t *message);

#endif
