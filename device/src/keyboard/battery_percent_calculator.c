#include "battery_percent_calculator.h"
#include <stdint.h>
#include "battery_manager.h"
#include "device.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(Battery);

typedef struct {
    uint16_t unit;
    uint16_t voltage;
} life_reference_record_t;

const life_reference_record_t LeftRecords [] = {
    { .unit = 0, .voltage = 4288 },
    { .unit = 200, .voltage = 4022 },
    { .unit = 218, .voltage = 3998 },
    { .unit = 248, .voltage = 3950 },
    { .unit = 298, .voltage = 3859 },
    { .unit = 338, .voltage = 3818 },
    { .unit = 378, .voltage = 3771 },
    { .unit = 418, .voltage = 3629 },
    { .unit = 458, .voltage = 3496 },
    { .unit = 478, .voltage = 3420 },
    { .unit = 488, .voltage = 3317 },
    { .unit = 498, .voltage = 3168 },
    { .unit = 598, .voltage = 1678 },
};

const life_reference_record_t RightRecords[] = {
    { .unit = 0, .voltage = 4400 },
    { .unit = 100, .voltage = 4039 },
    { .unit = 118, .voltage = 3974 },
    { .unit = 158, .voltage = 3863 },
    { .unit = 198, .voltage = 3770 },
    { .unit = 238, .voltage = 3682 },
    { .unit = 278, .voltage = 3645 },
    { .unit = 318, .voltage = 3611 },
    { .unit = 358, .voltage = 3587 },
    { .unit = 398, .voltage = 3548 },
    { .unit = 438, .voltage = 3500 },
    { .unit = 478, .voltage = 3438 },
    { .unit = 498, .voltage = 3415 },
    { .unit = 508, .voltage = 3392 },
    { .unit = 528, .voltage = 3230 },
    { .unit = 538, .voltage = 3065 },
    { .unit = 638, .voltage = 2400 },
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

static void runPercentTest(bool isRight, uint16_t actual, uint16_t max) {
    battery_manager_config_t cfg = {
        .maxVoltage = max,
        .stopChargingVoltage = 4200,
        .startChargingVoltage = 4000,
        .turnOnBacklightVoltage = 3550,
        .turnOffBacklightVoltage = 3400,
        .minWakeupVoltage = 3100,
        .minVoltage = 3000,
    };

    const life_reference_record_t* records = isRight ? RightRecords : LeftRecords;
    uint16_t count = isRight ? ARRAY_SIZE(RightRecords) : ARRAY_SIZE(LeftRecords);
    uint16_t percent = calculatePercent(actual, &cfg, records, count);

    printk("  - %d / %d -> %d%%\n", actual, max, percent);

}

void BatteryCalculator_RunPercentTests() {
    printk("Testing battery percent calculator (right):\n");
    runPercentTest(true, 3100, 4150);
    runPercentTest(true, 3800, 4150);
    runPercentTest(true, 4000, 4150);
    runPercentTest(true, 4100, 4150);
    runPercentTest(true, 4145, 4150);
    printk("  ---- (right)\n");
    runPercentTest(true, 3100, 4050);
    runPercentTest(true, 3800, 4050);
    runPercentTest(true, 4000, 4050);
    runPercentTest(true, 4045, 4050);
    printk("  ---- (left)\n");
    runPercentTest(false, 3100, 4050);
    runPercentTest(false, 3800, 4050);
    runPercentTest(false, 4000, 4050);
    runPercentTest(false, 4045, 4050);
}

#define TOLERANCE 1

uint16_t BatteryCalculator_Step(uint8_t oldPercentage, uint8_t newPercentage) {
    if (oldPercentage == newPercentage || oldPercentage == 0 || TOLERANCE == 0) {
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
    LOG_INF("Step called with %d %d -> %d\n", oldPercentage, newPercentage, res);
    return res;
}


