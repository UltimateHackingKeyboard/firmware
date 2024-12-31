#ifndef __BT_MANAGER_H__
#define __BT_MANAGER_H__

// Includes:

    #include <zephyr/bluetooth/bluetooth.h>
    #include <stdint.h>
    #include "device.h"

// Macros:

// Typedefs:

// Variables:

    extern bool BtManager_Restarting;

// Functions:

    void BtManager_InitBt();
    void BtManager_StartBt();
    void BtManager_StopBt();
    void BtManager_RestartBt();
    void BtManager_StartScanningAndAdvertising();

#endif // __BT_MANAGER_H__
