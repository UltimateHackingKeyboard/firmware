#include "power.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include "legacy/debug.h"
#include "legacy/slave_drivers/is31fl3xxx_driver.h"

bool RunningOnBattery = false;

void Power_ReportPowerState(uint8_t level, uint32_t ma) {
    bool newState = level == 3;
    if (newState != RunningOnBattery) {
        RunningOnBattery = newState;
        LedSlaveDriver_UpdateLeds();
    }
}

