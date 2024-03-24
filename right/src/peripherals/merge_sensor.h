#ifndef __MERGE_SENSOR_H__
#define __MERGE_SENSOR_H__

// Includes:

#include <stdbool.h>

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <device.h>
#endif

// Functions:

#if !(defined(__ZEPHYR__) && !defined(DEVICE_HAS_MERGE_SENSE))
    void MergeSensor_Init(void);
    bool MergeSensor_IsMerged(void);
#endif

#endif
