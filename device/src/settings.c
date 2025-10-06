#include "settings.h"
#include <zephyr/bluetooth/addr.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/device.h>
#include <string.h>
#include <stdio.h>
#include "bt_conn.h"
#include "dongle_leds.h"
#include "stubs.h"

#if DEVICE_IS_KEYBOARD
#include "keyboard/battery_manager.h"
#endif

bool RightAddressIsSet = false;

uint16_t Settings_MaxVoltage = 0;
uart_debug_mode_t Settings_UartDebugMode = UartDebugMode_I2CMode;

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

static int other(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    if (strcmp(name, "maxVoltage") == 0) {
#if DEVICE_IS_KEYBOARD
        read_cb(cb_arg, &Settings_MaxVoltage, sizeof(Settings_MaxVoltage));
        BatteryManager_SetMaxCharge(Settings_MaxVoltage);
#endif
    }
    return 0;
}

static int uartDebug(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    if (strcmp(name, "mode") == 0) {
#if DEVICE_IS_KEYBOARD
        read_cb(cb_arg, &Settings_UartDebugMode, sizeof(Settings_UartDebugMode));
#endif
    }
    return 0;
}


struct settings_handler settingsHandler = {
    .name = "uhk/addr",
    .h_set = peerAddressSet,
};

struct settings_handler otherHandler = {
    .name = "uhk/other",
    .h_set = other,
};

struct settings_handler uartDebugHandler = {
    .name = "uhk/uartDebug",
    .h_set = uartDebug,
};

void InitSettings(void) {
    DongleLeds_Update();
    settings_subsys_init();
    settings_register(&settingsHandler);
    settings_register(&otherHandler);
    settings_load();
}

void Settings_Reload(void) {
    setRightAddressIsSet(false);
    settings_load();
}

static bool shouldDeleteKey(const char *key) {
    const char *protectedKey = NULL;
    const bt_addr_le_t *protectedAddr = NULL;
#if DEVICE_IS_UHK80_LEFT
    protectedKey = "uhk/addr/right";
    protectedAddr = &Peers[PeerIdRight].addr;
#elif DEVICE_IS_UHK80_RIGHT
    protectedKey = "uhk/addr/left";
    protectedAddr = &Peers[PeerIdLeft].addr;
#endif

    if (protectedAddr == NULL || protectedKey == NULL) {
        return true;
    }

    if (strcmp(key, protectedKey) == 0) {
        return false;
    }

    // 2*6 bytes + type flag + null terminator
    const uint8_t addrStrLen = 14;
    char addrString[addrStrLen];

    // print addr to string without colons in hex using sprintf
    for (uint8_t i = 0; i < BLE_ADDR_LEN; i++) {
        sprintf(&addrString[i*2], "%02x", protectedAddr->a.val[BLE_ADDR_LEN-1-i]);
    }
    addrString[addrStrLen-2] = protectedAddr->type ? '1' : '0';
    addrString[addrStrLen-1] = '\0';

    const char* keyAddr = key + strlen(key) + 1 - addrStrLen;

    bool keyIsProtected = strncmp(keyAddr, addrString, addrStrLen) == 0;

    printk("Matched key: %s, addrString: %s ? %d\n", keyAddr, addrString, keyIsProtected);

    return !keyIsProtected;
}

static int delete_handler(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param) {
    if (shouldDeleteKey(key)) {
        settings_delete(key);
    }
    return 0;
}

void Settings_Erase(const char* reason) {
    printk("Settings: Erasing all settings, because: %s\n", reason);

    settings_load();
    settings_load_subtree_direct(NULL, delete_handler, NULL);
    settings_save();

    Settings_Reload();
}
