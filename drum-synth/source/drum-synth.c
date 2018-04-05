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
#define STARTING_VALUE 543
#define SAMPLING_FREQUENCY 44100

volatile uint32_t sampleIndex = 0;

/*
 * 256-Point Sine Lookup table for 1 cycle
 */
static const int sin_table[256] = {
		2048,2098,2148,2198,2248,2298,2348,2398,
		2447,2496,2545,2594,2642,2690,2737,2784,
		2831,2877,2923,2968,3013,3057,3100,3143,
		3185,3226,3267,3307,3346,3385,3423,3459,
		3495,3530,3565,3598,3630,3662,3692,3722,
		3750,3777,3804,3829,3853,3876,3898,3919,
		3939,3958,3975,3992,4007,4021,4034,4045,
		4056,4065,4073,4080,4085,4089,4093,4094,
		4095,4094,4093,4089,4085,4080,4073,4065,
		4056,4045,4034,4021,4007,3992,3975,3958,
		3939,3919,3898,3876,3853,3829,3804,3777,
		3750,3722,3692,3662,3630,3598,3565,3530,
		3495,3459,3423,3385,3346,3307,3267,3226,
		3185,3143,3100,3057,3013,2968,2923,2877,
		2831,2784,2737,2690,2642,2594,2545,2496,
		2447,2398,2348,2298,2248,2198,2148,2098,
		2048,1997,1947,1897,1847,1797,1747,1697,
		1648,1599,1550,1501,1453,1405,1358,1311,
		1264,1218,1172,1127,1082,1038,995,952,
		910,869,828,788,749,710,672,636,
		600,565,530,497,465,433,403,373,
		345,318,291,266,242,219,197,176,
		156,137,120,103,88,74,61,50,
		39,30,22,15,10,6,2,1,
		0,1,2,6,10,15,22,30,
		39,50,61,74,88,103,120,137,
		156,176,197,219,242,266,291,318,
		345,373,403,433,465,497,530,565,
		600,636,672,710,749,788,828,869,
		910,952,995,1038,1082,1127,1172,1218,
		1264,1311,1358,1405,1453,1501,1550,1599,
		1648,1697,1747,1797,1847,1897,1947,1997
};

/**
 * Sine lookup table interpolation
 * @param sample index 0-44100
 * @return value from sine LUT
 */
uint8_t getSinIndex(uint32_t sample) {
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
		/* TODO: Send sample at indexIndex to DAC */
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
    PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(STARTING_VALUE); // Initialize PIT0 to count down from STARTING_VALUE
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
		}
    }
    return 0 ;
}
