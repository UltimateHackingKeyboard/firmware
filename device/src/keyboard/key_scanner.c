#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/key_scanner.h"
#include "shell.h"
#include "keyboard/uart.h"
#include "nus_client.h"
#include "nus_server.h"
#include "device.h"
#include "oled/oled_buffer.h"
#include "logger.h"
#include "key_states.h"
#include "keyboard/key_layout.h"
#include "bool_array_converter.h"
#include "legacy/module.h"
#include "keyboard/logger.h"
#include "messenger.h"
#include "device.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

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

static void scanKeys() {
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
            }
        }
        gpio_pin_set_dt(&rows[rowId], 0);
    }

    KeyPressed = keyPressed;

    if (!somethingChanged) {
        return;
    }

    uint8_t compressedLength = MAX_KEY_COUNT_PER_MODULE/8+1;
    uint8_t compressedBuffer[compressedLength];
    if (DEVICE_IS_UHK80_LEFT) {
        memset(compressedBuffer, 0, compressedLength);
    }

    uint8_t slotId = DEVICE_IS_UHK80_LEFT ? SlotId_LeftKeyboardHalf : SlotId_RightKeyboardHalf;
    for (uint8_t rowId=0; rowId<KEY_MATRIX_ROWS; rowId++) {
        for (uint8_t colId=0; colId<KEY_MATRIX_COLS; colId++) {
            uint8_t sourceIndex = rowId*KEY_MATRIX_COLS + colId;
            uint8_t targetKeyId = KeyLayout_Uhk80_to_Uhk60[slotId][sourceIndex];

            if (targetKeyId < MAX_KEY_COUNT_PER_MODULE) {
                if (DEVICE_IS_UHK80_RIGHT) {
                    KeyStates[CURRENT_SLOT_ID][targetKeyId].hardwareSwitchState = keyStateBuffer[sourceIndex];
                }

                if (DEVICE_IS_UHK80_LEFT) {
                    BoolBitToBytes(keyStateBuffer[sourceIndex], targetKeyId, compressedBuffer);
                }
            }
        }
    }

    if (DEVICE_IS_UHK80_LEFT) {
        Messenger_Send(DeviceId_Uhk80_Right, SyncablePropertyId_LeftHalfKeyStates, compressedBuffer, compressedLength);
    }
}

void keyScanner() {
    while (true) {
        scanKeys();
        k_msleep(1);
    }
}

void InitKeyScanner(void)
{
    for (uint8_t rowId=0; rowId<6; rowId++) {
        gpio_pin_configure_dt(&rows[rowId], GPIO_OUTPUT);
    }
    for (uint8_t colId=0; colId<COLS_COUNT; colId++) {
        gpio_pin_configure_dt(&cols[colId], GPIO_INPUT);
    }

    k_thread_create(
            &thread_data, stack_area,
            K_THREAD_STACK_SIZEOF(stack_area),
            keyScanner,
            NULL, NULL, NULL,
            THREAD_PRIORITY, 0, K_NO_WAIT
            );
    k_thread_name_set(&thread_data, "key_scanner");
}
