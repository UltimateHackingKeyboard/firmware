#include "device.h"
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/key_scanner.h"
#include "shell.h"
#include "keyboard/uart_bridge.h"
#include "nus_client.h"
#include "nus_server.h"
#include "oled/oled_buffer.h"
#include "logger.h"
#include "key_states.h"
#include "bool_array_converter.h"
#include "module.h"
#include "logger.h"
#include "messenger.h"
#include "device.h"
#include "event_scheduler.h"
#include "main.h"
#include "config_manager.h"
#include "macros/keyid_parser.h"
#include "attributes.h"
#include "layouts/key_layout.h"
#include "layouts/key_layout_80_to_universal.h"
#include "test_switches.h"
#include "power_mode.h"
#include "keyboard/leds.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY -1

#define USE_QUICK_SCAN true

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

// Keyboard matrix definitions

static struct gpio_dt_spec rows[KEY_MATRIX_ROWS] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(row1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row3), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row4), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row5), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row6), gpios),
};

static struct gpio_dt_spec cols[KEY_MATRIX_COLS] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(col1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col3), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col4), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col5), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col6), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col7), gpios),
#if DEVICE_IS_UHK80_RIGHT
    GPIO_DT_SPEC_GET(DT_ALIAS(col8), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col9), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col10), gpios),
#endif
};

#define COLS_COUNT (sizeof(cols) / sizeof(cols[0]))
volatile bool KeyPressed;

volatile bool KeyScanner_ResendKeyStates = false;

static void scanAllKeys();

ATTR_UNUSED static void reportChange(uint8_t sourceIndex, bool active) {
    uint8_t slotId = DEVICE_IS_UHK80_LEFT ? SlotId_LeftKeyboardHalf : SlotId_RightKeyboardHalf;
    uint8_t keyId = KeyLayout_Uhk80_to_Uhk60[slotId][sourceIndex];
    const char* abbrev = MacroKeyIdParser_KeyIdToAbbreviation(slotId*64 + keyId);
    if (active) {
        Log("%s   down\n", abbrev);
    } else {
        Log("  %s up\n", abbrev);
    }
}

static bool scanKey(uint8_t rowId, uint8_t colId) {
    gpio_pin_set_dt(&rows[rowId], 1);
    bool keyState = gpio_pin_get_dt(&cols[colId]);
    gpio_pin_set_dt(&rows[rowId], 0);
    return keyState;
}

static bool isSfjlKey(uint8_t rowId, uint8_t colId) {
    if (DEVICE_IS_UHK80_LEFT) {
        return (rowId == 3 && (colId == 2 || colId == 4));
    }
    if (DEVICE_IS_UHK80_RIGHT) {
        return (rowId == 3 && (colId == 1 || colId == 3));
    }
    return false;
}

static sfjl_scan_result_t scanSfjl(bool fullScan) {
    bool somethingPressed = false;
    bool success = true;

#define CHECK(EXPECTED, EXPR) if (EXPR) { success &= EXPECTED; somethingPressed = true; } else { success &= !EXPECTED; }

    if (DEVICE_IS_UHK80_LEFT) {
        CHECK(true, scanKey(3, 2));
        CHECK(true, scanKey(3, 4));
    }

    if (DEVICE_IS_UHK80_RIGHT) {
        CHECK(true, scanKey(3, 1));
        CHECK(true, scanKey(3, 3));
    }

    for (uint8_t rowId=0; rowId<KEY_MATRIX_ROWS; rowId++) {
        if (!success && !fullScan) {
            break;
        }

        gpio_pin_set_dt(&rows[rowId], 1);
        for (uint8_t colId=0; colId<KEY_MATRIX_COLS; colId++) {
            bool keyState = gpio_pin_get_dt(&cols[colId]);
            bool isSfjl = isSfjlKey(rowId, colId);
            CHECK(isSfjl, keyState);
        }
        gpio_pin_set_dt(&rows[rowId], 0);
    }

    sfjl_scan_result_t res = success ? SfjlScanResult_FullMatch : (somethingPressed ? SfjlScanResult_SomethingPressed : SfjlScanResult_NonePressed);
    return res;
}

