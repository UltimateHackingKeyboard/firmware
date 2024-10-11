#include <zephyr/bluetooth/addr.h>
#include <zephyr/settings/settings.h>
#include "bt_conn.h"
#include "dongle_leds.h"
#include "stubs.h"

bool RightAddressIsSet = false;

static int peerAddressSet(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    static char foo_val[BT_ADDR_SIZE];
    read_cb(cb_arg, &foo_val, len);

    bool rightAddressIsSet = false;

    for (uint8_t i=0; i<PeerCount; i++) {
        if (strcmp(name, Peers[i].name) == 0) {
            printk("Settings: Found peer '%s' with address ", name);
            bt_addr_le_t *addr = &Peers[i].addr;
            addr->type = BT_ADDR_LE_RANDOM;
            for (uint8_t j=0; j<BT_ADDR_SIZE; j++) {
                addr->a.val[j] = foo_val[BT_ADDR_SIZE-1-j];
                printk("%02x", addr->a.val[j]);
            }
            printk("\n");
            if (i == PeerIdRight) {
                rightAddressIsSet = true;
            }
            break;
        }
    }

    if (rightAddressIsSet != RightAddressIsSet) {
        RightAddressIsSet = rightAddressIsSet;
        DongleLeds_Update();
    }

    return 0;
}

struct settings_handler settingsHandler = {
    .name = "uhk/addr",
    .h_set = peerAddressSet,
};

void InitSettings(void)
{
    settings_subsys_init();
    settings_register(&settingsHandler);
    settings_load();
}
