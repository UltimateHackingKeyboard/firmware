#include "fsl_rtos_abstraction.h"

static lock_object_t lockObject;

void sync_init(sync_object_t *obj, bool state)
{
    *obj = state;
}

bool sync_wait(sync_object_t *obj, uint32_t timeout)
{
    // Increment the object so we can tell if it changes. Because the increment is not
    // atomic (load, add, store), we must disabled interrupts during it.
    __disable_irq();
    ++(*obj);
    __enable_irq();

    // Wait for the object to be unlocked.
    while (*obj != 0)
    {
        // Spin.
    }

    return true;
}

void sync_signal(sync_object_t *obj)
{
    // Atomically unlock the object.
    __disable_irq();
    --(*obj);
    __enable_irq();
}

void sync_reset(sync_object_t *obj)
{
    __disable_irq();
    (*obj) = 0;
    __enable_irq();
}

void lock_init(void)
{
    __disable_irq();
    lockObject = 0;
    __enable_irq();
}

void lock_acquire(void)
{
    // Disable global IRQ until lock_release() is called.
    __disable_irq();
    ++lockObject;
}

void lock_release(void)
{
    // Restore previous state, enable global IRQ if all locks released.
    if (lockObject <= 1)
    {
        lockObject = 0;
        __enable_irq();
    }
    else
    {
        --lockObject;
    }
}
