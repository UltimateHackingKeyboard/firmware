#ifndef __BUFFER_H__
#define __BUFFER_H__

// Includes:

    #include "fsl_common.h"

// Functions:

    uint8_t  GetBufferUint8(const uint8_t *buffer, uint32_t offset);
    uint16_t GetBufferUint16(const uint8_t *buffer, uint32_t offset);
    uint32_t GetBufferUint32(const uint8_t *buffer, uint32_t offset);

    uint8_t  GetBufferUint8Be(const uint8_t *buffer, uint32_t offset);
    uint16_t GetBufferUint16Be(const uint8_t *buffer, uint32_t offset);
    uint32_t GetBufferUint32Be(const uint8_t *buffer, uint32_t offset);

    void SetBufferUint8(uint8_t *buffer, uint32_t offset, uint8_t value);
    void SetBufferUint16(uint8_t *buffer, uint32_t offset, uint16_t value);
    void SetBufferUint32(uint8_t *buffer, uint32_t offset, uint32_t value);

    void SetBufferInt8(uint8_t *buffer, uint32_t offset, int8_t value);
    void SetBufferInt16(uint8_t *buffer, uint32_t offset, int16_t value);
    void SetBufferInt32(uint8_t *buffer, uint32_t offset, int32_t value);

    void SetBufferUint8Be(uint8_t *buffer, uint32_t offset, uint8_t value);
    void SetBufferUint16Be(uint8_t *buffer, uint32_t offset, uint16_t value);
    void SetBufferUint32Be(uint8_t *buffer, uint32_t offset, uint32_t value);

    void SetBufferFloat(uint8_t *buffer, uint32_t offset, float value);

#endif
