/*
 * freescale flextimer Support
 *
 * Copyright 2018 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include "flextimer.h"

static u32 readl(void *addr)
{
	return *(u32 *)addr;
}

static void writel(u32 val, void *addr)
{
	*(u32 *)addr = val;
}

static u32 flextimer_read(void *addr)
{
	u32 le = readl(addr);
	u8 *mid = (u8 *)&le;

	return ((u32)mid[0] << 24) + ((u32)mid[1] << 16) +
		((u32)mid[2] << 8) + (u32)mid[3];
}

static void flextimer_write(u32 val, void *addr)
{
	u8 *mid = (u8 *)&val;
	u32 be = ((u32)mid[0] << 24) + ((u32)mid[1] << 16) +
		 ((u32)mid[2] << 8) + (u32)mid[3];

	writel(be, addr);
}

void write_protection(struct ftm_module *ftmx, bool is_enable)
{
	u32 reg = flextimer_read(&(ftmx->FMS));

	if (is_enable) {
		reg |= FLEXTIMER_FMS_WPEN;
		flextimer_write(reg, &(ftmx->FMS));
	} else{
		reg = flextimer_read(&(ftmx->MODE));
		reg |= FLEXTIMER_MODE_WPDIS;
		flextimer_write(reg, &(ftmx->MODE));
	}
}

static inline void ftm_irq_acknowledge(int hw_irq)
{
	unsigned int timeout = 100;
	struct ftm_module *ftmx;

	switch (hw_irq) {
	case 150:
		ftmx = FTM1;
		break;

	case 151:
		ftmx = FTM2;
		break;

	default:
		return;
	}

	while ((FLEXTIMER_SC_TOF & flextimer_read(&(ftmx->SC))) && timeout--)
		flextimer_write(flextimer_read(&(ftmx->SC)) &
		(~FLEXTIMER_SC_TOF), &(ftmx->SC));
}

ftm_irq_func flextimer_overflow_handle = NULL;
static void flextimer2_irq(int hw_irq, int src_coreid)
{
	ftm_irq_acknowledge(hw_irq);
	if (flextimer_overflow_handle != NULL)
		flextimer_overflow_handle();
}

static void flextimer_register_irq(u32 coreid, u32 hw_irq)
{
	irq_handle irq_func = NULL;

	switch (hw_irq) {
	case 150:
		break;

	case 151:
		irq_func = flextimer2_irq;
		break;

	default:
		return;
	}

	gic_irq_register(hw_irq, irq_func);
	gic_set_type(hw_irq);
	gic_set_target(1 << coreid, hw_irq);
	enable_interrupts();
}

void flextimer_config(struct ftm_module *ftmx, struct ftm_init_t *ftmx_init)
{
	u32 reg = 0, coreid = 1, hw_irq = 150;

	switch ((u32)ftmx) {
	case FTM1_BASE:
		hw_irq = 150;
		break;

	case FTM2_BASE:
		hw_irq = 151;
		break;

	default:
		return;
	}

	write_protection(ftmx, false);

	flextimer_write(ftmx_init->cnt_init, &(ftmx->CNTIN));
	flextimer_write(ftmx_init->cnt_init, &(ftmx->CNT));
	flextimer_write(ftmx_init->mod, &(ftmx->MOD));/* 100us */

	if (ftmx_init->overflow_is_enable) {
		flextimer_register_irq(coreid, hw_irq);
		reg |= FLEXTIMER_SC_TOIE;
	}
	reg |= FLEXTIMER_SC_CLKS(ftmx_init->clk);
	reg |= FLEXTIMER_SC_PS(ftmx_init->prescale);
	flextimer_write(reg, &(ftmx->SC));

	reg = flextimer_read(&(ftmx->MODE));
	reg |= FLEXTIMER_MODE_FTMEN;
	flextimer_write(reg, &(ftmx->MODE));

	write_protection(ftmx, true);
}

int flextimer_init(void)
{
	struct ftm_init_t flextimer2_init;

	flextimer2_init.overflow_is_enable = true;
	flextimer2_init.clk = clock_sys;
	flextimer2_init.prescale = div_4;
	flextimer2_init.overflow_is_skip = 0;
	flextimer2_init.cnt_init = 0;
	flextimer2_init.mod = 3750;
	flextimer_config(FTM2, &flextimer2_init);

	return 0;
}
