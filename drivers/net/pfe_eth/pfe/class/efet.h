/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef _CLASS_EFET_H_
#define _CLASS_EFET_H_

#define CLASS_EFET_ENTRY_ADDR		(EFET_BASE_ADDR + 0x00)
#define CLASS_EFET_ENTRY_SIZE		(EFET_BASE_ADDR + 0x04)
#define CLASS_EFET_ENTRY_DMEM_ADDR	(EFET_BASE_ADDR + 0x08)
#define CLASS_EFET_ENTRY_STATUS		(EFET_BASE_ADDR + 0x0c)
#define CLASS_EFET_ENTRY_ENDIAN		(EFET_BASE_ADDR + 0x10)

#define CBUS2DMEM	0
#define DMEM2CBUS	1

#define EFET2BUS_LE     (1 << 0)
#define PE2BUS_LE	(1 << 1)

void class_efet(u32 cbus_addr, u32 dmem_addr, u32 len, u32 dir);
void class_efet_wait(void);
void class_efet_sync(u32 cbus_addr, u32 dmem_addr, u32 len, u32 dir);

#endif /* _CLASS_EFET_H_ */

