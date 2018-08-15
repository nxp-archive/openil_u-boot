/*
 * freescale flextimer Support
 *
 * Copyright 2018 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __FLEX_TIMER_H
#define __FLEX_TIMER_H

#include <stdbool.h>
#include <common.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <asm/interrupt-gic.h>

/* FLEXTIMER module Status And Control register */
#define FLEXTIMER_SC_TOF			BIT(7)
#define FLEXTIMER_SC_TOIE			BIT(6)
#define FLEXTIMER_SC_CLKS(x)			((x) << 3)
#define FLEXTIMER_SC_CLKS_MASK			FLEXTIMER_SC_CLKS(0x3)
#define FLEXTIMER_SC_PS(x)			(x)
#define FLEXTIMER_SC_PS_MASK			(0x7)

/* FLEXTIMER module Features Mode Selection register */
#define FLEXTIMER_MODE_WPDIS			BIT(2)
#define FLEXTIMER_MODE_FTMEN			BIT(1)

/* FLEXTIMER module Fault Mode Status register */
#define FLEXTIMER_FMS_WPEN			BIT(6)

typedef void (*irq_handle)(int, int);

struct ftm_module {
	u32 SC;			/* 0x00 */
	u32 CNT;		/* 0x04 */
	u32 MOD;		/* 0x08 */
	u32 C0SC;		/* 0x0C */
	u32 C0v;		/* 0x10 */
	u32 C1SC;		/* 0x14 */
	u32 C1v;		/* 0x18 */
	u32 C2SC;		/* 0x1C */
	u32 C2v;		/* 0x20 */
	u32 C3SC;		/* 0x24 */
	u32 C3v;		/* 0x28 */
	u32 C4SC;		/* 0x2C */
	u32 C4v;		/* 0x30 */
	u32 C5SC;		/* 0x34 */
	u32 C5v;		/* 0x38 */
	u32 C6SC;		/* 0x3C */
	u32 C6v;		/* 0x40 */
	u32 C7SC;		/* 0x44 */
	u32 C7v;		/* 0x48 */
	u32 CNTIN;		/* 0x4C */
	u32 STATUS;		/* 0x50 */
	u32 MODE;		/* 0x54 */
	u32 SYNC;		/* 0x58 */
	u32 OUTINIT;		/* 0x5C */
	u32 OUTMASK;		/* 0x60 */
	u32 COMBINE;		/* 0x64 */
	u32 DEADTIME;		/* 0x68 */
	u32 EXTTRIG;		/* 0x6C */
	u32 POL;		/* 0x70 */
	u32 FMS;		/* 0x74 */
	u32 FILTER;		/* 0x78 */
	u32 FLTCTRL;		/* 0x7C */
	u32 QDCTRL;		/* 0x80 */
	u32 CONF;		/* 0x84 */
	u32 FLTPOL;		/* 0x88 */
	u32 SYNCONF;		/* 0x8C */
	u32 INVCTRL;		/* 0x90 */
	u32 SWOCTRL;		/* 0x94 */
	u32 PWMLOAD;		/* 0x98 */
};

#define FTM1_BASE           ((u32)0x029D0000)
#define FTM2_BASE           ((u32)0x029E0000)
#define FTM3_BASE           ((u32)0x029F0000)
#define FTM4_BASE           ((u32)0x02A00000)
#define FTM5_BASE           ((u32)0x02A10000)
#define FTM6_BASE           ((u32)0x02A20000)
#define FTM7_BASE           ((u32)0x02A30000)
#define FTM8_BASE           ((u32)0x02A40000)

#define FTM1                ((struct ftm_module *)FTM1_BASE)
#define FTM2                ((struct ftm_module *)FTM2_BASE)
#define FTM3                ((struct ftm_module *)FTM3_BASE)
#define FTM4                ((struct ftm_module *)FTM4_BASE)
#define FTM5                ((struct ftm_module *)FTM5_BASE)
#define FTM6                ((struct ftm_module *)FTM6_BASE)
#define FTM7                ((struct ftm_module *)FTM7_BASE)
#define FTM8                ((struct ftm_module *)FTM8_BASE)

enum ftm_clksrc_e {
	clock_no = 0,
	clock_sys,
	clock_fixed,
	clock_external
};

enum presdiv_e {
	div_1 = 0,
	div_2,
	div_4,
	div_8,
	div_16,
	div_32,
	div_64,
	div_128,
};

struct ftm_init_t {
	bool overflow_is_enable;
	enum ftm_clksrc_e clk;
	enum presdiv_e prescale;
	u8 overflow_is_skip;
	u16 cnt_init;
	u16 mod;
};

typedef void (*ftm_irq_func)(void);

extern ftm_irq_func flextimer_overflow_handle;

int flextimer_init(void);

#endif
