#ifndef __NUS_SERVER_H__
#define __NUS_SERVER_H__

// Includes:
//
    #include "link_protocol.h"

// Functions:

    extern void NusServer_Init(void);
    extern void NusServer_Send(const uint8_t *data, uint16_t len);
    extern void NusServer_SendSyncableProperty(syncable_property_id_t property, const uint8_t *data, uint16_t len);

#endif // __NUS_SERVER_H__
