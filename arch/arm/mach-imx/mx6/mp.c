// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2014
 * Gabriel Huau <contact@huau-gabriel.fr>
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * Copyright 2018-2020 NXP
 *
 */

#include <common.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/imx-regs.h>

#define MAX_CPUS 4
static struct src *src = (struct src *)SRC_BASE_ADDR;

static uint32_t cpu_reset_mask[MAX_CPUS] = {
	0, /* We don't really want to modify the cpu0 */
	SRC_SCR_CORE_1_RESET_MASK,
	SRC_SCR_CORE_2_RESET_MASK,
	SRC_SCR_CORE_3_RESET_MASK
};

static uint32_t cpu_ctrl_mask[MAX_CPUS] = {
	0, /* We don't really want to modify the cpu0 */
	SRC_SCR_CORE_1_ENABLE_MASK,
	SRC_SCR_CORE_2_ENABLE_MASK,
	SRC_SCR_CORE_3_ENABLE_MASK
};

DECLARE_GLOBAL_DATA_PTR;

int fsl_layerscape_wakeup_fixed_core(u32 coreid, u32 addr)
{

	printf("%s cores %d, addr=0x%x, gd->reloc_addr 0x%x\n",
		__func__, coreid, addr, gd->relocaddr);
	switch (coreid) {
	case 1:
		src->gpr3 = addr;
		break;
	case 2:
		src->gpr5 = addr;
		break;
	case 3:
		src->gpr7 = addr;
		break;
	default:
		return 1;
	}

	/* CPU N is ready to start */
	src->scr |= cpu_ctrl_mask[coreid];
	src->scr |= cpu_reset_mask[coreid];

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

int cpu_reset(u32 nr)
{
	/* Software reset of the CPU N */
	src->scr |= cpu_reset_mask[nr];
	return 0;
}

int cpu_status(u32 nr)
{
	printf("core %d => %d\n", nr, !!(src->scr & cpu_ctrl_mask[nr]));
	return 0;
}

int cpu_release(u32 nr, int argc, char *const argv[])
{
	uint32_t boot_addr;

	boot_addr = simple_strtoul(argv[0], NULL, 16);

	switch (nr) {
	case 1:
		src->gpr3 = boot_addr;
		break;
	case 2:
		src->gpr5 = boot_addr;
		break;
	case 3:
		src->gpr7 = boot_addr;
		break;
	default:
		return 1;
	}

	/* CPU N is ready to start */
	src->scr |= cpu_ctrl_mask[nr];

	return 0;
}

int is_core_valid(unsigned int core)
{
	uint32_t nr_cores = get_nr_cpus();

	if (core > nr_cores)
		return 0;

	return 1;
}

int cpu_disable(u32 nr)
{
	/* Disable the CPU N */
	src->scr &= ~cpu_ctrl_mask[nr];
	return 0;
}
