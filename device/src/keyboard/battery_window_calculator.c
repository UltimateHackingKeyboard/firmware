#include "battery_window_calculator.h"
#include "timer.h"
#include <inttypes.h>

uint32_t lastMeasurement = 0;

#define WINDOW_SIZE_MS ((uint32_t)8)*60*1000
#define WINDOW_SIZE (((WINDOW_SIZE_MS)/1000) / (((uint32_t)CHARGER_UPDATE_PERIOD)/1000))

uint16_t values[WINDOW_SIZE];
uint32_t sum = 0;
uint8_t pos = 0;
uint8_t count = 0;

static bool shouldAddRecord(uint16_t voltage) {
    return CurrentTime - lastMeasurement > CHARGER_UPDATE_PERIOD/2 && voltage != 0;
}

static void addNewRecord(uint16_t voltage) {
    if (count == WINDOW_SIZE) {
        sum -= values[pos];
    }

    values[pos] = voltage;
    pos++;

    if (pos >= WINDOW_SIZE) {
        pos = 0;
    }

    sum += voltage;

    count = MIN(count + 1, WINDOW_SIZE);
    lastMeasurement = CurrentTime;
}

uint16_t BatteryCalculator_CalculateWindowAverageVoltage(uint16_t voltage) {
    if (BATTERY_CALCULATOR_AVERAGE_ENABLED) {
        // printk("BatteryCalculator_CalculateWindowAverageVoltage (%d) of (%d):", voltage, count);
        // for (uint8_t i = 0; i < count; i++) {
        //     printk(" %d", values[i]);
        // }
        // printk(" sums to %d\n", sum);
        if (shouldAddRecord(voltage)) {
            addNewRecord(voltage);
            return sum / count;
        } else {
            return (sum + voltage) / (count + 1);
        }
    } else {
        return voltage;
    }
}

