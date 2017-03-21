/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/tzasc380.h>
#include <asm/types.h>
#include <asm/io.h>

int tzasc_set_region(u32 tzasc_base, u32 region_id, u32 enabled, u32 low_addr,
		     u32 high_addr, u32 size, u32 security, u32 subreg_disable_mask)
{
	u32 *reg;
	u32 *reg_base;

	reg_base = (u32 *)(tzasc_base + TZASC_REGIONS_REG + (region_id << 4));

	if (region_id == 0) {
		reg = (u32 *)((u32)reg_base + TZASC_REGION_ATTR_OFFSET);
		out_le32(reg, ((security & 0xF) << 28)); }
	 else{
		reg = (u32 *)((u32)reg_base + TZASC_REGION_LOWADDR_OFFSET);
		out_le32(reg, (low_addr & TZASC_REGION_LOWADDR_MASK));

		reg = (u32 *)((u32)reg_base + TZASC_REGION_HIGHADDR_OFFSET);
		out_le32(reg, high_addr);

		reg = (u32 *)((u32)reg_base + TZASC_REGION_ATTR_OFFSET);
		out_le32(reg, (((security & 0xF) << 28) | ((subreg_disable_mask & 0xFF) << 8) |
			       ((size & 0x3F) << 1) | (enabled & 0x1)));
	}

	return 0;
}
