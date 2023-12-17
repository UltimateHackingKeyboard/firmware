#include <zephyr/bluetooth/addr.h>
#include <zephyr/settings/settings.h>

static int peerAddressSet(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    static char foo_val[BT_ADDR_SIZE];
    printk("Address '%s' is ", name);
    read_cb(cb_arg, &foo_val, len);
    for (uint8_t i=0; i<BT_ADDR_SIZE; i++) {
        printk("%02X ", foo_val[i]);
    }
    printk("\n");
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
