// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2014
 * Gabriel Huau <contact@huau-gabriel.fr>
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <cpu_func.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/imx-regs.h>

static struct src *src = (struct src *)SRC_BASE_ADDR;

static uint32_t cpu_reset_mask[CONFIG_MAX_CPUS] = {
	0, /* We don't really want to modify the cpu0 */
	SRC_A53RCR0_CORE_1_RESET_MASK,
	SRC_A53RCR0_CORE_2_RESET_MASK,
	SRC_A53RCR0_CORE_3_RESET_MASK
};

static uint32_t cpu_ctrl_mask[CONFIG_MAX_CPUS] = {
	0, /* We don't really want to modify the cpu0 */
	SRC_A53RCR1_CORE_1_ENABLE_MASK,
	SRC_A53RCR1_CORE_2_ENABLE_MASK,
	SRC_A53RCR1_CORE_3_ENABLE_MASK
};

int cpu_reset(u32 nr)
{
	/* Software reset of the CPU N */
	src->a53rcr |= cpu_reset_mask[nr];
	return 0;
}

int cpu_status(u32 nr)
{
	printf("core %d => %d\n", nr, !!(src->a53rcr1 & cpu_ctrl_mask[nr]));
	return 0;
}


static inline void mmio_write_32(uintptr_t addr, uint32_t value)
{
    *(volatile uint32_t*)addr = value;
}

static inline uint32_t mmio_read_32(uintptr_t addr)
{
    return *(volatile uint32_t*)addr;
}

static inline void mmio_clrbits_32(uintptr_t addr, uint32_t clear)
{
    mmio_write_32(addr, mmio_read_32(addr) & ~clear);
}

static inline void mmio_setbits_32(uintptr_t addr, uint32_t set)
{
    mmio_write_32(addr, mmio_read_32(addr) | set);
}

void imx_set_cpu_entry(unsigned int core_id, uintptr_t sec_entrypoint)
{
    uint64_t temp_base;

    temp_base = (uint64_t) sec_entrypoint;
    temp_base >>= 2;

    mmio_write_32(IMX_SRC_BASE + IMX8M_SRC_GPR1_OFFSET + (core_id << 3),
        ((uint32_t)(temp_base >> 22) & 0xffff));
    mmio_write_32(IMX_SRC_BASE + IMX8M_SRC_GPR1_OFFSET + (core_id << 3) + 4,
        ((uint32_t)temp_base & 0x003fffff));
}

void imx_set_cpu_pwr_on(unsigned int core_id)
{
    /* clear the wfi power down bit of the core */
    mmio_clrbits_32(IMX_GPC_BASE + IMX8M_LPCR_A53_AD, COREx_WFI_PDN(core_id));

    /* assert the ncpuporeset */
    mmio_clrbits_32(IMX_SRC_BASE + IMX8M_SRC_A53RCR1, (1 << core_id));
    /* assert the pcg pcr bit of the core */
    mmio_setbits_32(IMX_GPC_BASE + COREx_PGC_PCR(core_id), 0x1);
    /* sw power up the core */
    mmio_setbits_32(IMX_GPC_BASE + CPU_PGC_UP_TRG, (1 << core_id));

    /* wait for the power up finished */
    while ((mmio_read_32(IMX_GPC_BASE + CPU_PGC_UP_TRG) & (1 << core_id)) != 0)
        ;

    /* deassert the pcg pcr bit of the core */
    mmio_clrbits_32(IMX_GPC_BASE + COREx_PGC_PCR(core_id), 0x1);
    /* deassert the ncpuporeset */
    mmio_setbits_32(IMX_SRC_BASE + IMX8M_SRC_A53RCR1, (1 << core_id));
}
int cpu_release(u32 nr, int argc, char *const argv[])
{
	uint32_t boot_addr;
	boot_addr = simple_strtoul(argv[0], NULL, 16);
	flush_dcache_all();
    imx_set_cpu_entry(nr,boot_addr);
    imx_set_cpu_pwr_on(nr);

	return 0;
}

int is_core_valid(unsigned int core)
{
    if(core < CONFIG_MAX_CPUS)
		return 1;
	return 0;
}

int cpu_disable(u32 nr)
{
	/* Disable the CPU N */
	src->a53rcr1 &= ~cpu_ctrl_mask[nr];
	return 0;
}

DECLARE_GLOBAL_DATA_PTR;

int fsl_layerscape_wakeup_fixed_core(u32 coreid, u32 addr)
{
	flush_dcache_all();
	imx_set_cpu_entry(coreid, addr);
	imx_set_cpu_pwr_on(coreid);

	return 0;
}

int get_core_id(void)
{
	unsigned long aff;

	return aff;
}
