/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/fsl_serdes.h>
#include <asm/arch/soc.h>
#include <fdt_support.h>
#include <hwconfig.h>
#include <ahci.h>
#include <mmc.h>
#include <scsi.h>
#include <fm_eth.h>
#include <fsl_esdhc.h>
#include <fsl_ifc.h>
#include <fsl_sec.h>
#ifndef CONFIG_TARGET_LS1028ARDB
#include "cpld.h"
#endif
#ifdef CONFIG_U_QE
#include <fsl_qe.h>
#endif
#include <asm/arch/ppa.h>

#ifdef CONFIG_TARGET_LS1028ARDB
#include "../common/qixis.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

int board_early_init_f(void)
{

#ifdef CONFIG_TARGET_LS1028ARDB
#ifdef CONFIG_SYS_I2C_EARLY_INIT
	i2c_early_init_f();
#endif
#endif
	fsl_lsch2_early_init_f();

	return 0;
}

#ifndef CONFIG_SPL_BUILD

#ifdef CONFIG_TARGET_LS1028ARDB
int checkboard(void)
{
	static const char *freq[2] = {"100.00", "Reserved"};

	char buf[64];
	u8 sw;
	int clock;

	printf("Board: LS1028A-RDB, ");

#ifdef CONFIG_FSL_QIXIS
	sw = QIXIS_READ(arch);
	printf("Board Arch: V%d, ", sw >> 4);
	printf("Board version: %c, boot from ", (sw & 0xf) + 'A');

	memset((u8 *)buf, 0x00, ARRAY_SIZE(buf));

	sw = QIXIS_READ(brdcfg[0]);
	sw = (sw & QIXIS_LBMAP_MASK) >> QIXIS_LBMAP_SHIFT;

#ifdef CONFIG_SD_BOOT
	puts("SD card\n");
#else
	switch (sw) {
	case 0:
	case 4:
		printf("NOR\n");
		break;
	case 1:
		printf("NAND\n");
		break;
	case 2:
	case 3:
		printf("EMU\n");
		break;
	default:
		printf("invalid setting of SW%u\n", QIXIS_LBMAP_SWITCH);
		break;
	}
#endif

	printf("FPGA: v%d.%d\n", QIXIS_READ(scver), QIXIS_READ(tagdata));

	puts("SERDES1 Reference : ");
	sw = QIXIS_READ(brdcfg[2]);
	clock = (sw >> 6) & 3;
	printf("Clock1 = %sMHz ", freq[clock]);
	clock = (sw >> 4) & 3;
	printf("Clock2 = %sMHz\n", freq[clock]);
#endif
	return 0;
}
#else
int checkboard(void)
{
	static const char *freq[2] = {"100.00MHZ", "156.25MHZ"};
#ifndef CONFIG_SD_BOOT
	u8 cfg_rcw_src1, cfg_rcw_src2;
	u16 cfg_rcw_src;
#endif
	u8 sd1refclk_sel;

	printf("Board: LS1043ARDB, boot from ");

#ifdef CONFIG_SD_BOOT
	puts("SD\n");
#else
	cfg_rcw_src1 = CPLD_READ(cfg_rcw_src1);
	cfg_rcw_src2 = CPLD_READ(cfg_rcw_src2);
	cpld_rev_bit(&cfg_rcw_src1);
	cfg_rcw_src = cfg_rcw_src1;
	cfg_rcw_src = (cfg_rcw_src << 1) | cfg_rcw_src2;

	if (cfg_rcw_src == 0x25)
		printf("vBank %d\n", CPLD_READ(vbank));
	else if (cfg_rcw_src == 0x106)
		puts("NAND\n");
	else
		printf("Invalid setting of SW4\n");
#endif

	printf("CPLD:  V%x.%x\nPCBA:  V%x.0\n", CPLD_READ(cpld_ver),
	       CPLD_READ(cpld_ver_sub), CPLD_READ(pcba_ver));

	puts("SERDES Reference Clocks:\n");
	sd1refclk_sel = CPLD_READ(sd1refclk_sel);
	printf("SD1_CLK1 = %s, SD1_CLK2 = %s\n", freq[sd1refclk_sel], freq[0]);

	return 0;
}
#endif

int board_init(void)
{
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;

#ifdef CONFIG_SYS_FSL_ERRATUM_A010315
	erratum_a010315();
#endif

#ifdef CONFIG_FSL_IFC
	init_final_memctl_regs();
#endif

#ifdef CONFIG_SECURE_BOOT
	/* In case of Secure Boot, the IBR configures the SMMU
	 * to allow only Secure transactions.
	 * SMMU must be reset in bypass mode.
	 * Set the ClientPD bit and Clear the USFCFG Bit
	 */
	u32 val;
	val = (in_le32(SMMU_SCR0) | SCR0_CLIENTPD_MASK) & ~(SCR0_USFCFG_MASK);
	out_le32(SMMU_SCR0, val);
	val = (in_le32(SMMU_NSCR0) | SCR0_CLIENTPD_MASK) & ~(SCR0_USFCFG_MASK);
	out_le32(SMMU_NSCR0, val);
#endif

#ifdef CONFIG_FSL_CAAM
	sec_init();
#endif

#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif

#ifdef CONFIG_U_QE
	u_qe_init();
#endif
	/* invert AQR105 IRQ pins polarity */
	out_be32(&scfg->intpcr, AQR105_IRQ_MASK);

	return 0;
}

