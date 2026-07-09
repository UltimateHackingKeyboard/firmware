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
    #include <assert.h>
    #include "fsl_common.h"
    #define DISABLE_IRQ() __disable_irq()
    #define ENABLE_IRQ() __enable_irq()
    #define ASSERT_IRQS_DISABLED() assert(__get_PRIMASK() != 0)
#endif

// Includes:

// Functions:

#endif // __ATOMICITY_H__