static sfjl_scan_result_t scanKeysOnce(sfjl_scan_result_t defaultResult, bool fullScan) {
    // In the lock mode, do both scans
    if (CurrentPowerMode < PowerMode_SfjlSleep) {
        scanAllKeys();
    }

    if (CurrentPowerMode > PowerMode_LightSleep) {
        defaultResult = MAX(defaultResult, scanSfjl(fullScan));
    }
    return defaultResult;
}

static bool scanSfjlWithBlinking(bool fullScan) {
    const uint16_t blinkCount = 3;
    const uint16_t blinktimeOn = 100;
    const uint16_t blinktimeOff = 200;
    const uint16_t minPressLength = 0;
    const uint16_t scanInterval = PowerModeConfig[CurrentPowerMode].keyScanInterval;

    sfjl_scan_result_t result = SfjlScanResult_NonePressed;

    result = scanKeysOnce(result, fullScan);

    if (result == SfjlScanResult_NonePressed || !fullScan) {
        return result == SfjlScanResult_FullMatch;
    }

    for (uint16_t i=0; i<blinkCount; i++) {
        Leds_BlinkSfjl(blinktimeOn);

        result = scanKeysOnce(result, fullScan);

        for (uint16_t time = 0; time < blinktimeOff && i < blinkCount-1; time += scanInterval) {
            k_msleep(scanInterval);
            result = scanKeysOnce(result, fullScan);
        }

        if ( CurrentPowerMode < PowerMode_LightSleep) {
            return true;
        }

        if (result == SfjlScanResult_FullMatch) {
            for (uint16_t j = 0; j < minPressLength/scanInterval; j++) {
                result = scanKeysOnce(result, fullScan);
                if (result != SfjlScanResult_FullMatch) {
                    break;
                }

                if ( CurrentPowerMode < PowerMode_LightSleep) {
                    return true;
                }
                k_msleep(scanInterval);
            }
            if (result == SfjlScanResult_FullMatch) {
                return true;
            }
        }
    }

    return false;
}

// This doesn't seem to decrease power consumption much, bu no reason not to use it.
static bool quickScan() {
    bool someKeyPressed = false;
    for (uint8_t rowId=0; rowId<KEY_MATRIX_ROWS; rowId++) {
        gpio_pin_set_dt(&rows[rowId], 1);
    }
    for (uint8_t colId=0; colId<KEY_MATRIX_COLS; colId++) {
        someKeyPressed |= gpio_pin_get_dt(&cols[colId]);
    }
    for (uint8_t rowId=0; rowId<KEY_MATRIX_ROWS; rowId++) {
        gpio_pin_set_dt(&rows[rowId], 0);
    }
    return someKeyPressed;
}

