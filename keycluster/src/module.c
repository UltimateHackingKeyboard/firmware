#include "fsl_gpio.h"
#include "fsl_port.h"
#include "module.h"

#define BLACKBERRY_TRACKBALL_LEFT_PORT PORTE
#define BLACKBERRY_TRACKBALL_LEFT_GPIO GPIOE
#define BLACKBERRY_TRACKBALL_LEFT_IRQ PORTE_IRQn
#define BLACKBERRY_TRACKBALL_LEFT_CLOCK kCLOCK_PortE
#define BLACKBERRY_TRACKBALL_LEFT_PIN 0

#define BLACKBERRY_TRACKBALL_RIGHT_PORT PORTE
#define BLACKBERRY_TRACKBALL_RIGHT_GPIO GPIOE
#define BLACKBERRY_TRACKBALL_RIGHT_IRQ PORTE_IRQn
#define BLACKBERRY_TRACKBALL_RIGHT_CLOCK kCLOCK_PortE
#define BLACKBERRY_TRACKBALL_RIGHT_PIN 17

#define BLACKBERRY_TRACKBALL_UP_PORT PORTE
#define BLACKBERRY_TRACKBALL_UP_GPIO GPIOE
#define BLACKBERRY_TRACKBALL_UP_IRQ PORTE_IRQn
#define BLACKBERRY_TRACKBALL_UP_CLOCK kCLOCK_PortE
#define BLACKBERRY_TRACKBALL_UP_PIN 1

#define BLACKBERRY_TRACKBALL_DOWN_PORT PORTE
#define BLACKBERRY_TRACKBALL_DOWN_GPIO GPIOE
#define BLACKBERRY_TRACKBALL_DOWN_IRQ PORTE_IRQn
#define BLACKBERRY_TRACKBALL_DOWN_CLOCK kCLOCK_PortE
#define BLACKBERRY_TRACKBALL_DOWN_PIN 16

pointer_delta_t PointerDelta;

key_vector_t keyVector = {
    .itemNum = KEYBOARD_VECTOR_ITEMS_NUM,
    .items = (key_vector_pin_t[]) {
        {PORTC, GPIOC, kCLOCK_PortC,  5}, // top key
        {PORTC, GPIOC, kCLOCK_PortC,  6}, // left key
        {PORTD, GPIOD, kCLOCK_PortD,  7}, // right key
        {PORTC, GPIOC, kCLOCK_PortC,  7}, // left microswitch
        {PORTA, GPIOA, kCLOCK_PortA, 19}, // trackball microswitch
        {PORTD, GPIOD, kCLOCK_PortD,  6}, // right microswitch
    },
};

void BlackberryTrackball_Init(void)
{
    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_LEFT_CLOCK);
    PORT_SetPinConfig(BLACKBERRY_TRACKBALL_LEFT_PORT, BLACKBERRY_TRACKBALL_LEFT_PIN,
        &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});

    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_RIGHT_CLOCK);
    PORT_SetPinConfig(BLACKBERRY_TRACKBALL_RIGHT_PORT, BLACKBERRY_TRACKBALL_RIGHT_PIN,
        &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});

    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_UP_CLOCK);
    PORT_SetPinConfig(BLACKBERRY_TRACKBALL_UP_PORT, BLACKBERRY_TRACKBALL_UP_PIN,
        &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});

    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_DOWN_CLOCK);
    PORT_SetPinConfig(BLACKBERRY_TRACKBALL_DOWN_PORT, BLACKBERRY_TRACKBALL_DOWN_PIN,
        &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});
}

#define TPM_SOURCE_CLOCK (CLOCK_GetFreq(kCLOCK_McgIrc48MClk) / 4)

