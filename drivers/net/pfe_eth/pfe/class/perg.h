/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef _PERG_H_
#define _PERG_H_

#define PERG_QB_BUF_STATUS		(PERG_BASE_ADDR + 0x00)
#define PERG_RO_BUF_STATUS		(PERG_BASE_ADDR + 0x04)
#define PERG_CLR_QB_BUF_STATUS		(PERG_BASE_ADDR + 0x08)
#define PERG_SET_RO_BUF_STATUS		(PERG_BASE_ADDR + 0x0c)
#define PERG_CLR_RO_ERR_PKT		(PERG_BASE_ADDR + 0x10)
#define PERG_CLR_BMU2_ERR_PKT		(PERG_BASE_ADDR + 0x14)

#define PERG_ID				(PERG_BASE_ADDR + 0x18)
#define PERG_TIMER1			(PERG_BASE_ADDR + 0x1c)
#define PERG_TIMER2			(PERG_BASE_ADDR + 0x20)
#define PERG_BUF1			(PERG_BASE_ADDR + 0x24)
#define PERG_BUF2			(PERG_BASE_ADDR + 0x28)
#define PERG_HOST_GP			(PERG_BASE_ADDR + 0x2c)
#define PERG_PE_GP			(PERG_BASE_ADDR + 0x30)
#define PERG_INT_ENABLE			(PERG_BASE_ADDR + 0x34)
#define PERG_INT_SRC			(PERG_BASE_ADDR + 0x38)

#endif /* _PERG_H_ */
