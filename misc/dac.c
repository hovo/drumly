/*
 * Copyright (c) 2017, NXP Semiconductor, Inc.
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
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
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
 
/**
 * @file    dac.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKL43Z4.h"
/* TODO: insert other include files here. */

/* TODO: insert other definitions and declarations here. */

/*
 * @brief   Application entry point.
 */
int main(void) {

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

    printf("Hello World\n");

    // A5 -> PTC1 -> DAC0_OUT
    // PTE30 -> Alt0 (default) -> DAC0_OUT

    /* It contains a 64-bit resistor ladder network, and 64-to-1
     * multiplexer, which selects an output voltage from one of 64
     * distinct levels that outputs from DAC0. It is controlled through
     * the DAC Control Register DACCR.
     * It's supply reference source can be selected from two sources Vin1, Vin2
     * The module can be powered power or disabled when not in use.
     * When in disabled mode, DAC0 is connected to the analog ground.
     */

    /*
     * DAC0_IRQn
     */

    /*
     * The output of the DAC can be placed on an external pin or set as one
     * of the analog comparator, op-amps, or ADC.
     */
    SIM->SCGC5 |= SIM_SCGC6_DAC0_MASK;

    /*
     * DAC Reference Select
     * The DAC selects DACREF_2 as the reference voltage
     */
    DAC0->C0 |= DAC_C0_DACRFS_MASK;

    /*
     * DAC Enable
     * The DAC system is enabled
     */
    DAC0->C0 |= DAC_C0_DACEN_MASK;

    /*
     * DAC Buffer Read Pointer
     */
    DAC0->C2 |= DAC_C2_DACBFRP_MASK;
    DAC0->C2 |= DAC_C2_DACBFRP(0U);


    return 0 ;
}
