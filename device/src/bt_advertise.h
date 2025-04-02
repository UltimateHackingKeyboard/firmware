#ifndef __BT_ADVERTISE_H__
#define __BT_ADVERTISE_H__

// Includes:

    #include <stdint.h>
    #include <zephyr/bluetooth/addr.h>
    #include "bt_pair.h"
    #include "shared/attributes.h"

// Macros:

    #define ADVERTISE_NUS (1 << 0)
    #define ADVERTISE_HID (1 << 1)
    #define ADVERTISE_DIRECTED_NUS (1 << 2)

// Typedefs:

    typedef struct {
        uint8_t advType;
        bt_addr_le_t* addr;
    } ATTR_PACKED adv_config_t;

// Variables:

    extern pairing_mode_t AdvertisingHid;

// Functions:

    void BtAdvertise_DisableAdvertisingIcon(void);
    uint8_t BtAdvertise_Start(adv_config_t advConfig);
    void BtAdvertise_Stop(void);
    adv_config_t BtAdvertise_Config();

#endif // __BT_ADVERTISE_H__
