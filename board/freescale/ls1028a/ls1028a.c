/*
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <fsl_ddr.h>
#include <asm/io.h>
#include <hwconfig.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <environment.h>
#include <asm/arch-fsl-layerscape/soc.h>
#include <i2c.h>
#include <asm/arch/soc.h>
#ifdef CONFIG_FSL_LS_PPA
#include <asm/arch/ppa.h>
#endif
#include <fsl_immap.h>
#include <netdev.h>
#include <video_fb.h>

#include <fdtdec.h>
#include <miiphy.h>
#include "../common/qixis.h"

DECLARE_GLOBAL_DATA_PTR;

int config_board_mux(void)
{
#if defined(CONFIG_TARGET_LS1028AQDS) && defined(CONFIG_FSL_QIXIS)
	u8 reg;

	reg = QIXIS_READ(brdcfg[13]);
	/* Field| Function
	 * --------------------------------------------------------------
	 * 7-6  | Controls I2C3 routing (net CFG_MUX_I2C3):
	 * I2C3 | 10= Routes {SCL, SDA} to CAN1 transceiver as {TX, RX}.
	 * --------------------------------------------------------------
	 * 5-4  | Controls I2C4 routing (net CFG_MUX_I2C4):
	 * I2C4 |11= Routes {SCL, SDA} to CAN2 transceiver as {TX, RX}.
	 */
	reg &= ~(0xf0);
	reg |= 0xb0;
	QIXIS_WRITE(brdcfg[13], reg);

	reg = QIXIS_READ(brdcfg[15]);
	/* Field| Function
	 * --------------------------------------------------------------
	 * 7    | Controls the CAN1 transceiver (net CFG_CAN1_STBY):
	 * CAN1 | 0= CAN #1 transceiver enabled
	 * --------------------------------------------------------------
	 * 6    | Controls the CAN2 transceiver (net CFG_CAN2_STBY):
	 * CAN2 | 0= CAN #2 transceiver enabled
	 */
	reg &= ~(0xc0);
	QIXIS_WRITE(brdcfg[15], reg);
#endif
	return 0;
}

int board_init(void)
{
#ifdef CONFIG_ENV_IS_NOWHERE
	gd->env_addr = (ulong)&default_environment[0];
#endif

#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif

#ifndef CONFIG_SYS_EARLY_PCI_INIT
	/* run PCI init to kick off ENETC */
#if 1
	pci_init();
#endif
#endif

	return 0;
}

#if defined(CONFIG_ARCH_MISC_INIT)
int arch_misc_init(void)
{
	config_board_mux();

	return 0;
}
#endif

int board_eth_init(bd_t *bis)
{
	return pci_eth_init(bis);
}

int board_early_init_f(void)
{

#ifdef CONFIG_SYS_I2C_EARLY_INIT
	i2c_early_init_f();
#endif
	fsl_lsch3_early_init_f();
	return 0;
}

#ifndef CONFIG_EMU_DDR
void detail_board_ddr_info(void)
{
	puts("\nDDR    ");
	print_size(gd->bd->bi_dram[0].size + gd->bd->bi_dram[1].size, "");
	print_ddr_info(0);
}
#endif

#ifdef CONFIG_OF_BOARD_SETUP
#ifdef CONFIG_FSL_ENETC
extern void enetc_setup(void *blob);
#endif
int ft_board_setup(void *blob, bd_t *bd)
{
	u64 base[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];
	ft_cpu_setup(blob, bd);

	/* fixup DT for the two GPP DDR banks */
	base[0] = gd->bd->bi_dram[0].start;
	size[0] = gd->bd->bi_dram[0].size;
	base[1] = gd->bd->bi_dram[1].start;
	size[1] = gd->bd->bi_dram[1].size;

#ifdef CONFIG_RESV_RAM
	/* reduce size if reserved memory is within this bank */
	if (gd->arch.resv_ram >= base[0] &&
	    gd->arch.resv_ram < base[0] + size[0])
		size[0] = gd->arch.resv_ram - base[0];
	else if (gd->arch.resv_ram >= base[1] &&
		 gd->arch.resv_ram < base[1] + size[1])
		size[1] = gd->arch.resv_ram - base[1];
#endif
	fdt_fixup_memory_banks(blob, base, size, 2);

#ifdef CONFIG_FSL_ENETC
	enetc_setup(blob);
#endif
	return 0;
}
#endif

