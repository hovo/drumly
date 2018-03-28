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
 * @file    timer.c
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
volatile uint32_t counter = 0;

void TPM1_IRQHandler(void) {
	counter++;
//	if(TPM1->CNT == TPM1->MOD){
//		// Clear the TOF
//		TPM1->SC |= TPM_SC_TOF_MASK;
//		TPM1->CNT = 0x0000;
//		printf("%d\n", counter);
//		counter = 0;
//	}
	printf("%d\n", counter);
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

    __disable_irq(); // global

    // D3 Arduino pinout -> PTA12 -> TPM1_CH0
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK; // Set Bit 9 to 1 -> Clock enabled
    PORTA->PCR[12] |= 0b001100000000; // Set bits 10,9,8 to 011 respectively
    SIM->SCGC6 |= SIM_SCGC6_TPM1_MASK; // Control gate enabled for TPM1 module
    SIM->SOPT2 |= 0x01000000; // TPMSRC = 01 -> 48MHz clock

    TPM1->SC = 0x0; // Disable TPM counter
    TPM1->SC = 0x07; // Pre-scale value of 128 -> the clock is still off
    TPM1->MOD = 0xAC44; // Max value
    TPM1->CNT = 0x0000;
    TPM1->SC |= TPM_SC_TOF_MASK; // Writing a 1 clears the TOF
    TPM1->SC |= TPM_SC_TOIE_MASK; // TPM_SC_TOIE_MASK
    TPM1->SC |= 0x08; // enable timer

    // NVIC
    NVIC->ISER[0] |= 0x00040000;  // TPM1_IRQn -> 18
    __enable_irq();

    return 0 ;
}
