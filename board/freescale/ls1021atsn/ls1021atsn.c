/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/arch/immap_ls102xa.h>
#include <asm/arch/clock.h>
#include <asm/arch/fsl_serdes.h>
#include <asm/arch/ls102xa_devdis.h>
#include <asm/arch/ls102xa_soc.h>
#include <asm/arch/ls102xa_sata.h>
#include <hwconfig.h>
#include <mmc.h>
#include <fsl_csu.h>
#include <fsl_esdhc.h>
#include <fsl_ifc.h>
#include <fsl_immap.h>
#include <netdev.h>
#include <fsl_mdio.h>
#include <tsec.h>
#include <fsl_sec.h>
#include <fsl_devdis.h>
#include <spl.h>
#include "../common/sleep.h"
#ifdef CONFIG_U_QE
#include <fsl_qe.h>
#endif
#include <fsl_validate.h>
#include <asm/tzasc380.h>

DECLARE_GLOBAL_DATA_PTR;

#define VERSION_MASK		0x00FF
#define BANK_MASK		0x0001
#define CONFIG_RESET		0x1
#define INIT_RESET		0x1

#define CPLD_SET_MUX_SERDES	0x20
#define CPLD_SET_BOOT_BANK	0x40

#define BOOT_FROM_UPPER_BANK	0x0
#define BOOT_FROM_LOWER_BANK	0x1

#define LANEB_SATA		(0x01)
#define LANEB_SGMII1		(0x02)
#define LANEC_SGMII1		(0x04)
#define LANEC_PCIEX1		(0x08)
#define LANED_PCIEX2		(0x10)
#define LANED_SGMII2		(0x20)

#define MASK_LANE_B		0x1
#define MASK_LANE_C		0x2
#define MASK_LANE_D		0x4
#define MASK_SGMII		0x8

#define KEEP_STATUS		0x0
#define NEED_RESET		0x1

#define SOFT_MUX_ON_I2C3_IFC	0x2
#define SOFT_MUX_ON_CAN3_USB2	0x8
#define SOFT_MUX_ON_QE_LCD	0x10

#define PIN_I2C3_IFC_MUX_I2C3	0x0
#define PIN_I2C3_IFC_MUX_IFC	0x1
#define PIN_CAN3_USB2_MUX_USB2	0x0
#define PIN_CAN3_USB2_MUX_CAN3	0x1
#define PIN_QE_LCD_MUX_LCD	0x0
#define PIN_QE_LCD_MUX_QE	0x1


#if !defined(CONFIG_QSPI_BOOT) && !defined(CONFIG_SD_BOOT_QSPI)
static void convert_serdes_mux(int type, int need_reset);

void cpld_show(void)
{
	struct ccsr_gur *dcfg = (struct ccsr_gur *)CONFIG_SYS_FSL_GUTS_ADDR;
	u32 cpldrev;

	cpldrev = in_be32(&dcfg->gpporcr1);

	printf("CPLD:  V%d.%d\n", ((cpldrev >> 28) & 0xf), ((cpldrev >> 24) & 0xf));

	return;
}
#endif

int checkboard(void)
{
	puts("Board: LS1021ATSN\n");
#if !defined(CONFIG_QSPI_BOOT) && !defined(CONFIG_SD_BOOT_QSPI)
	cpld_show();
#endif

	return 0;
}

