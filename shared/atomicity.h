#ifndef __ATOMICITY_H__
#define __ATOMICITY_H__

#ifdef __ZEPHYR__
    #include <zephyr/irq.h>
    #define DISABLE_IRQ() unsigned int key = irq_lock()
    #define ENABLE_IRQ() irq_unlock(key)
#else
    #include "fsl_common.h"
    #define DISABLE_IRQ() __disable_irq()
    #define ENABLE_IRQ() __enable_irq()
#endif

// Includes:

// Functions:

#endif // __ATOMICITY_H__
