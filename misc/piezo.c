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
 * @file    piezo.c
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


volatile bool isConverted = 0;
volatile uint32_t result = 0;

void ADC0_IRQHandler(void) {
	NVIC_ClearPendingIRQ(ADC0_IRQn);
	ADC0->SC1[0] = 0x48;

	isConverted = 1;
	result = ADC0->R[0];

#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

/*
 * @brief   Application entry point.
 */
int main(void) {
	uint32_t threshold = 3000U;

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

    // Enable Interrupt for ADC0
    EnableIRQ(ADC0_IRQn);

    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    PORTB->PCR[0] = 0x000; // ALT0 (default mode)

    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
    ADC0->CFG1 = 0x40 | 0x10 | 0x04 | 0x00; // Software triggering on the ADC (12 bit encoding)
    ADC0->SC2 &= ~0x40;

    ADC0->SC1[0] = 0x48;

    while(1) {
    	if(isConverted){
    		if(result >= threshold){
    			//printf("Value: %d \n", result);
    		    printf("Knock detected \n");
    		}
    		isConverted = 0;
    	}
    }

    return 0;
}
