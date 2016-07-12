/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef _UART_H_
#define _UART_H_

#define UART_THR	(UART_BASE_ADDR + 0x00)
#define UART_IER	(UART_BASE_ADDR + 0x04)
#define UART_IIR	(UART_BASE_ADDR + 0x08)
#define UART_LCR	(UART_BASE_ADDR + 0x0c)
#define UART_MCR	(UART_BASE_ADDR + 0x10)
#define UART_LSR	(UART_BASE_ADDR + 0x14)
#define UART_MDR	(UART_BASE_ADDR + 0x18)
#define UART_SCRATCH	(UART_BASE_ADDR + 0x1c)

#endif /* _UART_H_ */
