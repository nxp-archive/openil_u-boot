// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2021 NXP.
 *
 */

#include <common.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/arch/immap_ls102xa.h>
#include <asm/arch/mp.h>
#include <asm/secure.h>

DECLARE_GLOBAL_DATA_PTR;

int fsl_layerscape_wakeup_fixed_core(u32 coreid)
{
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);

	out_be32(&gur->scratchrw[0], CONFIG_SYS_TEXT_BASE);
	out_be32(&gur->brrl, 0x2);

	/*
	 * LS1 STANDBYWFE is not captured outside the ARM module in the soc.
	 * So add a delay to wait bootrom execute WFE.
	 */
	udelay(1);

	asm volatile("sev");

	return 0;
}

int is_core_valid(unsigned int core)
{
	return (core < CONFIG_MAX_CPUS) ? 1 : 0;
}

int is_core_online(u64 cpu_id)
{
	return 0;
}

int cpu_reset(int nr)
{
	return 0;
}

int cpu_disable(int nr)
{
	puts("Feature is not implemented.\n");

	return 0;
}

int core_to_pos(int nr)
{
	return 0;
}

int cpu_status(int nr)
{
	return 0;
}

int cpu_release(int nr, int argc, char * const argv[])
{
	return 0;
}

int get_core_id(void)
{
	unsigned long aff;

	asm volatile("mrc p15, 0, %0, c0, c0, 5\n"
			: "=r" (aff)
			:
			: "memory");

	return aff & 0x3;
}
