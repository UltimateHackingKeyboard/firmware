#include <zephyr/bluetooth/addr.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/device.h>
#include "bt_conn.h"
#include "dongle_leds.h"
#include "stubs.h"

bool RightAddressIsSet = false;

static void setRightAddressIsSet(bool isSet) {
    if (RightAddressIsSet != isSet) {
        RightAddressIsSet = isSet;
        DongleLeds_Update();
    }
}

// (This isn't getting called at all when there is no "uhk/addr" settings present.)
static int peerAddressSet(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    static char foo_val[BT_ADDR_SIZE];
    read_cb(cb_arg, &foo_val, len);

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
                setRightAddressIsSet(true);
            }
            break;
        }
    }

    return 0;
}

struct settings_handler settingsHandler = {
    .name = "uhk/addr",
    .h_set = peerAddressSet,
};

void InitSettings(void) {
    settings_subsys_init();
    settings_register(&settingsHandler);
    settings_load();
}

void Settings_Reload(void) {
    setRightAddressIsSet(false);
    settings_load();
}

static int delete_handler(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param) {
    settings_delete(key);
    return 0;
}

void Settings_Erase(void) {
    settings_load();
    settings_load_subtree_direct(NULL, delete_handler, NULL);
    settings_save();

    printk("Settings: Erased all settings\n");

    Settings_Reload();
}
