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
 * @file    drum-synth.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKL43Z4.h"
#include "fsl_pit.h"
#include "fsl_dac.h"

#define mask 255
#define CH_0_STARTING_VALUE 543U
#define SAMPLING_FREQUENCY 44100

volatile uint32_t sampleIndex = 0;

/*
 * 256-Point Sine Lookup table for 1 cycle
 */
static const uint16_t sin_table[256] = {
		248,254,260,266,272,278,284,290,
		296,302,308,314,320,326,332,337,
		343,348,354,360,365,370,375,381,
		386,391,396,401,405,410,415,419,
		423,428,432,436,440,444,447,451,
		454,458,461,464,467,470,472,475,
		477,479,482,483,485,487,489,490,
		491,492,493,494,495,495,496,496,
		496,496,496,495,495,494,493,492,
		491,490,489,487,485,483,482,479,
		477,475,472,470,467,464,461,458,
		454,451,447,444,440,436,432,428,
		423,419,415,410,405,401,396,391,
		386,381,375,370,365,360,354,348,
		343,337,332,326,320,314,308,302,
		296,290,284,278,272,266,260,254,
		248,242,236,230,224,218,212,206,
		200,194,188,182,176,170,164,159,
		153,148,142,136,131,126,121,115,
		110,105,100,95,91,86,81,77,
		73,68,64,60,56,52,49,45,
		42,38,35,32,29,26,24,21,
		19,17,14,13,11,9,7,6,
		5,4,3,2,1,1,0,0,
		0,0,0,1,1,2,3,4,
		5,6,7,9,11,13,14,17,
		19,21,24,26,29,32,35,38,
		42,45,49,52,56,60,64,68,
		73,77,81,86,91,95,100,105,
		110,115,121,126,131,136,142,148,
		153,159,164,170,176,182,188,194,
		200,206,212,218,224,230,236,242
};

/**
 * Sine lookup table interpolation
 * @param sample index 0-44100
 * @return value from sine LUT
 */
uint8_t getSinIndex(uint16_t sample) {
	uint8_t i = sample & mask;
	return sin_table[i];
}

/*
 * Interrupt handler for PIT (Periodic Interrupt Timer)
 * Responsible of streaming data to the DAC module
 */
void PIT_IRQHandler(void) {
	// Clear pending IRQ
	NVIC_ClearPendingIRQ(PIT_IRQn);

	if(PIT->CHANNEL[0].TFLG & PIT_TFLG_TIF_MASK){
		// Clear interrupt request flag for channel 0
		PIT->CHANNEL[0].TFLG &= PIT_TFLG_TIF_MASK;
		// Increment the sample index
		sampleIndex++;
		uint16_t sineValue = getSinIndex(sampleIndex);
		DAC_SetBufferValue(DAC0, 0U, sineValue);
	}

	PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
}

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

    /*------------------ ADC Configuration -------------------- */
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK; // PORTB clock gate control: Clock enabled
    PORTB->PCR[0] =  0x000; // Alt0 (default mode)

    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK; // Enable the DAC module
    ADC0->CFG1 = 0x40 | 0x10 | 0x04 | 0x00; // Software triggering on the ADC (12 bit encoding)
    ADC0->SC2 &= ~0x40;
    ADC0->SC2 |= 0b00000001; // Enable Vref2 3.3v

    /*------------------ DAC Configuration -------------------- */
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK; // PORTE clock gate control: Clock enabled
    PORTE->PCR[30] = 0x000; // Set POERT to DAC0_OUT

    SIM->SCGC6 |= SIM_SCGC6_DAC0_MASK; // DAC0 Clock Gate Control: Clock enabled
    DAC0->C0 |= DAC_C0_DACRFS_MASK; // Vref2
    DAC0->C0 |= DAC_C0_DACEN_MASK; // Enable the DAC module
    DAC0->C2 = (DAC0->C2 & ~DAC_C2_DACBFRP_MASK) | DAC_C2_DACBFRP(0U); // Make sure the read pointer to the start.

    /*------------------ PIT Configuration -------------------- */
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK; // PIT Clock Gate Control: Clock Enabled
    PIT->MCR &= ~PIT_MCR_MDIS_MASK; // Enable module
    PIT->MCR |= PIT_MCR_FRZ_MASK; // Freeze timers in debug mode
    PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(CH_0_STARTING_VALUE); // Initialize PIT0 to count down from STARTING_VALUE
    PIT->CHANNEL[0].TCTRL &= PIT_TCTRL_CHN_MASK; // No chaining of timers
    PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TIE_MASK; // Timer Interrupt Enable: Interrupt will be requested whenever TIF is set

    /*------------------ NVIC Configuration -------------------- */
    NVIC_SetPriority(PIT_IRQn, 128); // Set PIT IRQ priority
    NVIC_ClearPendingIRQ(PIT_IRQn); // Clear any pending IRQ from PIT
    NVIC_EnableIRQ(PIT_IRQn);
    NVIC_EnableIRQ(ADC0_IRQn);

    __enable_irq(); // Ensure interrupts are not masked globally

    PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TEN_MASK; // Start the timer channel
    // PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TEN_MASK;

    ADC0->SC1[0] = 0x48;
    /* Event Loop */
    while(1) {
		// Reset sample index
		if(sampleIndex == SAMPLING_FREQUENCY){
			printf("44100 samples reached\n");
			sampleIndex = 0;
			//PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TEN_MASK;
		}
    }
    return 0 ;
}
