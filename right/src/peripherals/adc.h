#ifndef __ADC_H__
#define __ADC_H__

// Macros:

    #define ADC_PORT  PORTB
    #define ADC_PIN   0
    #define ADC_CLOCK kCLOCK_PortB

    #define ADC_BASE ADC0
    #define ADC_CHANNEL_GROUP 0U
    #define ADC_USER_CHANNEL 8U

// Functions:

    void ADC_Init(void);
    uint32_t ADC_Measure(void);

#endif
