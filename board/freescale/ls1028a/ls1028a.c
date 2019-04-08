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
#ifndef CONFIG_LPUART
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
#endif
	return 0;
}

#ifdef CONFIG_LPUART
u32 get_lpuart_clk(void)
{
	return gd->bus_clk / CONFIG_SYS_FSL_LPUART_CLK_DIV;
}
#endif

int board_init(void)
{
#ifdef CONFIG_ENV_IS_NOWHERE
	gd->env_addr = (ulong)&default_environment[0];
#endif
#ifdef CONFIG_FSL_CAAM
	sec_init();
#endif
#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif

#ifndef CONFIG_SYS_EARLY_PCI_INIT
	/* run PCI init to kick off ENETC */
	pci_init();
#endif

#if defined(CONFIG_TARGET_LS1028ARDB)
	u8 val = I2C_MUX_CH_DEFAULT;

	i2c_write(I2C_MUX_PCA_ADDR_PRI, 0x0b, 1, &val, 1);
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
#ifdef CONFIG_LPUART
	u8 uart;
#endif

	fsl_lsch3_early_init_f();

#ifdef CONFIG_LPUART
	/* Field| Function
	 * --------------------------------------------------------------
	 * 7-6  | Controls I2C3 routing (net CFG_MUX_I2C3):
	 * I2C3 | 11= Routes {SCL, SDA} to LPUART1 header as {SOUT, SIN}.
	 * --------------------------------------------------------------
	 * 5-4  | Controls I2C4 routing (net CFG_MUX_I2C4):
	 * I2C4 |11= Routes {SCL, SDA} to LPUART1 header as {CTS_B, RTS_B}.
	 */
	/* use lpuart0 as system console */
	uart = QIXIS_READ(brdcfg[13]);
	uart &= ~CFG_LPUART_MUX_MASK;
	uart |= CFG_LPUART_EN;
	QIXIS_WRITE(brdcfg[13], uart);
#endif
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

#define DCFG_RCWSR13    0x130

int esdhc_status_fixup(void *blob, const char *compat)
{
	u32 __iomem *dcfg_ccsr = (u32 __iomem *)DCFG_BASE;
	char esdhc1_path[] = "/soc/esdhc@2150000";
	bool sdhc2_en = false;
	u8 mux_sdhc2;
	u8 io = 0;

	/*
	 * The PMUX IO-expander for mux select is used to control
	 * the muxing of various onboard interfaces.
	 *
	 * IO0[5:3] indicates SDHC2 interface demultiplexer
	 * select lines.
	 *      000 - eMMC Memory
	 *      001 - GPIO
	 *      010 - SPI2
	 *      011 - Data
	 *      101 - Reserved
	 *      110 - XSPI1_B
	 *      111 - Reserved
	 */

	io = in_le32(dcfg_ccsr + DCFG_RCWSR13);

	mux_sdhc2 = (io & 0x38) >> 3;
	/* Enable SDHC2 only when use eMMC */
	if (mux_sdhc2 == 0)
		sdhc2_en = true;

	if (!sdhc2_en)
		do_fixup_by_path(blob, esdhc1_path, "status", "disabled",
				sizeof("disabled"), 1);
	return 0;
}

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
#define MUX_DBG(fmt, args...)  do {} while (0)
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
#ifdef SIMULATOR
	static u8 oldsel;
#endif

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

extern int enetc_imdio_read(struct mii_dev *bus, int port, int dev, int reg);
extern void enetc_imdio_write(struct mii_dev *bus, int port, int dev, int reg, u16 val);

extern int serdes_protocol;

#define PCS_INF(fmt, args...)  do {} while (0)
#define PCS_ERR(fmt, args...)  printf("PCS: " fmt, ##args)
void setup_4xSGMII(void)
{
#if defined(CONFIG_TARGET_LS1028AQDS)
	#define NETC_PF5_BAR0_BASE	0x1f8140000
	#define NETC_PF5_ECAM_BASE	0x1F0005000
	#define NETC_PCS_SGMIICR1(n)	(0x001ea1804 + (n) * 0x10)
	struct mii_dev bus = {0};
	u16 value;
	int i, to;

	/* turn on PCI function */
	out_le16(NETC_PF5_ECAM_BASE + 4, 0xffff);
	bus.priv = (void *)NETC_PF5_BAR0_BASE + 0x8030;

	for (i = 0; i < 4; i++) {
		if (((serdes_protocol >> (i * 4)) & 0xf) != 0x9)
			continue;

		out_le32(NETC_PCS_SGMIICR1(i), 0x00000800 + 0x08000000 * i);

		value = PHY_SGMII_IF_MODE_SGMII | PHY_SGMII_IF_MODE_AN;
		enetc_imdio_write(&bus, i, MDIO_DEVAD_NONE, 0x14, value);
		/* Dev ability according to SGMII specification */
		value = PHY_SGMII_DEV_ABILITY_SGMII;
		enetc_imdio_write(&bus, i, MDIO_DEVAD_NONE, 0x04, value);
		/* Adjust link timer for SGMII */
		enetc_imdio_write(&bus, i, MDIO_DEVAD_NONE, 0x13, 0x0003);
		enetc_imdio_write(&bus, i, MDIO_DEVAD_NONE, 0x12, 0x06a0);

		/* restart AN */
		value = PHY_SGMII_CR_DEF_VAL | PHY_SGMII_CR_RESET_AN;
		enetc_imdio_write(&bus, i, MDIO_DEVAD_NONE, 0x00, value);

		/* wait for link */
		to = 1000;
		do {
			value = enetc_imdio_read(&bus, i,
						 MDIO_DEVAD_NONE, 0x01);
			if ((value & 0x0024) == 0x0024)
				break;
		} while (--to);
		PCS_INF("BMSR[%d]: %04x\n", i, value);
		if ((value & 0x0024) != 0x0024) {
			PCS_ERR("PCS[%d] didn't link up, giving up.\n", i);
			continue;
		}
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

	PCS_INF("trying to set up QSGMII, this is hardcoded for SERDES x5xx!!!!\n");

	//out_le32(NETC_PCS_QSGMIICR1, 0x20000000);

	/* turn on PCI function */
	out_le16(NETC_PF5_ECAM_BASE + 4, 0xffff);

	bus.priv = (void *)NETC_PF5_BAR0_BASE + 0x8030;

	for (i = 0; i < 4; i++) {
		value = PHY_SGMII_IF_MODE_SGMII | PHY_SGMII_IF_MODE_AN;
		enetc_imdio_write(&bus, i, MDIO_DEVAD_NONE, 0x14, value);
		/* Dev ability according to SGMII specification */
		value = PHY_SGMII_DEV_ABILITY_SGMII;
		enetc_imdio_write(&bus, i, MDIO_DEVAD_NONE, 0x04, value);
		/* Adjust link timer for SGMII */
		enetc_imdio_write(&bus, i, MDIO_DEVAD_NONE, 0x13, 0x0003);
		enetc_imdio_write(&bus, i, MDIO_DEVAD_NONE, 0x12, 0x06a0);
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
		PCS_ERR("couldn't find MDIO bus, skipping external PHY config\n");
		return;
	}

	for (i = phy_addr; i < phy_addr + 4; i++) {
		ext_bus->write(ext_bus, i, MDIO_DEVAD_NONE, 0x1f, 0x0010);
		value = ext_bus->read(ext_bus, i, MDIO_DEVAD_NONE, 0x13);
		value = (value & 0x3fff) | (1 << 14);
		ext_bus->write(ext_bus, i, MDIO_DEVAD_NONE, 0x12, 0x80e0);

		to = 1000;

		do {
			value = ext_bus->read(ext_bus, i, MDIO_DEVAD_NONE, 0x12);
			if (!(value & 0x8000))
				break;
		} while (--to);
		if (value & 0x8000)
			PCS_ERR("PHY[%d] reset timeout\n", i);


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

	for (i = 0; i < 4; i++) {
		to = 1000;
		do {
			value = enetc_imdio_read(&bus, i, MDIO_DEVAD_NONE, 1);
			if ((value & 0x0024) == 0x0024)
				break;
		} while (--to);
		PCS_INF("BMSR: %04x\n", value);
		if ((value & 0x24) != 0x24) {
			PCS_ERR("PCS[%d] didn't link up, giving up.\n", i);
			break;
		}
	}
}

static void setup_switch(void)
{

#define L2SW_PORTS	5

#define L2SW_BASE				0x1fc000000
#define L2SW_SYS				(L2SW_BASE + 0x010000)
#define L2SW_ES0				(L2SW_BASE + 0x040000)
#define L2SW_IS1				(L2SW_BASE + 0x050000)
#define L2SW_IS2				(L2SW_BASE + 0x060000)
#define L2SW_GMII(i)				(L2SW_BASE + 0x100000 + (i)*0x10000)
#define L2SW_QSYS				(L2SW_BASE + 0x200000)

#define L2SW_SYS_SYSTEM				(L2SW_SYS + 0x00000E00)
#define L2SW_SYS_RAM_CTRL			(L2SW_SYS + 0x00000F24)

#define L2SW_ES0_TCAM_CTRL			(L2SW_ES0 + 0x000003C0)
#define L2SW_IS1_TCAM_CTRL			(L2SW_IS1 + 0x000003C0)
#define L2SW_IS2_TCAM_CTRL			(L2SW_IS2 + 0x000003C0)

#define L2SW_GMII_CLOCK_CFG(i)			(L2SW_GMII(i) + 0x00000000)
#define L2SW_GMII_MAC_ENA_CFG(i)		(L2SW_GMII(i) + 0x0000001C)
#define L2SW_GMII_MAC_IFG_CFG(i)		(L2SW_GMII(i) + 0x0000001C + 0x14)

#define L2SW_QSYS_SYSTEM			(L2SW_QSYS + 0x0000F460)
#define L2SW_QSYS_SYSTEM_SWITCH_PORT_MODE(i)	(L2SW_QSYS_SYSTEM + 0x20 + (i)*4)


	int to, i;

	PCS_INF("trying to set up L2 switch\n");
	// Core memories
	out_le32(L2SW_SYS_RAM_CTRL, 0x2);
	to = 100;
	while (--to && (in_le32(L2SW_SYS_RAM_CTRL) & 0x2))
		udelay(1);
	if (in_le32(L2SW_SYS_RAM_CTRL) & 0x2)
		PCS_ERR("TO waiting for switch memories init\n");

	// Switch Core
	out_le32(L2SW_SYS_SYSTEM, 0x00000001);

	/* ES0 */
	out_le32(L2SW_ES0_TCAM_CTRL, 0x00000001);
	/* IS1 */
	out_le32(L2SW_IS1_TCAM_CTRL, 0x00000001);
	/* IS2 */
	out_le32(L2SW_IS2_TCAM_CTRL, 0x00000001);
	udelay(20);

	// Initialize the ports of the L2 switch
	for (i = 0; i < L2SW_PORTS; i++) {
		// MAC Tx and Rx
		out_le32(L2SW_GMII_MAC_ENA_CFG(i), 0x00000011);
		out_le32(L2SW_GMII_CLOCK_CFG(i), 0x00000001);
		out_le32(L2SW_QSYS_SYSTEM_SWITCH_PORT_MODE(i), 0x00004a00);
		out_le32(L2SW_GMII_MAC_IFG_CFG(i), 0x00000515);
	}
}

#define DEV_ABILITY_RSVD1	0x4000
#define DEV_ABILITY_DUP		0x1000
#define DEV_ABILITY_ABIL0	0x0001

#define CONTROL_RESET		0x8000
#define CONTROL_AN_EN		0x1000
#define CONTROL_RESTART_AN	0x0200

static void setup_SXGMII(void)
{
#if defined(CONFIG_TARGET_LS1028AQDS)
	#define NETC_PF0_BAR0_BASE	0x1f8010000
	#define NETC_PF0_ECAM_BASE	0x1F0000000
	#define NETC_IERB_BASE		0x1F0800000
	//#define NETC_PCS_SGMIICR1(n)	(0x001ea1804 + (n) * 0x10)
	struct mii_dev bus = {0}, *ext_bus;
	u16 value;
	int to;

	if ((serdes_protocol & 0xf) != 0x0001)
		return;

	PCS_INF("trying to set up SXGMII, this is hardcoded for SERDES 1xxx!!!!\n");

	// writing this kills the link for some reason
	//out_le32(NETC_PCS_SGMIICR1(0), 0x00000000);

	/* turn on PCI function */
	out_le16(NETC_PF0_ECAM_BASE + 4, 0xffff);

	/* indicate XGMII in boot-loader param reg */
	/* TODO: this has to be properly designed at some point */
	out_le32(0x1F0800000 + 0xd004, 0x80000000);

	/* set MAC in SXGMII mode */
	out_le32(NETC_PF0_BAR0_BASE + 0x8300, 0x00001000);

	bus.priv = (void *)NETC_PF0_BAR0_BASE + 0x8030;

	value = DEV_ABILITY_RSVD1 | DEV_ABILITY_DUP | DEV_ABILITY_ABIL0;
	enetc_imdio_write(&bus, 0, 0x1f, 0x4, value);
	value = CONTROL_RESET | CONTROL_AN_EN | CONTROL_RESTART_AN;
	enetc_imdio_write(&bus, 0, 0x1f, 0x0, value);
	to = 1000;
	do {
		value = enetc_imdio_read(&bus, 0, 0x1f, 0x0);
		if (!(value & 0x8000))
			break;
	} while (--to);
	if (value & 0x8000)
		PCS_ERR("PHY[0] reset timeout\n");

	/* start AN */
	value =  CONTROL_AN_EN | CONTROL_RESTART_AN;
	enetc_imdio_write(&bus, 0, 0x1f, 0x0, value);

//#if Aquantia config?
	int phy_addr = 2;
	char *mdio_name = "mdio@40";

	/* configure AQR PHY */
	ext_bus = miiphy_get_dev_by_name(mdio_name);
	if (!ext_bus) {
		PCS_ERR("couldn't find MDIO bus, ignoring the PHY\n");
		return;
	}

	if (ext_bus->read(ext_bus, phy_addr, 1, 2) == 0x03a1 &&
	    ext_bus->read(ext_bus, phy_addr, 1, 3) == 0xb662) {
		ext_bus->write(ext_bus, phy_addr, 4, 0xc441, 0x0008);
	} else {
		PCS_ERR("unknown PHY, no init done on it\n");
	}

//#if wait_for_linkup?
	to = 10000;
	do {
		value = enetc_imdio_read(&bus, 0, 0x1f, 0x1);
		if ((value & 0x24) == 0x24)
			break;
	} while (--to);
	PCS_INF("BMSR: %04x\n", value);
	if ((value & 0x24) != 0x24)
		PCS_ERR("PCS[0] didn't link up, giving up.\n");
#endif
}

/* tested with loopback card */
void setup_QSXGMII(void)
{
	#define NETC_PF5_BAR0_BASE	0x1f8140000
	#define NETC_PF5_ECAM_BASE	0x1F0005000
	struct mii_dev bus = {0}, *ext_bus;
	u16 value;
	int to, i;

#if defined(CONFIG_TARGET_LS1028AQDS)
	if ((serdes_protocol & 0xf0) != 0x0030)
		return;

	PCS_INF("trying to set up QSXGMII, this is hardcoded for SERDES x3xx!!!!\n");

	/* turn on PCI function */
	out_le16(NETC_PF5_ECAM_BASE + 4, 0xffff);

	bus.priv = (void *)NETC_PF5_BAR0_BASE + 0x8030;

	/* set up transit equalization control on lane 1*/
	out_le32(0x1ea0000 + 0x818 + 0x40, 0x00003000);

	/*reset lane */
	enetc_imdio_write(&bus, 0, 0x03, 0, 0x8000);
	to = 1000;
	do {
		if (enetc_imdio_read(&bus, 0, 0x03, 0) & 0x8000)
	break;
	} while (--to);

	/* set capabilities for AN  and restart AN*/
	for (i = 0; i < 4; i++) {
		enetc_imdio_write(&bus, i, 0x1f, 4, 0xd801);
		enetc_imdio_write(&bus, i, 0x1f, 0, 0x1200);
	}
//#if Aquantia config
	int phy_addr = 0;
	char *mdio_name = "mdio@50";

	/* configure AQR PHY */
	ext_bus = miiphy_get_dev_by_name(mdio_name);
	if (!ext_bus) {
		PCS_ERR("couldn't find MDIO bus, ignoring the PHY\n");
		return;
	}

	if (ext_bus->read(ext_bus, phy_addr, 1, 2) == 0x03a1 &&
	    ext_bus->read(ext_bus, phy_addr, 1, 3) == 0xb712) {
		for (i = phy_addr; i < phy_addr + 4; i++) {
			ext_bus->write(ext_bus, i, 4, 0xc441, 0x0008);
		}
	} else {
		PCS_ERR("unknown PHY, no init done on it\n");
	}

	for (i = 0; i < 4; i++) {
		to = 1000;
		do {
			value = enetc_imdio_read(&bus, i, 0x1f, 1);
			if ((value & 0x24) == 0x24)
				break;
		} while (--to);
		PCS_INF("BMSR %04x\n", value);
		if ((value & 0x24) != 0x24) {
			PCS_ERR("PCS[1:%d] didn't link up, giving up.\n", i);
			break;
		}
	}

#endif
}

static void setup_1xSGMII(void)
{
	#define NETC_PF0_BAR0_BASE	0x1f8010000
	#define NETC_PF0_ECAM_BASE	0x1F0000000
	#define NETC_PCS_SGMIICR1(n)	(0x001ea1804 + (n) * 0x10)
	struct mii_dev bus = {0};
	u16 value;
	int to;

	if ((serdes_protocol & 0xf) != 0x0008)
		return;

	PCS_INF("trying to set up SGMII, this is hardcoded for SERDES 8xxx!!!!\n");

	out_le32(NETC_PCS_SGMIICR1(0), 0x00000800);

	/* turn on PCI function */
	out_le16(NETC_PF0_ECAM_BASE + 4, 0xffff);


	bus.priv = (void *)NETC_PF0_BAR0_BASE + 0x8030;
	value = PHY_SGMII_IF_MODE_SGMII | PHY_SGMII_IF_MODE_AN;
	enetc_imdio_write(&bus, 0, MDIO_DEVAD_NONE, 0x14, value);
	/* Dev ability according to SGMII specification */
	value = PHY_SGMII_DEV_ABILITY_SGMII;
	enetc_imdio_write(&bus, 0, MDIO_DEVAD_NONE, 0x04, value);
	/* Adjust link timer for SGMII */
	enetc_imdio_write(&bus, 0, MDIO_DEVAD_NONE, 0x13, 0x0003);
	enetc_imdio_write(&bus, 0, MDIO_DEVAD_NONE, 0x12, 0x06a0);

	/* restart AN */
	value = PHY_SGMII_CR_DEF_VAL | PHY_SGMII_CR_RESET_AN;

	enetc_imdio_write(&bus, 0, MDIO_DEVAD_NONE, 0x00, value);
	/* wait for link */
	to = 1000;
	do {
		value = enetc_imdio_read(&bus, 0, MDIO_DEVAD_NONE, 0x01);
		if ((value & 0x0024) == 0x0024)
			break;
	} while (--to);
	PCS_INF("BMSR %04x\n", value);
	if ((value & 0x0024) != 0x0024)
		PCS_ERR("PCS[0] didn't link up, giving up.\n");
}


void setup_RGMII(void)
{
#if defined(CONFIG_TARGET_LS1028AQDS)
	#define NETC_PF1_BAR0_BASE	0x1f8050000
	#define NETC_PF1_ECAM_BASE	0x1F0001000
	struct mii_dev *ext_bus;
	char *mdio_name = "mdio@00";
	int phy_addr = 5;
	int value;

	PCS_INF("trying to set up RGMII\n");

	/* turn on PCI function */
	out_le16(NETC_PF1_ECAM_BASE + 4, 0xffff);
	out_le32(NETC_PF1_BAR0_BASE + 0x8300, 0x8006);

	/* configure AQR PHY */
	ext_bus = miiphy_get_dev_by_name(mdio_name);
	if (!ext_bus) {
		PCS_ERR("couldn't find MDIO bus, ignoring the PHY\n");
		return;
	}
	/* Atheros magic */
	ext_bus->write(ext_bus, phy_addr, MDIO_DEVAD_NONE, 0x0d, 0x0007);
	ext_bus->write(ext_bus, phy_addr, MDIO_DEVAD_NONE, 0x0e, 0x8016);
	ext_bus->write(ext_bus, phy_addr, MDIO_DEVAD_NONE, 0x0d, 0x4007);
	value = ext_bus->read(ext_bus, phy_addr, MDIO_DEVAD_NONE, 0x0e);
	if (value == 0xffff)
		goto phy_err;
	ext_bus->write(ext_bus, phy_addr, MDIO_DEVAD_NONE, 0x0e, value | 0x0018);
	ext_bus->write(ext_bus, phy_addr, MDIO_DEVAD_NONE, 0x1d, 0x0005);
	value = ext_bus->read(ext_bus, phy_addr, MDIO_DEVAD_NONE, 0x1e);
	if (value == 0xffff)
		goto phy_err;
	ext_bus->write(ext_bus, phy_addr, MDIO_DEVAD_NONE, 0x1e, value | 0x0100);
	/* restart AN */
	ext_bus->write(ext_bus, phy_addr, MDIO_DEVAD_NONE, 0x0d, 0x1200);

	return;
phy_err:
	PCS_ERR("RGMII PHY access error, giving up.\n");
#endif
}



#include "dm/device.h"
#include "../drivers/net/fsl_enetc.h"
extern void register_imdio(struct udevice *dev);
void setup_switch_mdio(void)
{
	struct udevice dev;
	struct enetc_devfn hw;

	dev.name = "sw";
	dev.priv = &hw;
	hw.port_regs = (void *)0x1f8140000;
	register_imdio(&dev);

}

#ifdef CONFIG_LAST_STAGE_INIT
int last_stage_init(void)
{
#if defined(CONFIG_FSL_ENETC) && defined(CONFIG_FSL_QIXIS)
	setup_mdio_mux();
#endif

	setup_switch_mdio();

	setup_1xSGMII();
	setup_4xSGMII();
	setup_QSGMII();
	setup_SXGMII();
	setup_RGMII();
#if 1
	setup_QSXGMII();
#endif
	setup_switch();
	return 0;
}
#endif

#ifdef CONFIG_FSL_QIXIS
int checkboard(void)
{
	u8 sw;
	int clock;
	char *board;
	char buf[64] = {0};
	static const char *freq[6] = {"100.00", "125.00", "156.25",
					"161.13", "322.26", "100.00 SS"};

	cpu_name(buf);
	/* find the board details */
	sw = QIXIS_READ(id);
	switch (sw) {
	case 0x46:
		board = "QDS";
		break;
	case 0x47:
		board = "RDB";
		break;
	case 0x49:
		board = "HSSI";
		break;
	default:
		board = "unknown";
		break;
	}

	sw = QIXIS_READ(arch);
	printf("Board: %s-%s, Version: %c, boot from ",
	       buf, board, (sw & 0xf) + 'A' - 1);

	sw = QIXIS_READ(brdcfg[0]);
	sw = (sw & QIXIS_LBMAP_MASK) >> QIXIS_LBMAP_SHIFT;

#ifdef CONFIG_SD_BOOT
	puts("SD\n");
#elif defined(CONFIG_EMMC_BOOT)
	puts("eMMC\n");
#else
	switch (sw) {
	case 0:
	case 4:
		printf("NOR\n");
		break;
	case 1:
		printf("NAND\n");
		break;
#ifndef CONFIG_TARGET_LS1028AQDS
	case 2:
#endif
	case 3:
		printf("EMU\n");
		break;
	default:
		printf("invalid setting of SW%u\n", QIXIS_LBMAP_SWITCH);
		break;
	}
#endif

	printf("FPGA: v%d (%s: %s_%s)\n", QIXIS_READ(scver),
	       !qixis_read_released()  ? "INTERIM" : "RELEASED",
			board, qixis_read_date(buf));

	puts("SERDES1 Reference : ");
	sw = QIXIS_READ(brdcfg[2]);
#ifdef CONFIG_TARGET_LS1028ARDB
	clock = (sw >> 6) & 3;
#else
	clock = (sw >> 4) & 0xf;
#endif

	printf("Clock1 = %sMHz ", freq[clock]);
#ifdef CONFIG_TARGET_LS1028ARDB
	clock = (sw >> 4) & 3;
#else
	clock = sw & 0xf;
#endif
	printf("Clock2 = %sMHz\n", freq[clock]);

	return 0;
}
#endif

void qixis_dump_switch(void)
{
#ifdef CONFIG_FSL_QIXIS
	int i, nr_of_cfgsw;

	QIXIS_WRITE(cms[0], 0x00);
	nr_of_cfgsw = QIXIS_READ(cms[1]);

	puts("DIP switch settings dump:\n");
	for (i = 1; i <= nr_of_cfgsw; i++) {
		QIXIS_WRITE(cms[0], i);
		printf("SW%d = (0x%02x)\n", i, QIXIS_READ(cms[1]));
	}
#endif
}

void *video_hw_init(void)
{
	return NULL;
}

#ifdef CONFIG_EMMC_BOOT
void *esdhc_get_base_addr(void)
{
	return (void *)CONFIG_SYS_FSL_ESDHC1_ADDR;
}

#endif
