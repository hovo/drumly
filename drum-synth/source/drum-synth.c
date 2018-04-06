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
		310,318,325,333,340,348,355,363,
		370,378,385,393,400,407,414,422,
		429,436,443,449,456,463,469,476,
		482,489,495,501,507,512,518,524,
		529,535,540,545,550,554,559,563,
		568,572,576,580,583,587,590,593,
		596,599,602,604,607,609,611,612,
		614,615,617,618,619,619,620,620,
		620,620,620,619,619,618,617,615,
		614,612,611,609,607,604,602,599,
		596,593,590,587,583,580,576,572,
		568,563,559,554,550,545,540,535,
		529,524,518,512,507,501,495,489,
		482,476,469,463,456,449,443,436,
		429,422,414,407,400,393,385,378,
		370,363,355,348,340,333,325,318,
		310,302,295,287,280,272,265,257,
		250,242,235,227,220,213,206,198,
		191,184,177,171,164,157,151,144,
		138,131,125,119,113,108,102,96,
		91,85,80,75,70,66,61,57,
		52,48,44,40,37,33,30,27,
		24,21,18,16,13,11,9,8,
		6,5,3,2,1,1,0,0,
		0,0,0,1,1,2,3,5,
		6,8,9,11,13,16,18,21,
		24,27,30,33,37,40,44,48,
		52,57,61,66,70,75,80,85,
		91,96,102,108,113,119,125,131,
		138,144,151,157,164,171,177,184,
		191,198,206,213,220,227,235,242,
		250,257,265,272,280,287,295,302
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

    __enable_irq(); // Ensure interrupts are not masked globally

    PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TEN_MASK; // Start the timer channel
    // PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TEN_MASK;

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
