/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_common.h"
#include "fsl_port.h"
#include "board.h"

void BOARD_InitPins(void)
{
    // Ungate ports.
    CLOCK_EnableClock(kCLOCK_PortA); // LEDs
    CLOCK_EnableClock(kCLOCK_PortB); // SW3, I2C
    CLOCK_EnableClock(kCLOCK_PortC); // SW2
    CLOCK_EnableClock(kCLOCK_PortD); // LEDs
    CLOCK_EnableClock(kCLOCK_PortE); // UART1 for OpenSDA

    // Set up UART1 for OpenSDA.
    PORT_SetPinMux(PORTE, 0u, kPORT_MuxAlt3);
    PORT_SetPinMux(PORTE, 1u, kPORT_MuxAlt3);

    // Set up SW2.
    port_pin_config_t switchConfig = {0};
    switchConfig.pullSelect = kPORT_PullUp;
    switchConfig.mux = kPORT_MuxAsGpio;
    PORT_SetPinConfig(BOARD_SW2_PORT, BOARD_SW2_GPIO_PIN, &switchConfig);

    // Set up SW3.
    PORT_SetPinConfig(BOARD_SW3_PORT, BOARD_SW3_GPIO_PIN, &switchConfig);

    // Initialize LEDs.

    PORT_SetPinMux(BOARD_LED_RED_GPIO_PORT, BOARD_LED_RED_GPIO_PIN, kPORT_MuxAsGpio);
    PORT_SetPinMux(BOARD_LED_GREEN_GPIO_PORT, BOARD_LED_GREEN_GPIO_PIN, kPORT_MuxAsGpio);
    PORT_SetPinMux(BOARD_LED_BLUE_GPIO_PORT, BOARD_LED_BLUE_GPIO_PIN, kPORT_MuxAsGpio);

    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput, 0,
    };

    GPIO_PinInit(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN, &led_config);
    GPIO_PinInit(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PIN, &led_config);
    GPIO_PinInit(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PIN, &led_config);

    GPIO_SetPinsOutput(BOARD_LED_RED_GPIO,   1 << BOARD_LED_RED_GPIO_PIN);
    GPIO_SetPinsOutput(BOARD_LED_GREEN_GPIO, 1 << BOARD_LED_GREEN_GPIO_PIN);
    GPIO_SetPinsOutput(BOARD_LED_BLUE_GPIO,  1 << BOARD_LED_BLUE_GPIO_PIN);

    // Initialize I2C.

    port_pin_config_t pinConfig = {0};
    pinConfig.pullSelect = kPORT_PullUp;
    pinConfig.openDrainEnable = kPORT_OpenDrainEnable;

    PORT_SetPinConfig(PORTB, 2, &pinConfig);
    PORT_SetPinConfig(PORTB, 3, &pinConfig);

    PORT_SetPinMux(PORTB, 2, kPORT_MuxAlt2);
    PORT_SetPinMux(PORTB, 3, kPORT_MuxAlt2);
}
