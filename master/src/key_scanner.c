#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "key_scanner.h"
#include "shell.h"
#include "uart.h"

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
#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT
    GPIO_DT_SPEC_GET(DT_ALIAS(col8), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col9), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col10), gpios),
#endif
};

#define COLS_COUNT (sizeof(cols) / sizeof(cols[0]))
uint8_t KeyStates[KEY_MATRIX_ROWS][KEY_MATRIX_COLS];
volatile char KeyPressed;

void keyScanner() {
    while (true) {
        KeyPressed = false;
        for (uint8_t rowId=0; rowId<KEY_MATRIX_ROWS; rowId++) {
            gpio_pin_set_dt(&rows[rowId], 1);
            for (uint8_t colId=0; colId<KEY_MATRIX_COLS; colId++) {
                bool keyState = gpio_pin_get_dt(&cols[colId]);
                KeyStates[rowId][colId] = keyState;
                if (keyState) {
                    KeyPressed = true;
                    if (Shell.keyLog) {
                        char buffer[20];
                        sprintf(buffer, "SW%c%c\n", rowId+'1', colId+'1');
                        printk("%s", buffer);
                        for (uint8_t i=0; i<strlen(buffer); i++) {
                            uart_poll_out(uart_dev, buffer[i]);
                        }
                    }
                }
            }
            gpio_pin_set_dt(&rows[rowId], 0);
        }

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
}