#if defined(CONFIG_FSL_ENETC) && defined(CONFIG_FSL_QIXIS)

#define MUX_INF(fmt, args...)	do {} while (0)
#define MUX_DBG(fmt, args...)	printf("MDIO MUX: " fmt, ##args)
#define MUX_ERR(fmt, args...)	printf("MDIO MUX: " fmt, ##args)
struct mdio_qixis_mux {
	struct mii_dev *bus;
	int select;
	int mask;
	int mdio_node;
};

void *mdio_mux_get_parent(struct mii_dev *bus)
{
	struct mdio_qixis_mux *priv = bus->priv;
	const char *mdio_name;
	struct mii_dev *parent_bus;

	mdio_name = fdt_get_name(gd->fdt_blob, priv->mdio_node, NULL);
	parent_bus = miiphy_get_dev_by_name(mdio_name);
	if (!parent_bus)
		MUX_ERR("couldn't find MDIO bus %s\n", mdio_name);
	MUX_INF("defer to parent bus %s\n", mdio_name);
	return parent_bus;
}

//sim doesn't actually have qixis, keep the code sane
//#define SIMULATOR
void mux_group_select(struct mdio_qixis_mux *priv)
{
	u8 brdcfg4, reg;
	static u8 oldsel;

	brdcfg4 = QIXIS_READ(brdcfg[4]);
	reg = brdcfg4;

	reg = (reg & ~priv->mask) | priv->select;

#ifndef SIMULATOR
	if (!(brdcfg4 ^ reg))
		return;

	udelay(100);
#else
	if (oldsel == reg)
		return;
	oldsel = reg;
#endif

	MUX_DBG(" qixis_write %02x\n", reg);
	QIXIS_WRITE(brdcfg[4], reg);
}

int mdio_mux_write(struct mii_dev *bus, int port_addr, int dev_addr,
		   int regnum, u16 value)
{
	struct mii_dev *parent_bus;

	parent_bus = mdio_mux_get_parent(bus);
	if (!parent_bus)
		return -ENODEV;

	mux_group_select(bus->priv);

	MUX_INF("write to parent MDIO\n");
	return parent_bus->write(parent_bus, port_addr, dev_addr, regnum, value);
}

int mdio_mux_read(struct mii_dev *bus, int port_addr, int dev_addr,
		  int regnum)
{
	struct mii_dev *parent_bus;

	parent_bus = mdio_mux_get_parent(bus);
	if (!parent_bus)
		return -ENODEV;

	mux_group_select(bus->priv);

	MUX_INF("read from parent MDIO\n");
	return parent_bus->read(parent_bus, port_addr, dev_addr, regnum);
}

int mdio_mux_reset(struct mii_dev *bus)
{
	return 0;
}

static void setup_mdio_mux_group(int offset, int mdio_node, int mask)
{
	const void *fdt = gd->fdt_blob;
	struct mdio_qixis_mux *group;
	int select;

	char path[32];

	fdt_get_path(fdt, offset, path, 32);
	MUX_INF("reading node %s\n", path);

	select = fdtdec_get_int(fdt, offset, "reg", -1);
	if (select < 0) {
		MUX_ERR("invalid selection word");
		return;
	}

	group = malloc(sizeof(struct mdio_qixis_mux));
	group->select = select;
	group->mask = mask;
	group->mdio_node = mdio_node;
	group->bus = mdio_alloc();
	if (!group->bus) {
		MUX_ERR("failed to allocate mdio bus\n");
		goto err;
	}
	group->bus->read = mdio_mux_read;
	group->bus->write = mdio_mux_write;
	group->bus->reset = mdio_mux_reset;
	strcpy(group->bus->name, fdt_get_name(fdt, offset, NULL));
	group->bus->priv = group;
	MUX_INF("register MUX ""%s"" sel=%x, mask=%x\n", group->bus->name, group->select, group->mask);
	mdio_register(group->bus);
	return;

err:
	free(group);
}

