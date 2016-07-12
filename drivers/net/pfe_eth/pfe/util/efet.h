/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef _UTIL_EFET_H_
#define _UTIL_EFET_H_

#define EFET_ENTRY_ADDR		0x00
#define EFET_ENTRY_SIZE		0x04
#define EFET_ENTRY_DMEM_ADDR	0x08
#define EFET_ENTRY_STATUS	0x0c
#define EFET_ENTRY_ENDIAN	0x10

#define CBUS2DMEM	0
#define DMEM2CBUS	1

#define EFET2BUS_LE     (1 << 0)

void util_efet(int i, u32 cbus_addr, u32 dmem_addr, u32 len, u8 dir);
void util_efet_wait(int i);
void util_efet_sync(int i, u32 cbus_addr, u32 dmem_addr, u32 len, u8 dir);

#endif /* _UTIL_EFET_H_ */

