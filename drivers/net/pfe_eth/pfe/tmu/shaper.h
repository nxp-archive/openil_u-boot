/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef _SHAPER_H_
#define _SHAPER_H_

/* Offsets from SHAPPERx_BASE_ADDR */
#define SHAPER_CTRL	0x00
#define SHAPER_WEIGHT	0x04
#define SHAPER_PKT_LEN	0x08

#define SHAPER_CTRL_ENABLE(x)	(((x) & 0x1) << 0)
#define SHAPER_CTRL_QNO(x)	(((x) & 0x3f) << 1)
#define SHAPER_CTRL_CLKDIV(x)	(((x) & 0xffff) << 16)

#define SHAPER_WEIGHT_FRACWT(x)	(((x) & 0xff) << 0)
#define SHAPER_WEIGHT_INTWT(x)		(((x) & 0x3) << 8)
#define SHAPER_WEIGHT_MAXCREDIT(x)	(((x) & 0x3fffff) << 10)

#define PORT_SHAPER_MASK (1 << 0)

#endif /* _SHAPER_H_ */
