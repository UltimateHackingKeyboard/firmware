#include "battery_percent_calculator.h"
#include <stdint.h>
#include "battery_manager.h"
#include "device.h"

typedef struct {
    uint16_t unit;
    uint16_t voltage;
} life_reference_record_t;

const life_reference_record_t LeftRecords [] = {
    { .unit = 0, .voltage = 4022 },
    { .unit = 18, .voltage = 3998 },
    { .unit = 58, .voltage = 3950 },
    { .unit = 98, .voltage = 3859 },
    { .unit = 138, .voltage = 3818 },
    { .unit = 178, .voltage = 3771 },
    { .unit = 218, .voltage = 3629 },
    { .unit = 258, .voltage = 3496 },
    { .unit = 278, .voltage = 3420 },
    { .unit = 288, .voltage = 3317 },
    { .unit = 298, .voltage = 3168 },
};

const life_reference_record_t RightRecords[] = {
    { .unit = 0, .voltage = 4039 },
    { .unit = 18, .voltage = 3974 },
    { .unit = 58, .voltage = 3863 },
    { .unit = 98, .voltage = 3770 },
    { .unit = 138, .voltage = 3682 },
    { .unit = 178, .voltage = 3645 },
    { .unit = 218, .voltage = 3611 },
    { .unit = 258, .voltage = 3587 },
    { .unit = 298, .voltage = 3548 },
    { .unit = 338, .voltage = 3500 },
    { .unit = 378, .voltage = 3438 },
    { .unit = 398, .voltage = 3415 },
    { .unit = 408, .voltage = 3392 },
    { .unit = 428, .voltage = 3230 },
    { .unit = 438, .voltage = 3065 },
};

static uint16_t uninterpolate(
    uint16_t outLeft, uint16_t outRight,
    uint16_t inLeft, uint16_t in, uint16_t inRight,
    bool coalesce
) {
    uint16_t inRange = inRight - inLeft;
    uint16_t outRange = outRight - outLeft;

    if (inRange == 0 || outRange == 0) {
        return (outLeft + outRight) / 2;
    }

    float pos = (float)(in - inLeft) / (float)(inRight - inLeft);
    float _out = (float)outLeft + pos * ((float)outRight - (float)outLeft);
    int16_t out = (uint16_t)_out;

    if (coalesce) {
        if (out < outLeft && outLeft <= outRight) {
            return outLeft;
        }

        if (out > outLeft && outLeft >= outRight) {
            return outRight;
        }

        if (out < outRight && outRight <= outLeft) {
            return outLeft;
        }

        if (out > outRight && outRight >= outLeft) {
            return outRight;
        }
    }
    return out;
}

static uint16_t findMinuteOf(uint16_t voltage, const life_reference_record_t* records, uint16_t count) {
    const life_reference_record_t* smaller = NULL;
    const life_reference_record_t* bigger = NULL;
    if (voltage > records[0].voltage) {
        return 0;
    }
    for (uint16_t i = 1; i < count; i++) {
        if (records[i].voltage < voltage) {
            bigger = &records[i-1];
            smaller = &records[i];
            break;
        }
    }

    if (smaller == NULL || bigger == NULL) {
        return records[count-1].unit;
    }

    return uninterpolate(bigger->unit, smaller->unit, bigger->voltage, voltage, smaller->voltage, true);
}

static uint16_t calculatePercent(
    uint16_t voltage,
    battery_manager_config_t* config,
    const life_reference_record_t* records, uint16_t count
) {
    uint16_t fullMinute = findMinuteOf(config->maxVoltage, records, count);
    uint16_t emptyMinute = findMinuteOf(config->minVoltage, records, count);
    uint16_t currentMinute = findMinuteOf(voltage, records, count);
    uint16_t percent = uninterpolate(1, 100, emptyMinute, currentMinute, fullMinute, true);

    return percent;
}

uint16_t BatteryCalculator_CalculatePercent(uint16_t correctedVoltage) {
    battery_manager_config_t* config = BatteryManager_GetCurrentBatteryConfig();
    const life_reference_record_t* records = DEVICE_IS_UHK80_RIGHT ? RightRecords : LeftRecords;
    uint16_t count = DEVICE_IS_UHK80_RIGHT ? ARRAY_SIZE(RightRecords) : ARRAY_SIZE(LeftRecords);
    uint16_t percent = calculatePercent(correctedVoltage, config, records, count);

    return percent;
}

#define TOLERANCE 4

uint16_t BatteryCalculator_Step(uint8_t oldPercentage, uint8_t newPercentage) {
    if (oldPercentage == newPercentage || oldPercentage == 0) {
        return newPercentage;
    }

    int8_t step;
    int8_t absoluteDiff;
    int8_t res = 0;

    if (oldPercentage < newPercentage) {
        step = 1;
        absoluteDiff = newPercentage - oldPercentage;
    } else {
        step = -1;
        absoluteDiff = oldPercentage - newPercentage;
    }

    if (absoluteDiff > TOLERANCE*2) {
        res = oldPercentage + (absoluteDiff-TOLERANCE*2)*step;
    } else if (absoluteDiff > TOLERANCE || newPercentage == 100 || newPercentage == 1) {
        res = oldPercentage + step;
    } else if (absoluteDiff) {
        res = oldPercentage;
    }
    printk("Step called with %d %d -> %d\n", oldPercentage, newPercentage, res);
    return res;
}