void ddrmc_init(void)
{
	struct ccsr_ddr *ddr = (struct ccsr_ddr *)CONFIG_SYS_FSL_DDR_ADDR;
	u32 temp_sdram_cfg, tmp;

	out_be32(&ddr->sdram_cfg, DDR_SDRAM_CFG);

	out_be32(&ddr->cs0_bnds, DDR_CS0_BNDS);
	out_be32(&ddr->cs0_config, DDR_CS0_CONFIG);

	out_be32(&ddr->timing_cfg_0, DDR_TIMING_CFG_0);
	out_be32(&ddr->timing_cfg_1, DDR_TIMING_CFG_1);
	out_be32(&ddr->timing_cfg_2, DDR_TIMING_CFG_2);
	out_be32(&ddr->timing_cfg_3, DDR_TIMING_CFG_3);
	out_be32(&ddr->timing_cfg_4, DDR_TIMING_CFG_4);
	out_be32(&ddr->timing_cfg_5, DDR_TIMING_CFG_5);

#ifdef CONFIG_DEEP_SLEEP
	if (is_warm_boot()) {
		out_be32(&ddr->sdram_cfg_2,
			 DDR_SDRAM_CFG_2 & ~SDRAM_CFG2_D_INIT);
		out_be32(&ddr->init_addr, CONFIG_SYS_SDRAM_BASE);
		out_be32(&ddr->init_ext_addr, (1 << 31));

		/* DRAM VRef will not be trained */
		out_be32(&ddr->ddr_cdr2,
			 DDR_DDR_CDR2 & ~DDR_CDR2_VREF_TRAIN_EN);
	} else
#endif
	{
		out_be32(&ddr->sdram_cfg_2, DDR_SDRAM_CFG_2);
		out_be32(&ddr->ddr_cdr2, DDR_DDR_CDR2);
	}

	out_be32(&ddr->sdram_mode, DDR_SDRAM_MODE);
	out_be32(&ddr->sdram_mode_2, DDR_SDRAM_MODE_2);

	out_be32(&ddr->sdram_interval, DDR_SDRAM_INTERVAL);

	out_be32(&ddr->ddr_wrlvl_cntl, DDR_DDR_WRLVL_CNTL);

	out_be32(&ddr->ddr_wrlvl_cntl_2, DDR_DDR_WRLVL_CNTL_2);
	out_be32(&ddr->ddr_wrlvl_cntl_3, DDR_DDR_WRLVL_CNTL_3);

	out_be32(&ddr->ddr_cdr1, DDR_DDR_CDR1);

	out_be32(&ddr->sdram_clk_cntl, DDR_SDRAM_CLK_CNTL);
	out_be32(&ddr->ddr_zq_cntl, DDR_DDR_ZQ_CNTL);

	out_be32(&ddr->cs0_config_2, DDR_CS0_CONFIG_2);

	/* DDR erratum A-009942 */
	tmp = in_be32(&ddr->debug[28]);
	out_be32(&ddr->debug[28], tmp | 0x0070006f);

	udelay(1);

#ifdef CONFIG_DEEP_SLEEP
	if (is_warm_boot()) {
		/* enter self-refresh */
		temp_sdram_cfg = in_be32(&ddr->sdram_cfg_2);
		temp_sdram_cfg |= SDRAM_CFG2_FRC_SR;
		out_be32(&ddr->sdram_cfg_2, temp_sdram_cfg);

		temp_sdram_cfg = (DDR_SDRAM_CFG_MEM_EN | SDRAM_CFG_BI);
	} else
#endif
		temp_sdram_cfg = (DDR_SDRAM_CFG_MEM_EN & ~SDRAM_CFG_BI);

	out_be32(&ddr->sdram_cfg, DDR_SDRAM_CFG | temp_sdram_cfg);

#ifdef CONFIG_DEEP_SLEEP
	if (is_warm_boot()) {
		/* exit self-refresh */
		temp_sdram_cfg = in_be32(&ddr->sdram_cfg_2);
		temp_sdram_cfg &= ~SDRAM_CFG2_FRC_SR;
		out_be32(&ddr->sdram_cfg_2, temp_sdram_cfg);
	}
#endif
}

