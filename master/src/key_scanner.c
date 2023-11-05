#include <zephyr/kernel.h>

#define THREAD_STACK_SIZE 500
#include <zephyr/kernel.h>
#include "key_scanner.h"

#define THREAD_PRIORITY 5

K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
struct k_thread thread_data;

void keyScanner() {
}

void InitKeyScanner(void)
{
    k_thread_create(
        &thread_data, stack_area,
        K_THREAD_STACK_SIZEOF(stack_area),
        keyScanner,
        NULL, NULL, NULL,
        THREAD_PRIORITY, 0, K_NO_WAIT
    );
}
