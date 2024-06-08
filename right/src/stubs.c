#include "stubs.h"

#ifdef __ZEPHYR__
#include "device.h"
#else
#include "device/device.h"
#endif

#if DEVICE_IS_UHK_DONGLE
#include "keyboard/state_sync.h"

void StateSync_UpdateLayer(layer_id_t layerId, bool fullUpdate) {};
void StateSync_UpdateProperty(state_sync_prop_id_t propId, void* data) {};

#endif
