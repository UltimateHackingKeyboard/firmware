#include "wormhole.h"
#include "versioning.h"

#define WORMHOLE_MAGIC_NUMBER (0x3b04cd9e94521f9a ^ 0xdeadbeef ^ *((uint32_t*)DeviceMD5Checksums[DEVICE_ID]))

#ifdef __ZEPHYR__

#include <zephyr/kernel.h>
__noinit wormhole_data_t StateWormhole;

#else

#include "attributes.h"
ATTR_DATA_NO_INIT wormhole_data_t StateWormhole;
#endif

bool StateWormhole_IsOpen(void) {
    return (StateWormhole.magicNumber == WORMHOLE_MAGIC_NUMBER);
}

void StateWormhole_Open(void) {
    StateWormhole.magicNumber = WORMHOLE_MAGIC_NUMBER;
}

void StateWormhole_Clean(void) {
    StateWormhole.rebootToPowerMode = false;
    StateWormhole.persistStatusBuffer = false;
    StateWormhole.wasReboot = false;
}

void StateWormhole_Close(void) {
    StateWormhole.magicNumber = 0;
    StateWormhole_Clean();
}