static void scanAllKeys() {
    bool somethingChanged = false;
    static bool keyStateBuffer[KEY_MATRIX_ROWS*KEY_MATRIX_COLS];
    bool keyPressed = false;


    for (uint8_t rowId=0; rowId<KEY_MATRIX_ROWS; rowId++) {
        gpio_pin_set_dt(&rows[rowId], 1);
        for (uint8_t colId=0; colId<KEY_MATRIX_COLS; colId++) {
            bool keyState = gpio_pin_get_dt(&cols[colId]);

            uint8_t targetIndex = rowId*KEY_MATRIX_COLS + colId;

            keyPressed |= keyState;

            if (keyStateBuffer[targetIndex] != keyState) {
                somethingChanged = true;
                keyStateBuffer[targetIndex] = keyState;
                // reportChange(targetIndex, keyState);
            }
        }
        gpio_pin_set_dt(&rows[rowId], 0);
    }

    KeyPressed = keyPressed;

    if (!somethingChanged && !KeyScanner_ResendKeyStates) {
        return;
    }

    KeyScanner_ResendKeyStates = false;

    uint8_t compressedLength = MAX_KEY_COUNT_PER_MODULE/8+1;
    uint8_t compressedBuffer[compressedLength];
    if (DEVICE_IS_UHK80_LEFT) {
        memset(compressedBuffer, 0, compressedLength);
    }

    uint8_t slotId = DEVICE_IS_UHK80_LEFT ? SlotId_LeftKeyboardHalf : SlotId_RightKeyboardHalf;
    for (uint8_t rowId=0; rowId<KEY_MATRIX_ROWS; rowId++) {
        for (uint8_t colId=0; colId<KEY_MATRIX_COLS; colId++) {
            uint8_t sourceIndex = rowId*KEY_MATRIX_COLS + colId;
            uint8_t targetKeyId;

            if (DataModelVersion.major >= 8) {
                targetKeyId = KeyLayout_Uhk80_to_Universal[slotId][sourceIndex];
            } else {
                targetKeyId = KeyLayout_Uhk80_to_Uhk60[slotId][sourceIndex];
            }

            if (targetKeyId < MAX_KEY_COUNT_PER_MODULE) {
                if (TestSwitches) {
                    if ( keyStateBuffer[sourceIndex] ) {
                        Ledmap_ActivateTestled(slotId, targetKeyId);
                        EventVector_WakeMain();
                    }
                    KeyStates[CURRENT_SLOT_ID][targetKeyId].hardwareSwitchState = false;
                    continue;
                }

                if (DEVICE_IS_UHK80_RIGHT) {
                    KeyStates[CURRENT_SLOT_ID][targetKeyId].hardwareSwitchState = keyStateBuffer[sourceIndex];
                }

                if (DEVICE_IS_UHK80_LEFT) {
                    BoolBitToBytes(keyStateBuffer[sourceIndex], targetKeyId, compressedBuffer);
                }
            }
        }
    }

    if (DEVICE_IS_UHK80_RIGHT) {
        EventVector_Set(EventVector_StateMatrix);
        EventVector_WakeMain();
    }

    if (DEVICE_IS_UHK80_LEFT) {
        Messenger_Send2(DeviceId_Uhk80_Right, MessageId_SyncableProperty, SyncablePropertyId_LeftHalfKeyStates, compressedBuffer, compressedLength);
    }
}

bool KeyScanner_ScanAndWakeOnSfjl(bool fullScan, bool wake) {
    if (scanSfjlWithBlinking(true)) {
        if (wake) {
            PowerMode_ActivateMode(PowerMode_Awake, false, false, "key scanner wake");
            EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
        }
        return true;
    }
    return false;
}

static void scanKeys() {
    if (CurrentPowerMode > PowerMode_LightSleep) {
        KeyScanner_ScanAndWakeOnSfjl(true, true);
    } else if (!USE_QUICK_SCAN || KeyPressed || quickScan()) {
        scanAllKeys();
    }
}

void keyScanner() {
    while (true) {
        scanKeys();
        k_msleep(PowerModeConfig[CurrentPowerMode].keyScanInterval);
    }
}

void InitKeyScanner_Min(void) {
    for (uint8_t rowId=0; rowId<6; rowId++) {
        gpio_pin_configure_dt(&rows[rowId], GPIO_OUTPUT);
    }
    for (uint8_t colId=0; colId<COLS_COUNT; colId++) {
        gpio_pin_configure_dt(&cols[colId], GPIO_INPUT);
    }
}

void InitKeyScanner(void)
{
    InitKeyScanner_Min();

    k_thread_create(
            &thread_data, stack_area,
            K_THREAD_STACK_SIZEOF(stack_area),
            keyScanner,
            NULL, NULL, NULL,
            THREAD_PRIORITY, 0, K_NO_WAIT
            );
    k_thread_name_set(&thread_data, "key_scanner");
}


