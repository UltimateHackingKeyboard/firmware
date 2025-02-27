#include "wormhole.h"


#ifdef __ZEPHYR__

#include <zephyr/kernel.h>
__noinit wormhole_data_t Wormhole;

#else

#include "attributes.h"
ATTR_NO_INIT wormhole_data_t Wormhole;

#endif

