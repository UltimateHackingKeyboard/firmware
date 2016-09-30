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
#include "test_led.h"

void BOARD_InitPins(void)
{
    // Ungate ports.
    CLOCK_EnableClock(kCLOCK_PortA); // LEDs
    CLOCK_EnableClock(kCLOCK_PortB); // SW3
    CLOCK_EnableClock(kCLOCK_PortC); // SW2
    CLOCK_EnableClock(kCLOCK_PortD); // LEDs, I2C

    // Set up switches
    port_pin_config_t switchConfig = {
        .pullSelect = kPORT_PullUp,
        .mux = kPORT_MuxAsGpio,
    };
    PORT_SetPinConfig(BOARD_SW2_PORT, BOARD_SW2_GPIO_PIN, &switchConfig);

    // Initialize LEDs.

    PORT_SetPinMux(TEST_LED_GPIO_PORT, TEST_LED_GPIO_PIN, kPORT_MuxAsGpio);
    TEST_RED_INIT(LOGIC_LED_ON);
//    GPIO_SetPinsOutput(TEST_LED_GPIO,   1 << TEST_LED_GPIO_PIN);

    // Initialize I2C.

    port_pin_config_t pinConfig = {0};
    pinConfig.pullSelect = kPORT_PullUp;
    pinConfig.openDrainEnable = kPORT_OpenDrainEnable;

    PORT_SetPinConfig(PORTD, 2, &pinConfig);
    PORT_SetPinConfig(PORTD, 3, &pinConfig);

    PORT_SetPinMux(PORTD, 2, kPORT_MuxAlt7);
    PORT_SetPinMux(PORTD, 3, kPORT_MuxAlt7);
}
