#ifndef __FSL_RTOS_ABSTRACTION_H__
#define __FSL_RTOS_ABSTRACTION_H__

#include <fsl_os_abstraction.h>

// Interrupt synchronization object.
typedef volatile int32_t sync_object_t;

// Interrupt lock object.
typedef volatile uint32_t lock_object_t;

enum sync_timeouts
{
    kSyncWaitForever = 0xffffffffU // Constant to pass for the sync_wait() timeout in order to wait indefinitely.
};

/*!
 * @brief Initialize a synchronization object to a given state.
 *
 * @param obj The sync object to initialize.
 * @param state The initial state of the object. Pass true to make the sync object start
 *      out locked, or false to make it unlocked.
 */
void sync_init(sync_object_t *obj, bool state);

/*!
 * @brief Wait for a synchronization object to be signalled.
 *
 * @param obj The synchronization object.
 * @param timeout The maximum number of milliseconds to wait for the object to be signalled.
 *      Pass the #kSyncWaitForever constant to wait indefinitely for someone to signal the object.
 *      If 0 is passed for this timeout, then the function will return immediately if the object
 *      is locked.
 *
 * @retval true The object was signalled.
 * @retval false A timeout occurred.
 */
bool sync_wait(sync_object_t *obj, uint32_t timeout);

/*!
 * @brief Signal for someone waiting on the syncronization object to wake up.
 *
 * @param obj The synchronization object to signal.
 */
void sync_signal(sync_object_t *obj);

/*!
 * @brief Reset the synchronization object
 *
 * @param obj The synchronization object to signal.
 */
void sync_reset(sync_object_t *obj);

//! @brief Initialize the lock object
void lock_init(void);

//! @brief Disable global irq and store previous state.
void lock_acquire(void);

//! @brief Restore previous state.
void lock_release(void);

#endif