void setup_mdio_mux(void)
{
	const void *fdt = gd->fdt_blob;
	int offset, mux_node, mdio_node, mask;

	mux_node = fdt_path_offset(fdt, "/qixis/mdio-mux@54");
	if (mux_node < 0) {
		MUX_ERR("no MDIO MUX node\n");
		return;
	}

	mask = fdtdec_get_int(fdt, mux_node, "mux-mask", -1);
	if (mask < 0)
		MUX_ERR("invalid mux-mask\n");
	mdio_node = fdtdec_lookup_phandle(fdt, mux_node, "mdio-parent-bus");

	/* go through MUX nodes, register MDIO buses */
	for (offset = fdt_first_subnode(fdt, mux_node);
	     offset >= 0;
	     offset = fdt_next_subnode(fdt, offset)) {
		setup_mdio_mux_group(offset, mdio_node, mask);
	};
}
#endif /* #if defined(CONFIG_FSL_ENETC) && defined(CONFIG_FSL_QIXIS) */

#include <fsl_memac.h>

extern void memac_mdio_write22(struct mii_dev *bus, int port, int dev, int reg, int val);
extern int memac_mdio_read22(struct mii_dev *bus, int port, int dev, int reg);

extern int serdes_protocol;

void setup_4xSGMII(void)
{
#if defined(CONFIG_TARGET_LS1028AQDS)
	#define NETC_PF5_BAR0_BASE	0x1f8140000
	#define NETC_PF5_ECAM_BASE	0x1F0005000
	#define NETC_PCS_SGMIICR1(n)	(0x001ea1804 + (n) * 0x10)
	struct mii_dev bus = {0};
	u16 value;
	int i;

	if (serdes_protocol != 0x9999)
		return;

	printf("trying to set up 4xSGMII, this is hardcoded for SERDES 9999!!!!\n");

	out_le32(NETC_PCS_SGMIICR1(0), 0x00000000);
	out_le32(NETC_PCS_SGMIICR1(1), 0x08000000);
	out_le32(NETC_PCS_SGMIICR1(2), 0x10000000);
	out_le32(NETC_PCS_SGMIICR1(3), 0x18000000);

	/* turn on PCI function */
	out_le16(NETC_PF5_ECAM_BASE + 4, 0xffff);

	bus.priv = NETC_PF5_BAR0_BASE + 0x8030;

	for (i = 0; i < 4; i++) {
		value = PHY_SGMII_IF_MODE_SGMII | PHY_SGMII_IF_MODE_AN;
		memac_mdio_write22(&bus, i, MDIO_DEVAD_NONE, 0x14, value);
		/* Dev ability according to SGMII specification */
		value = PHY_SGMII_DEV_ABILITY_SGMII;
		memac_mdio_write22(&bus, i, MDIO_DEVAD_NONE, 0x04, value);
		/* Adjust link timer for SGMII */
		memac_mdio_write22(&bus, i, MDIO_DEVAD_NONE, 0x13, 0x0003);
		memac_mdio_write22(&bus, i, MDIO_DEVAD_NONE, 0x12, 0x06a0);
		value = memac_mdio_read22(&bus, i, MDIO_DEVAD_NONE, 1);
		value = memac_mdio_read22(&bus, i, MDIO_DEVAD_NONE, 1);
		printf("BMSR p%d %04x\n", i, (int)value);
	}
#endif
}

