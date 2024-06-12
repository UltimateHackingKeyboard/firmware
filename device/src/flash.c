#include "flash.h"
#include "legacy/storage.h"
#include <zephyr/kernel.h>

const struct flash_area *hardwareConfigArea;
const struct flash_area *userConfigArea;

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY -1

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;
static k_tid_t threadId = 0;

typedef struct {
    storage_operation_t operation;
    config_buffer_id_t configBufferId;
    void (*successCallback)(void);
} command_t;

command_t currentCommand = {};

struct k_mutex isBusyMutex;


uint8_t Flash_LaunchTransfer(storage_operation_t operation, config_buffer_id_t configBufferId, void (*successCallback))
{
    if (k_mutex_lock(&isBusyMutex, K_NO_WAIT) == 0) {
        currentCommand.operation = operation;
        currentCommand.configBufferId = configBufferId;
        currentCommand.successCallback = successCallback;
        k_wakeup(threadId);
        return 0;
    } else {
        return 1;
    }
}

int Flash_ReadAreaSync(const struct flash_area *fa, off_t off, void *dst, size_t len) {
    k_mutex_lock(&isBusyMutex, K_FOREVER);
    int res = flash_area_read(fa, off, dst, len);
    k_mutex_unlock(&isBusyMutex);
    return res;
}

bool Flash_IsBusy() {
    if (k_mutex_lock(&isBusyMutex, K_NO_WAIT) == 0) {
        k_mutex_unlock(&isBusyMutex);
        return false;
    } else {
        return true;
    }
}

static void executeCommand() {
    const struct flash_area *configArea = currentCommand.configBufferId == ConfigBufferId_HardwareConfig ? hardwareConfigArea : userConfigArea;
    size_t configSize = currentCommand.configBufferId == ConfigBufferId_HardwareConfig ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE;

    if (currentCommand.operation == StorageOperation_Read) {
        flash_area_read(configArea, 0, ConfigBufferIdToConfigBuffer(currentCommand.configBufferId)->buffer, configSize);
    } else if (currentCommand.operation == StorageOperation_Write) {
        flash_area_erase(configArea, 0, configSize);
        flash_area_write(configArea, 0, ConfigBufferIdToConfigBuffer(currentCommand.configBufferId)->buffer, configSize);
    }
    if (currentCommand.successCallback) {
        currentCommand.successCallback();
    }
}

void flash() {
    while (true) {
        if (k_mutex_lock(&isBusyMutex, K_NO_WAIT) != 0) {
            executeCommand();
        }
        k_mutex_unlock(&isBusyMutex);
        k_sleep(K_FOREVER);
    }
}

void InitFlash() {
    k_mutex_init(&isBusyMutex);

    threadId = k_thread_create(
            &thread_data, stack_area,
            K_THREAD_STACK_SIZEOF(stack_area),
            flash,
            NULL, NULL, NULL,
            THREAD_PRIORITY, 0, K_NO_WAIT
            );
    k_thread_name_set(&thread_data, "flash");
}
