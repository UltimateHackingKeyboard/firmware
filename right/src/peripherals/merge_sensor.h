#ifndef __MERGE_SENSOR_H__
#define __MERGE_SENSOR_H__

// Includes:

#include <stdbool.h>

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <device.h>
#endif

// Variables:

    typedef enum {
        MergeSensorState_Unknown = 0,
        MergeSensorState_Split = 1,
        MergeSensorState_Joined = 2,
    } merge_sensor_state_t;

    extern merge_sensor_state_t MergeSensor_HalvesAreMerged;

// Functions:

    void MergeSensor_Init(void);
    merge_sensor_state_t MergeSensor_IsMerged(void);

    void MergeSensor_Update(void);

#endif
