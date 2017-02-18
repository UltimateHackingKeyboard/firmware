#ifndef __INIT_H__
#define __INIT_H__

#include "fsl_common.h"

// Functions:

    void InitPeripherials();
    void restartI2C();
    uint32_t I2C_Watchdog = 0;
#endif
