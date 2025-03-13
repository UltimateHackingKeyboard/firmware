#include "wormhole.h"


#ifdef __ZEPHYR__

#include <zephyr/kernel.h>
__noinit wormhole_data_t StateWormhole;

#else

#include "attributes.h"
ATTR_NO_INIT wormhole_data_t StateWormhole;

#endif

void StateWormhole_Close() {
    StateWormhole.magicNumber = 0;
    StateWormhole.rebootToPowerMode = false;
    StateWormhole.persistStatusBuffer = false;
}