int dram_init(void)
{
#if (!defined(CONFIG_SPL) || defined(CONFIG_SPL_BUILD))
	ddrmc_init();
#endif

	gd->ram_size = PHYS_SDRAM_SIZE;

#if defined(CONFIG_DEEP_SLEEP) && !defined(CONFIG_SPL_BUILD)
	fsl_dp_resume();
#endif

#if 1
#ifdef	CONFIG_ARMV7_TEE
#define	CSU_SEC_ACCESS_REG_OFFSET	(0x21c/4)
#define	TZASC_BYPASS_MUX_DISABLE	0x4
#define	CCI_TERMINATE_BARRIER_TX	0x8


	/* Configure CCI control override register to terminate all barrier transactions */
	out_le32(((u32 *)CONFIG_SYS_CCI400_ADDR), CCI_TERMINATE_BARRIER_TX);
	/* Configure CSU secure access register to disable TZASC bypass mux */
	out_be32(((u32 *)((u32 *)CONFIG_SYS_FSL_CSU_ADDR + CSU_SEC_ACCESS_REG_OFFSET)), TZASC_BYPASS_MUX_DISABLE);
	/* Set security permissions for region 0 */
	tzasc_set_region(CONFIG_SYS_FSL_TZASC_ADDR, 0, 0, 0, 0, 0, TZASC_REGION_SECURITY_NSRW, 0);

	/* Set region 1 */
	tzasc_set_region(CONFIG_SYS_FSL_TZASC_ADDR, 1, TZASC_REGION_ENABLED, CONFIG_OPTEE_ENTRY,
			0, TZASC_REGION_SIZE_64MB, TZASC_REGION_SECURITY_SRW, 0x80);

	/* Set region 2 */
#define	TEE_RAM_UPPER_SUBREGION_OFFSET	0x03800000
	tzasc_set_region(CONFIG_SYS_FSL_TZASC_ADDR, 2, TZASC_REGION_ENABLED, (CONFIG_OPTEE_ENTRY + TEE_RAM_UPPER_SUBREGION_OFFSET),
	0, TZASC_REGION_SIZE_8MB, TZASC_REGION_SECURITY_SRW, 0xc0);
#endif
#endif
	return 0;
}

#ifdef CONFIG_FSL_ESDHC
struct fsl_esdhc_cfg esdhc_cfg[1] = {
	{CONFIG_SYS_FSL_ESDHC_ADDR},
};

int board_mmc_init(bd_t *bis)
{
	esdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);

	return fsl_esdhc_initialize(bis, &esdhc_cfg[0]);
}
#endif

int board_eth_init(bd_t *bis)
{
#ifdef CONFIG_TSEC_ENET
	struct fsl_pq_mdio_info mdio_info;
	struct tsec_info_struct tsec_info[4];
	int num = 0;

#ifdef CONFIG_TSEC1
	SET_STD_TSEC_INFO(tsec_info[num], 1);
	if (is_serdes_configured(SGMII_TSEC1)) {
		puts("eTSEC1 is in sgmii mode.\n");
		tsec_info[num].flags |= TSEC_SGMII;
	}
	num++;
#endif
#ifdef CONFIG_TSEC2
	SET_STD_TSEC_INFO(tsec_info[num], 2);
	if (is_serdes_configured(SGMII_TSEC2)) {
		puts("eTSEC2 is in sgmii mode.\n");
		tsec_info[num].flags |= TSEC_SGMII;
	}
	num++;
#endif
#ifdef CONFIG_TSEC3
	SET_STD_TSEC_INFO(tsec_info[num], 3);
	num++;
#endif
	if (!num) {
		printf("No TSECs initialized\n");
		return 0;
	}

	mdio_info.regs = (struct tsec_mii_mng *)CONFIG_SYS_MDIO_BASE_ADDR;
	mdio_info.name = DEFAULT_MII_NAME;
	fsl_pq_mdio_init(bis, &mdio_info);

	tsec_eth_init(bis, tsec_info, num);
#endif

	return pci_eth_init(bis);
}

#if !defined(CONFIG_QSPI_BOOT) && !defined(CONFIG_SD_BOOT_QSPI)
int config_serdes_mux(void)
{
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	u32 protocol = in_be32(&gur->rcwsr[4]) & RCWSR4_SRDS1_PRTCL_MASK;

	protocol >>= RCWSR4_SRDS1_PRTCL_SHIFT;
	switch (protocol) {
	case 0x10:
		convert_serdes_mux(LANEB_SATA, KEEP_STATUS);
		convert_serdes_mux(LANED_PCIEX2 |
				LANEC_PCIEX1, KEEP_STATUS);
		break;
	case 0x20:
		convert_serdes_mux(LANEB_SGMII1, KEEP_STATUS);
		convert_serdes_mux(LANEC_PCIEX1, KEEP_STATUS);
		convert_serdes_mux(LANED_SGMII2, KEEP_STATUS);
		break;
	case 0x30:
		convert_serdes_mux(LANEB_SATA, KEEP_STATUS);
		convert_serdes_mux(LANEC_SGMII1, KEEP_STATUS);
		convert_serdes_mux(LANED_SGMII2, KEEP_STATUS);
		break;
	case 0x70:
		convert_serdes_mux(LANEB_SATA, KEEP_STATUS);
		convert_serdes_mux(LANEC_PCIEX1, KEEP_STATUS);
		convert_serdes_mux(LANED_SGMII2, KEEP_STATUS);
		break;
	}

	return 0;
}
#endif

