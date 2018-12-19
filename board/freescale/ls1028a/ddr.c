/*
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fsl_ddr_sdram.h>
#include <fsl_ddr_dimm_params.h>
#include <asm/arch/soc.h>
#include <asm/arch/clock.h>
#include <asm/io.h>
#include "ddr.h"

DECLARE_GLOBAL_DATA_PTR;

void fsl_ddr_board_options(memctl_options_t *popts,
			   dimm_params_t *pdimm,
			   unsigned int ctrl_num)
{
	const struct board_specific_parameters *pbsp, *pbsp_highest = NULL;
	ulong ddr_freq;

	if (ctrl_num > 1) {
		printf("Not supported controller number %d\n", ctrl_num);
		return;
	}
	if (!pdimm->n_ranks)
		return;

	/*
	 * we use identical timing for all slots. If needed, change the code
	 * to  pbsp = rdimms[ctrl_num] or pbsp = udimms[ctrl_num];
	 */
	pbsp = udimms[0];

	/* Get clk_adjust, wrlvl_start, wrlvl_ctl, according to the board ddr
	 * freqency and n_banks specified in board_specific_parameters table.
	 */
	ddr_freq = get_ddr_freq(0) / 1000000;
	while (pbsp->datarate_mhz_high) {
		if (pbsp->n_ranks == pdimm->n_ranks) {
			if (ddr_freq <= pbsp->datarate_mhz_high) {
				popts->clk_adjust = pbsp->clk_adjust;
				popts->wrlvl_start = pbsp->wrlvl_start;
				popts->wrlvl_ctl_2 = pbsp->wrlvl_ctl_2;
				popts->wrlvl_ctl_3 = pbsp->wrlvl_ctl_3;
				goto found;
			}
			pbsp_highest = pbsp;
		}
		pbsp++;
	}

	if (pbsp_highest) {
		printf("Error: board specific timing not found for %lu MT/s\n",
		       ddr_freq);
		printf("Trying to use the highest speed (%u) parameters\n",
		       pbsp_highest->datarate_mhz_high);
		popts->clk_adjust = pbsp_highest->clk_adjust;
		popts->wrlvl_start = pbsp_highest->wrlvl_start;
		popts->wrlvl_ctl_2 = pbsp->wrlvl_ctl_2;
		popts->wrlvl_ctl_3 = pbsp->wrlvl_ctl_3;
	} else {
		panic("DIMM is not supported by this board");
	}
found:
	debug("Found timing match: n_ranks %d, data rate %d, rank_gb %d\n"
		"\tclk_adjust %d, wrlvl_start %d, wrlvl_ctrl_2 0x%x, wrlvl_ctrl_3 0x%x\n",
		pbsp->n_ranks, pbsp->datarate_mhz_high, pbsp->rank_gb,
		pbsp->clk_adjust, pbsp->wrlvl_start, pbsp->wrlvl_ctl_2,
		pbsp->wrlvl_ctl_3);



	popts->half_strength_driver_enable = 0;
	/*
	 * Write leveling override
	 */
	popts->wrlvl_override = 1;
	popts->wrlvl_sample = 0xf;


	/* Enable ZQ calibration */
	popts->zq_en = 1;

	/* Enable DDR hashing */
	popts->addr_hash = 1;

	popts->ddr_cdr1 = DDR_CDR1_DHC_EN | DDR_CDR1_ODT(DDR_CDR_ODT_60ohm);

	popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_60ohm) |
			  DDR_CDR2_VREF_TRAIN_EN | DDR_CDR2_VREF_RANGE_2;
}

#ifdef CONFIG_EMU
#if defined(CONFIG_EMU_PXP)
void setup_tzc(void)
{
	out_le32(0x01100004,  0x1);
	out_le32(0x01100120,  0x00000000);
	out_le32(0x01100124,  0x00000000);
	out_le32(0x01100128,  0xffffffff);
	out_le32(0x0110012C,  0xffffffff);
	out_le32(0x01100130,  0xc0000001);
	out_le32(0x01100134,  0xffffffff);
	out_le32(0x01100008,  0x00010001);
}

void ddrmc_init(void)
{
	setup_tzc();

	out_le32(0x000001080000,0x000001ff);
	out_le32(0x000001080008,0x100011ff);
	out_le32(0x000001080010,0x200021ff);
	out_le32(0x000001080018,0x300031ff);
	out_le32(0x000001080080,0x80000522); /* DDR Config : load from flash */
	out_le32(0x000001080084,0x80000522);
	out_le32(0x000001080088,0x80000522);
	out_le32(0x00000108008c,0x80000522);
	out_le32(0x000001080104,0x110108);
	out_le32(0x000001080108,0x44c2a222);
	out_le32(0x00000108010c,0x484804);
	out_le32(0x000001080100,0x1051000);
	out_le32(0x000001080250,0x0);
	out_le32(0x000001080160,0x0);
	out_le32(0x000001080164,0x0);
	out_le32(0x000001080168,0x0);
	out_le32(0x00000108016C,0x20000000);
	out_le32(0x000001080250,0x0);
	out_le32(0x000001080118,0x4);
	out_le32(0x00000108011c,0x00000000);
	out_le32(0x000001080220,0x00000400);
	out_le32(0x000001080224,0x04000000);
	out_le32(0x000001080b2c,0x8000);
	out_le32(0x000001080124,0x0);
	out_le32(0x000001080170,0x87090700);
	out_le32(0x000001080114,0x00000000);
	out_le32(0x000001080130,0x02000000);
	out_le32(0x000001080F10,0xff800800);
	out_le32(0x000001080F14,0x08000800);
	out_le32(0x000001080F18,0x08000800);
	out_le32(0x000001080F1C,0x08000800);
	out_le32(0x000001080F20,0x08000800);
	out_le32(0x000001080f08,0x00000400);
	out_le32(0x000001080110,0xC50C0000);
}
#elif defined(CONFIG_EMU_CFP)
void setup_tzc(void)
{
    out_le32(0x1100000 + 0x004,  0x00000001);
    out_le32(0x1100000 + 0x110,  0xc0000000);
    out_le32(0x1100000 + 0x114,  0xffffffff);
    out_le32(0x1100000 + 0x128,  0xfffff000);
    out_le32(0x1100000 + 0x12C,  0x000000ff);
    out_le32(0x1100000 + 0x130,  0xc0000001);
    out_le32(0x1100000 + 0x134,  0xffffffff);
    out_le32(0x1100000 + 0x008,  0x00000001);
}

