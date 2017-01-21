#ifndef __MERGE_SENSOR_H__
#define __MERGE_SENSOR_H__

// Includes:

    #include "fsl_gpio.h"

// Macros:

#if UHK_PCB_MAJOR_VERSION >= 7
    #define MERGE_SENSOR_GPIO        GPIOB
    #define MERGE_SENSOR_PORT        PORTB
    #define MERGE_SENSOR_CLOCK       kCLOCK_PortB
    #define MERGE_SENSOR_PIN         3
    #define MERGE_SENSOR_IRQ         PORTB_IRQn
    #define MERGE_SENSOR_IRQ_HANDLER PORTB_IRQHandler
#else
    #define MERGE_SENSOR_GPIO        GPIOB
    #define MERGE_SENSOR_PORT        PORTB
    #define MERGE_SENSOR_CLOCK       kCLOCK_PortB
    #define MERGE_SENSOR_PIN         2
    #define MERGE_SENSOR_IRQ         PORTB_IRQn
    #define MERGE_SENSOR_IRQ_HANDLER PORTB_IRQHandler
#endif

    #define MERGE_SENSOR_IS_MERGED !GPIO_ReadPinInput(MERGE_SENSOR_GPIO, MERGE_SENSOR_PIN)

// Functions:

    extern void InitMergeSensor();

#endif
