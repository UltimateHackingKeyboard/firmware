#ifndef __MERGE_SENSOR_H__
#define __MERGE_SENSOR_H__

// Includes:

#include <stdbool.h>

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <device.h>
#endif

// Variables:

    extern bool MergeSensor_HalvesAreMerged;

// Functions:

#if !(defined(__ZEPHYR__) && !defined(DEVICE_HAS_MERGE_SENSOR))
    void MergeSensor_Init(void);
    bool MergeSensor_IsMerged(void);
#endif

    void MergeSensor_Update(void);

#endif