tpm_channel_t tpmChannels[] = {
    {.clock=kCLOCK_PortC, .port=PORTC, .pin=2U, .mux=kPORT_MuxAlt4, .tpmBase=TPM0, .chnlNumber=1U, .dutyCyclePercent=100},
    {.clock=kCLOCK_PortC, .port=PORTC, .pin=3U, .mux=kPORT_MuxAlt4, .tpmBase=TPM0, .chnlNumber=2U, .dutyCyclePercent=100},
    {.clock=kCLOCK_PortC, .port=PORTC, .pin=1U, .mux=kPORT_MuxAlt4, .tpmBase=TPM0, .chnlNumber=0U, .dutyCyclePercent=100},
    {.clock=kCLOCK_PortD, .port=PORTD, .pin=4U, .mux=kPORT_MuxAlt4, .tpmBase=TPM0, .chnlNumber=4U, .dutyCyclePercent=100},
    {.clock=kCLOCK_PortC, .port=PORTC, .pin=4U, .mux=kPORT_MuxAlt4, .tpmBase=TPM0, .chnlNumber=3U, .dutyCyclePercent=100},
    {.clock=kCLOCK_PortD, .port=PORTD, .pin=5U, .mux=kPORT_MuxAlt4, .tpmBase=TPM0, .chnlNumber=5U, .dutyCyclePercent=100},
//    {.clock=kCLOCK_PortB, .port=PORTB, .pin=0U, .mux=kPORT_MuxAlt3, .tpmBase=TPM1, .chnlNumber=0U, .dutyCyclePercent=100},
//    {.clock=kCLOCK_PortB, .port=PORTB, .pin=1U, .mux=kPORT_MuxAlt3, .tpmBase=TPM1, .chnlNumber=1U, .dutyCyclePercent=100},
    {.clock=kCLOCK_PortA, .port=PORTA, .pin=1U, .mux=kPORT_MuxAlt3, .tpmBase=TPM2, .chnlNumber=0U, .dutyCyclePercent=100},
};

void Tpm_Init(void)
{
    CLOCK_SetTpmClock(1U);

    tpm_config_t tpmInfo;
    TPM_GetDefaultConfig(&tpmInfo);
    tpmInfo.prescale = kTPM_Prescale_Divide_4;

    tpm_chnl_pwm_signal_param_t tpmParam;
    tpmParam.level = kTPM_LowTrue;

    uint8_t count = sizeof(tpmChannels) / sizeof(tpmChannels[0]);

    for (uint8_t i=0; i<count; i++) {
        CLOCK_EnableClock(tpmChannels[i].clock);
        PORT_SetPinMux(tpmChannels[i].port, tpmChannels[i].pin, tpmChannels[i].mux);

        tpmParam.chnlNumber = tpmChannels[i].chnlNumber;
        tpmParam.dutyCyclePercent = tpmChannels[i].dutyCyclePercent;

        TPM_Init(tpmChannels[i].tpmBase, &tpmInfo);
        TPM_SetupPwm(tpmChannels[i].tpmBase, &tpmParam, 1U, kTPM_EdgeAlignedPwm, 24000U, TPM_SOURCE_CLOCK);
        TPM_StartTimer(tpmChannels[i].tpmBase, kTPM_SystemClock);
    }
}

bool oldLeft, oldRight, oldUp, oldDown;

void BlackberryTrackball_Update(void)
{
    uint8_t newLeft = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_LEFT_GPIO, BLACKBERRY_TRACKBALL_LEFT_PIN);
    if (oldLeft != newLeft) {
        PointerDelta.x--;
        oldLeft = newLeft;
    }

    uint8_t newRight = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_RIGHT_GPIO, BLACKBERRY_TRACKBALL_RIGHT_PIN);
    if (oldRight != newRight) {
        PointerDelta.x++;
        oldRight = newRight;
    }

    uint8_t newUp = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_UP_GPIO, BLACKBERRY_TRACKBALL_UP_PIN);
    if (oldUp != newUp) {
        PointerDelta.y--;
        oldUp = newUp;
    }

    uint8_t newDown = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_DOWN_GPIO, BLACKBERRY_TRACKBALL_DOWN_PIN);
    if (oldDown != newDown) {
        PointerDelta.y++;
        oldDown = newDown;
    }
}

void Module_Init(void)
{
    KeyVector_Init(&keyVector);
    BlackberryTrackball_Init();
    Tpm_Init();
}

void Module_Loop(void)
{
    BlackberryTrackball_Update();
}
