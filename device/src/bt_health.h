#ifndef __BT_HEALTH_H__
#define __BT_HEALTH_H__

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include "right/src/bt_defs.h"

void Bt_HealthCheck(const char* reason);
void Bt_HandleError(const char* context, int err);

#endif // __BT_HEALTH_H__ 