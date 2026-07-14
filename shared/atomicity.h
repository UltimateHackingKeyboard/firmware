#ifndef __ATOMICITY_H__
#define __ATOMICITY_H__

#ifdef __ZEPHYR__
    #include <zephyr/irq.h>
    #include <zephyr/sys/__assert.h>
    #include <cmsis_core.h>
    #define DISABLE_IRQ() unsigned int key = irq_lock()
    #define ENABLE_IRQ() irq_unlock(key)
    // irq_lock() masks via BASEPRI on Cortex-M; PRIMASK covers __disable_irq/ISR-entry masking.
    #define ASSERT_IRQS_DISABLED() __ASSERT((__get_PRIMASK() != 0) || (__get_BASEPRI() != 0), "irqs are enabled")
#else
    #include "fsl_common.h"

    // Soft assert: reports the failure into the status buffer and continues
    // (a failed hard assert() would end in newlib's silent _exit hang).
    // Only the first failure is printed - SOFT_ASSERT may fire from hot ISR
    // paths. Defined in right/src/debug.c.
#ifdef __cplusplus
    extern "C" void SoftAssertFailed(const char* file, int line);
#else
    void SoftAssertFailed(const char* file, int line);
#endif
    #define SOFT_ASSERT(COND) do { if (!(COND)) { SoftAssertFailed(__FILE__, __LINE__); } } while (0)

    // __enable_irq() is unconditional, so nested DISABLE_IRQ sections would silently
    // re-enable irqs at the inner ENABLE_IRQ; softassert against such nesting.
    #define DISABLE_IRQ() do { SOFT_ASSERT(__get_PRIMASK() == 0); __disable_irq(); } while (0)
    #define ENABLE_IRQ() __enable_irq()
    #define ASSERT_IRQS_DISABLED() SOFT_ASSERT(__get_PRIMASK() != 0)
#endif

// Includes:

// Functions:

#endif // __ATOMICITY_H__
