/*
 * Copyright (c) 2010, Mariano Alvira <mar@devl.org> and other contributors
 * to the MC1322x project (http://mc1322x.devl.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of libmc1322x: see http://mc1322x.devl.org
 * for details. 
 *
 *
 */

#include <mc1322x.h>
#include <stdint.h>

#define MOD 9999
#define CLK 24000000
#define DIV 16 /* uart->CON.XTIM = 0 is 16x oversample (datasheet is incorrect) */

#include <stdio.h>
void uart_setbaud(volatile struct UART_struct * uart, uint32_t baud) {
	uint64_t inc;

	/* baud rate eqn from reference manual */
	/* multiply by an additional 10 to do a fixed point round later */
	inc = ((uint64_t) baud * DIV * MOD * 10 / CLK ) - 10 ;
	/* add 5 and divide by 10 to get a rounding */
	inc = (inc + 5) / 10;

        /* UART must be disabled to set the baudrate */
	uart->CONbits = (struct UART_CON) {
		.TXE = 0,
		.RXE = 0,
	};

	uart->BR = ( (uint16_t)inc << 16 ) | MOD;

	uart->CONbits = (struct UART_CON) {
		.XTIM = 0,
		.TXE = 1,
		.RXE = 1,
	};
}

void uart_init(volatile struct UART_struct * uart, uint32_t baud) {
	/* enable the uart so we can set the gpio mode */
	/* see Section 11.5.1.2 Alternate Modes */
	/* you must enable the peripheral first BEFORE setting the function in GPIO_FUNC_SEL */
	/* From the datasheet: "The peripheral function will control operation of the pad IF */
	/* THE PERIPHERAL IS ENABLED. */
	uart->CONbits = (struct UART_CON) {
		.TXE = 1,
		.RXE = 1,
	};
	/* interrupt when there are this number or more bytes free in the TX buffer*/
	uart->TXCON = 16;

	if( uart == UART1 ) {
		/* TX and CTS as outputs */
		GPIO->PAD_DIR_SET.GPIO_14 = 1;
		GPIO->PAD_DIR_SET.GPIO_16 = 1;
		
		/* RX and RTS as inputs */
		GPIO->PAD_DIR_RESET.GPIO_15 = 1;
		GPIO->PAD_DIR_RESET.GPIO_17 = 1;

		/* set GPIO15-14 to UART (UART1 TX and RX)*/
		GPIO->FUNC_SEL.GPIO_14 = 1;
		GPIO->FUNC_SEL.GPIO_15 = 1;

		u1_head = 0; u1_tail = 0;

		/* tx and rx interrupts are enabled in the UART by default */
		/* see status register bits 13 and 14 */
		/* enable UART1 interrupts in the interrupt controller */
		enable_irq(UART1);

	} else {
		/* do the same as above but for UART2 */
		GPIO->PAD_DIR_SET.GPIO_18 = 1;
		GPIO->PAD_DIR_SET.GPIO_19 = 1;

		GPIO->PAD_DIR_RESET.GPIO_20 = 1;
		GPIO->PAD_DIR_RESET.GPIO_21 = 1;

		GPIO->FUNC_SEL.GPIO_18 = 1;
		GPIO->FUNC_SEL.GPIO_19 = 1;

		u2_head = 0; u2_tail = 0;

		enable_irq(UART2);
	}

	uart_setbaud(uart, baud);

}

