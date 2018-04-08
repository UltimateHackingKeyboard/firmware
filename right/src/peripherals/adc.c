#include "fsl_adc16.h"
#include "fsl_port.h"
#include "peripherals/adc.h"

static adc16_channel_config_t adc16ChannelConfigStruct;

void ADC_Init(void)
{
    adc16_config_t adc16ConfigStruct;

    CLOCK_EnableClock(ADC_CLOCK);
    PORT_SetPinMux(ADC_PORT, ADC_PIN, kPORT_PinDisabledOrAnalog);

    /*
     * adc16ConfigStruct.referenceVoltageSource = kADC16_ReferenceVoltageSourceVref;
     * adc16ConfigStruct.clockSource = kADC16_ClockSourceAsynchronousClock;
     * adc16ConfigStruct.enableAsynchronousClock = true;
     * adc16ConfigStruct.clockDivider = kADC16_ClockDivider8;
     * adc16ConfigStruct.resolution = kADC16_ResolutionSE12Bit;
     * adc16ConfigStruct.longSampleMode = kADC16_LongSampleDisabled;
     * adc16ConfigStruct.enableHighSpeed = false;
     * adc16ConfigStruct.enableLowPower = false;
     * adc16ConfigStruct.enableContinuousConversion = false;
     */
    ADC16_GetDefaultConfig(&adc16ConfigStruct);
    ADC16_Init(ADC_BASE, &adc16ConfigStruct);
    ADC16_EnableHardwareTrigger(ADC_BASE, false); /* Make sure the software trigger is used. */
    if (kStatus_Success != ADC16_DoAutoCalibration(ADC_BASE))
    {
        // Handle error
    }

    adc16ChannelConfigStruct.channelNumber = ADC_USER_CHANNEL;
    adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = false;
    adc16ChannelConfigStruct.enableDifferentialConversion = false;
}

uint32_t ADC_Measure(void)
{
    /*
     When in software trigger mode, each conversion would be launched once calling the "ADC16_ChannelConfigure()"
     function, which works like writing a conversion command and executing it. For another channel's conversion,
     just to change the "channelNumber" field in channel's configuration structure, and call the
     "ADC16_ChannelConfigure() again.
    */
    ADC16_SetChannelConfig(ADC_BASE, ADC_CHANNEL_GROUP, &adc16ChannelConfigStruct);
    while (0U == (kADC16_ChannelConversionDoneFlag & ADC16_GetChannelStatusFlags(ADC_BASE, ADC_CHANNEL_GROUP))) {}
    return ADC16_GetChannelConversionValue(ADC_BASE, ADC_CHANNEL_GROUP);
}