void setup_QSGMII(void)
{
	#define NETC_PF5_BAR0_BASE	0x1f8140000
	#define NETC_PF5_ECAM_BASE	0x1F0005000
	#define NETC_PCS_QSGMIICR1	0x001ea1884
	struct mii_dev bus = {0}, *ext_bus;
	u16 value;
	int i, to;

	if ((serdes_protocol & 0xf0) != 0x50)
		return;

	printf("trying to set up QSGMII, this is hardcoded for SERDES x5xx!!!!\n");

	out_le32(NETC_PCS_QSGMIICR1, 0x20000000);

	/* turn on PCI function */
	out_le16(NETC_PF5_ECAM_BASE + 4, 0xffff);

	bus.priv = NETC_PF5_BAR0_BASE + 0x8030;

	for (i = 4; i < 8; i++) {
		value = PHY_SGMII_IF_MODE_SGMII | PHY_SGMII_IF_MODE_AN;
		memac_mdio_write22(&bus, i, MDIO_DEVAD_NONE, 0x14, value);
		/* Dev ability according to SGMII specification */
		value = PHY_SGMII_DEV_ABILITY_SGMII;
		memac_mdio_write22(&bus, i, MDIO_DEVAD_NONE, 0x04, value);
		/* Adjust link timer for SGMII */
		memac_mdio_write22(&bus, i, MDIO_DEVAD_NONE, 0x13, 0x0003);
		memac_mdio_write22(&bus, i, MDIO_DEVAD_NONE, 0x12, 0x06a0);
	}


	int phy_addr;
	char *mdio_name;
 #if defined(CONFIG_TARGET_LS1028ARDB)
	mdio_name = "netc_mdio";
	phy_addr = 0x10;
#elif defined(CONFIG_TARGET_LS1028AQDS)
	mdio_name = "mdio@50";
	phy_addr = 0x08;
#endif

	/* set up VSC PHY - this works on RDB only for now*/
	ext_bus = miiphy_get_dev_by_name(mdio_name);
	if (!ext_bus) {
		printf("couldn't find MDIO bus, ignoring the PHY\n");
		return;
	}

	/* set up VSC PHY - this works on RDB only for now*/
	ext_bus = miiphy_get_dev_by_name(mdio_name);
	if (!ext_bus) {
		printf("couldn't find MDIO bus, ignoring the PHY\n");
		return;
	}

	for (i = phy_addr; i < phy_addr + 4; i++) {
		ext_bus->write(ext_bus, i, MDIO_DEVAD_NONE, 0x1f, 0x0010);
		value = ext_bus->read(ext_bus, i, MDIO_DEVAD_NONE, 0x13);
		value = (value & 0x3fff) | (1 << 14);
		ext_bus->write(ext_bus, i, MDIO_DEVAD_NONE, 0x12, 0x80e0);

		to = 1000;
		while (--to && (ext_bus->read(ext_bus, i, MDIO_DEVAD_NONE, 0x12) & 0x8000))
			;
		if (!to)
			printf("PHY%d TO\n", i);

		ext_bus->write(ext_bus, i, MDIO_DEVAD_NONE, 0x1f, 0x0000);
		value = ext_bus->read(ext_bus, i, MDIO_DEVAD_NONE, 0x17);
		value = (value & 0xf8ff);
		ext_bus->write(ext_bus, i, MDIO_DEVAD_NONE, 0x17, value);

		ext_bus->write(ext_bus, i, MDIO_DEVAD_NONE, 0x1f, 0x0003);
		value = ext_bus->read(ext_bus, i, MDIO_DEVAD_NONE, 0x10);
		value = value | 0x80;
		ext_bus->write(ext_bus, i, MDIO_DEVAD_NONE, 0x10, value);
		ext_bus->write(ext_bus, i, MDIO_DEVAD_NONE, 0x1f, 0x0000);
		ext_bus->write(ext_bus, 1, MDIO_DEVAD_NONE, 0x00, 0x3300);
	}

	for (i = 4; i < 8; i++) {
		to = 1000;
		while (--to && 0x0024 != ((value = memac_mdio_read22(&bus, i, MDIO_DEVAD_NONE, 1)) & 0x0024))
			continue;
		printf("BMSR p%d %04x\n", i, (int)value);
		if (0x0024 != (value & 0x0024))
			printk("QSGMII PCS%d TO\n", i);
	}
}