struct sdversion_t{
	unsigned char   updateflag;
	unsigned char   updatepart;
	unsigned char   data[0x200 - 2];
};

int check_SDenv_version(void)
{
	struct mmc *mmc;
	uint blk_start, blk_cnt;
	unsigned long offset, size;
	struct sdversion_t sdversion_env;
	int dev = 0;

	offset	= 0x1FE000;
	size	= 0x200;
	mmc = find_mmc_device(dev);
	struct blk_desc *desc = mmc_get_blk_desc(mmc);
	blk_start   = ALIGN(offset, mmc->read_bl_len) / mmc->read_bl_len;
	blk_cnt     = ALIGN(size, mmc->read_bl_len) / mmc->read_bl_len;
	blk_dread(desc, blk_start, blk_cnt, (uchar *)&sdversion_env);
	if(sdversion_env.updateflag == '1')
	{
		printf("system is updating\n");
		sdversion_env.updateflag = '2';
		blk_dwrite(desc, blk_start, blk_cnt, (uchar *)&sdversion_env);
		return 0;
	}
	else if(sdversion_env.updateflag == '2')
	{
		sdversion_env.updateflag = '3';
		blk_dwrite(desc, blk_start, blk_cnt, (uchar *)&sdversion_env);
		if(sdversion_env.updatepart == '3')
		{
			env_set("bootfile", env_get("bootfile_old"));
			return 0;
		}
		else
			return 1;
	}
	else
		return 0;
}

int last_stage_init(void)
{
	if(check_SDenv_version())
		   env_set("bootcmd","run rollbackboot");
	
	/* enable watchdog and set timeout to 120s */
	writew( 0x34FF, WDOG1_BASE_ADDR);
	return 0;
}

int config_board_mux(void)
{
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;
	u32 usb_pwrfault;

	if (hwconfig("qe-hdlc")) {
		out_be32(&scfg->rcwpmuxcr0,
			 (in_be32(&scfg->rcwpmuxcr0) & ~0xff00) | 0x6600);
		printf("Assign to qe-hdlc clk, rcwpmuxcr0=%x\n",
		       in_be32(&scfg->rcwpmuxcr0));
	} else {
#ifdef CONFIG_HAS_FSL_XHCI_USB
		out_be32(&scfg->rcwpmuxcr0, 0x3333);
		out_be32(&scfg->usbdrvvbus_selcr, SCFG_USBDRVVBUS_SELCR_USB1);
		usb_pwrfault = (SCFG_USBPWRFAULT_DEDICATED <<
				SCFG_USBPWRFAULT_USB3_SHIFT) |
				(SCFG_USBPWRFAULT_DEDICATED <<
				SCFG_USBPWRFAULT_USB2_SHIFT) |
				(SCFG_USBPWRFAULT_SHARED <<
				 SCFG_USBPWRFAULT_USB1_SHIFT);
		out_be32(&scfg->usbpwrfault_selcr, usb_pwrfault);
#endif
	}
	return 0;
}

#if defined(CONFIG_MISC_INIT_R)
int misc_init_r(void)
{
	config_board_mux();
	return 0;
}
#endif

void fdt_del_qe(void *blob)
{
	int nodeoff = 0;

	while ((nodeoff = fdt_node_offset_by_compatible(blob, 0,
				"fsl,qe")) >= 0) {
		fdt_del_node(blob, nodeoff);
	}
}

int ft_board_setup(void *blob, bd_t *bd)
{
	u64 base[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	/* fixup DT for the two DDR banks */
	base[0] = gd->bd->bi_dram[0].start;
	size[0] = gd->bd->bi_dram[0].size;
	base[1] = gd->bd->bi_dram[1].start;
	size[1] = gd->bd->bi_dram[1].size;

	fdt_fixup_memory_banks(blob, base, size, 2);
	ft_cpu_setup(blob, bd);

#ifdef CONFIG_SYS_DPAA_FMAN
	fdt_fixup_fman_ethernet(blob);
#endif

	/*
	 * qe-hdlc and usb multi-use the pins,
	 * when set hwconfig to qe-hdlc, delete usb node.
	 */
	if (hwconfig("qe-hdlc"))
#ifdef CONFIG_HAS_FSL_XHCI_USB
		fdt_del_node_and_alias(blob, "usb1");
#endif
	/*
	 * qe just support qe-uart and qe-hdlc,
	 * if qe-uart and qe-hdlc are not set in hwconfig,
	 * delete qe node.
	 */
	if (!hwconfig("qe-uart") && !hwconfig("qe-hdlc"))
		fdt_del_qe(blob);

	return 0;
}

u8 flash_read8(void *addr)
{
	return __raw_readb(addr + 1);
}

void flash_write16(u16 val, void *addr)
{
	u16 shftval = (((val >> 8) & 0xff) | ((val << 8) & 0xff00));

	__raw_writew(shftval, addr);
}

u16 flash_read16(void *addr)
{
	u16 val = __raw_readw(addr);

	return (((val) >> 8) & 0x00ff) | (((val) << 8) & 0xff00);
}

#endif