int board_early_init_f(void)
{
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;

#ifdef CONFIG_TSEC_ENET
	/* clear BD & FR bits for BE BD's and frame data */
	clrbits_be32(&scfg->etsecdmamcr, SCFG_ETSECDMAMCR_LE_BD_FR);
	out_be32(&scfg->etsecmcr, SCFG_ETSECCMCR_GE2_CLK125);
#endif

#ifdef CONFIG_FSL_IFC
	init_early_memctl_regs();
#endif

	arch_soc_init();

#if defined(CONFIG_DEEP_SLEEP)
	if (is_warm_boot()) {
		timer_init();
		dram_init();
	}
#endif

	return 0;
}

#ifdef CONFIG_SPL_BUILD
void board_init_f(ulong dummy)
{
	void (*second_uboot)(void);

	/* Clear the BSS */
	memset(__bss_start, 0, __bss_end - __bss_start);

	get_clocks();

#if defined(CONFIG_DEEP_SLEEP)
	if (is_warm_boot())
		fsl_dp_disable_console();
#endif

	preloader_console_init();

	dram_init();

	/* Allow OCRAM access permission as R/W */
#ifdef CONFIG_LAYERSCAPE_NS_ACCESS
	enable_layerscape_ns_access();
	enable_layerscape_ns_access();
#endif

	/*
	 * if it is woken up from deep sleep, then jump to second
	 * stage uboot and continue executing without recopying
	 * it from SD since it has already been reserved in memeory
	 * in last boot.
	 */
	if (is_warm_boot()) {
		second_uboot = (void (*)(void))CONFIG_SYS_TEXT_BASE;
		second_uboot();
	}

	board_init_r(NULL, 0);
}
#endif


int board_init(void)
{
#ifndef CONFIG_SYS_FSL_NO_SERDES
	fsl_serdes_init();
#if !defined(CONFIG_QSPI_BOOT) && !defined(CONFIG_SD_BOOT_QSPI)
	config_serdes_mux();
#endif
#endif
	ls102xa_smmu_stream_id_init();

#ifdef CONFIG_LAYERSCAPE_NS_ACCESS
	enable_layerscape_ns_access();
#endif

#ifdef CONFIG_U_QE
	u_qe_init();
#endif

	return 0;
}

#if defined(CONFIG_SPL_BUILD)
void spl_board_init(void)
{
	ls102xa_smmu_stream_id_init();
}
#endif

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#ifdef CONFIG_SCSI_AHCI_PLAT
	ls1021a_sata_init();
#endif
#ifdef CONFIG_CHAIN_OF_TRUST
	fsl_setenv_chain_of_trust();
#endif

	return 0;
}
#endif

#if defined(CONFIG_MISC_INIT_R)
int misc_init_r(void)
{
#ifdef CONFIG_FSL_DEVICE_DISABLE
	device_disable(devdis_tbl, ARRAY_SIZE(devdis_tbl));
#endif

#ifdef CONFIG_FSL_CAAM
	return sec_init();
#endif
}
#endif

#if defined(CONFIG_DEEP_SLEEP)
void board_sleep_prepare(void)
{
#ifdef CONFIG_LAYERSCAPE_NS_ACCESS
	enable_layerscape_ns_access();
#endif
}
#endif

int ft_board_setup(void *blob, bd_t *bd)
{
	ft_cpu_setup(blob, bd);

#ifdef CONFIG_PCI
	ft_pci_setup(blob, bd);
#endif

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