void ddrmc_init(void)
{
	setup_tzc();

	out_le32(0x01080b28, 0x80040000);
	out_le32(0x01080b2c, 0x0000a101);
	out_le32(0x01080080, 0x80010322);
	out_le32(0x01080000, 0x000003ff);
	out_le32(0x010800c0, 0x00000000);
	out_le32(0x01080104, 0xd0550018);
	out_le32(0x01080108, 0xc9c6be44);
	out_le32(0x0108010c, 0x00590114);
	out_le32(0x01080100, 0x010b1000);
	out_le32(0x01080160, 0x00220000);
	out_le32(0x01080164, 0x02401400);
	out_le32(0x01080168, 0x00000000);
	out_le32(0x0108016c, 0x13300000);
	out_le32(0x01080250, 0x01334800);
	out_le32(0x01080170, 0x83020102);
	out_le32(0x01080250, 0x01334800);
	out_le32(0x01080174, 0x06550607);
	out_le32(0x01080190, 0x00000000);
	out_le32(0x01080194, 0x00000000);
	out_le32(0x01080110, 0x450c0005);
	out_le32(0x01080114, 0x00401000);
	out_le32(0x01080118, 0x01010215);
	out_le32(0x0108011c, 0x00100000);
	out_le32(0x01080124, 0x16da05b6);
	out_le32(0x01080f70, 0x30003000);
	out_le32(0x01080f08, 0x00000400);
	out_le32(0x01080f10, 0xff800000);
	out_le32(0x01080110, 0xc50c0005);

}
#endif

#ifdef CONFIG_EMU_DDR
int dram_init(void)
{
	puts("Skipping DDR init..\n");

	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;

	return 0;
}
#else
int fsl_initdram(void)
{
	puts("Initializing DDR....using Hardcoded settings\n");

	ddrmc_init();
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;

	return 0;
}
#endif
#else

#ifdef CONFIG_TARGET_LS1028ARDB
static phys_size_t fixed_sdram(void)
{
	size_t ddr_size;

#if defined(CONFIG_SPL_BUILD) || !defined(CONFIG_SPL)
	fsl_ddr_cfg_regs_t ddr_cfg_regs = {
		.cs[0].bnds		= 0x000000ff,
		.cs[0].config		= 0x80040422,
		.cs[0].config_2		= 0,
		.cs[1].bnds		= 0,
		.cs[1].config		= 0,
		.cs[1].config_2		= 0,

		.timing_cfg_3		= 0x01111000,
		.timing_cfg_0		= 0xd0550018,
		.timing_cfg_1		= 0xFAFC0C42,
		.timing_cfg_2		= 0x0048c114,
		.ddr_sdram_cfg		= 0xe50c000c,
		.ddr_sdram_cfg_2	= 0x00401110,
		.ddr_sdram_mode		= 0x01010230,
		.ddr_sdram_mode_2	= 0x0,

		.ddr_sdram_md_cntl	= 0x0600001f,
		.ddr_sdram_interval	= 0x18600618,
		.ddr_data_init		= 0xdeadbeef,

		.ddr_sdram_clk_cntl	= 0x02000000,
		.ddr_init_addr		= 0,
		.ddr_init_ext_addr	= 0,

		.timing_cfg_4		= 0x00000002,
		.timing_cfg_5		= 0x07401400,
		.timing_cfg_6		= 0x0,
		.timing_cfg_7		= 0x23300000,

		.ddr_zq_cntl		= 0x8A090705,
		.ddr_wrlvl_cntl		= 0x86550607,
		.ddr_sr_cntr		= 0,
		.ddr_sdram_rcw_1	= 0,
		.ddr_sdram_rcw_2	= 0,
		.ddr_wrlvl_cntl_2	= 0x0708080A,
		.ddr_wrlvl_cntl_3	= 0x0A0B0C09,

		.ddr_sdram_mode_9	= 0x00000400,
		.ddr_sdram_mode_10	= 0x04000000,

		.timing_cfg_8		= 0x06115600,

		.dq_map_0		= 0x5b65b658,
		.dq_map_1		= 0xd96d8000,
		.dq_map_2		= 0,
		.dq_map_3		= 0x01600000,

		.ddr_cdr1		= 0x80040000,
		.ddr_cdr2		= 0x000000C1
	};


	fsl_ddr_set_memctl_regs(&ddr_cfg_regs, 0, 0);
#endif
	ddr_size = 1ULL << 32;

	return ddr_size;
}
#endif

int fsl_initdram(void)
{
#ifdef CONFIG_TARGET_LS1028ARDB
	puts("Initializing DDR....using fixed timing\n");
	gd->ram_size = fixed_sdram();
#else
	puts("Initializing DDR....using SPD\n");
#if defined(CONFIG_SPL) && !defined(CONFIG_SPL_BUILD)
	gd->ram_size = fsl_ddr_sdram_size();
#else
	gd->ram_size = fsl_ddr_sdram();
#endif
#endif
	return 0;
}
#endif