static void setup_1xSGMII(void)
{
	#define NETC_PF0_BAR0_BASE	0x1f8010000
	#define NETC_PF0_ECAM_BASE	0x1F0000000
	//#define NETC_PCS_SGMIICR1(n)	(0x001ea1804 + (n) * 0x10)
	struct mii_dev bus = {0};
	u16 value;

	if ((serdes_protocol & 0xf) != 0x0008)
		return;

	printf("trying to set up SGMII, this is hardcoded for SERDES 8xxx!!!!\n");

	//out_le32(NETC_PCS_SGMIICR1(0), 0x00000000);
	// writing this kills the link for some reason

	/* turn on PCI function */
	out_le16(NETC_PF0_ECAM_BASE + 4, 0xffff);

	bus.priv = NETC_PF0_BAR0_BASE + 0x8030;

	udelay(100);

	value = PHY_SGMII_IF_MODE_SGMII | PHY_SGMII_IF_MODE_AN;
	memac_mdio_write22(&bus, 0, MDIO_DEVAD_NONE, 0x14, value);
	/* Dev ability according to SGMII specification */
	value = PHY_SGMII_DEV_ABILITY_SGMII;
	memac_mdio_write22(&bus, 0, MDIO_DEVAD_NONE, 0x04, value);
	/* Adjust link timer for SGMII */
	memac_mdio_write22(&bus, 0, MDIO_DEVAD_NONE, 0x13, 0x0003);
	memac_mdio_write22(&bus, 0, MDIO_DEVAD_NONE, 0x12, 0x06a0);
	value = PHY_SGMII_CR_DEF_VAL | PHY_SGMII_CR_RESET_AN;
	memac_mdio_write22(&bus, 0, MDIO_DEVAD_NONE, 0x00, value);
	memac_mdio_read22(&bus, 0, MDIO_DEVAD_NONE, 0x01);
	value = memac_mdio_read22(&bus, 0, MDIO_DEVAD_NONE, 0x01);
	value = memac_mdio_read22(&bus, 0, MDIO_DEVAD_NONE, 0x01);
	if (!(value & 4))
		printf("\nSERDES lane didn't link up, status %04x\n", (int)value);
	printf("BMSR %04x\n", value);
}

#ifdef CONFIG_LAST_STAGE_INIT
int last_stage_init(void)
{
#if defined(CONFIG_FSL_ENETC) && defined(CONFIG_FSL_QIXIS)
	setup_mdio_mux();
#endif

	setup_1xSGMII();
	setup_4xSGMII();
	setup_QSGMII();
	return 0;
}
#endif

#ifdef CONFIG_TARGET_LS1028AQDS
int checkboard(void)
{
#ifdef CONFIG_FSL_QIXIS
	char buf[64];
#ifndef CONFIG_SD_BOOT
	u8 sw;
#endif
#endif

	puts("Board: LS1028AQDS, boot from ");

#ifdef CONFIG_FSL_QIXIS
#ifdef CONFIG_SD_BOOT
	puts("SD\n");
#elif defined(CONFIG_EMMC_BOOT)
	puts("eMMC card\n");
#else

	sw = QIXIS_READ(brdcfg[0]);

	switch (sw) {
	case 0x00:
		printf("Serial NOR flash\n");
		break;
	case 0x20:
		printf("Serial NAND flash\n");
		break;
	case 0x60:
		printf("QSPI Emulator\n");
		break;
	default:
		printf("invalid setting of SW: 0x%x\n", sw);
		break;
	}
#endif

	printf("Sys ID: 0x%02x, Sys Ver: 0x%02x\n",
	       QIXIS_READ(id), QIXIS_READ(arch));

	printf("FPGA:  v%d (%s), build %d\n",
	       (int)QIXIS_READ(scver), qixis_read_tag(buf),
	       (int)qixis_read_minor());
#endif

	return 0;
}
#endif



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
	printf("Board version: %c, boot from ", (sw & 0xf) + 'A' - 1);

	memset((u8 *)buf, 0x00, ARRAY_SIZE(buf));

	sw = QIXIS_READ(brdcfg[0]);
	sw = (sw & QIXIS_LBMAP_MASK) >> QIXIS_LBMAP_SHIFT;

#ifdef CONFIG_SD_BOOT
	puts("SD card\n");
#elif defined(CONFIG_EMMC_BOOT)
	puts("eMMC card\n");
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
#endif

void *video_hw_init(void)
{
}

#ifdef CONFIG_EMMC_BOOT
void *esdhc_get_base_addr(void)
{
	return (void *)CONFIG_SYS_FSL_ESDHC1_ADDR;
}

#endif
