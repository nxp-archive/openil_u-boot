// SPDX-License-Identifier: GPL-2.0+
/*
 * Felix ethernet switch driver
 * Copyright 2018-2019 NXP
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <memalign.h>
#include <asm/io.h>
#include <pci.h>
#include <miiphy.h>
#include <dm/lists.h>
#include <ethsw.h>

/* defines especially around PCS are reused from enetc */
#include "fsl_enetc.h"

#define FELIX_PORT_DRV_NAME	"felix-port"

#define PCI_DEVICE_ID_FELIX_ETHSW	0xEEF0
#define FELIX_PM_IMDIO_BASE		0x8030

/* Max port count, including any internal ports */
#define FELIX_PORT_COUNT		6

/* Register map for BAR4 */
#define FELIX_SYS			0x010000
#define FELIX_ES0			0x040000
#define FELIX_IS1			0x050000
#define FELIX_IS2			0x060000
#define FELIX_GMII(port)		(0x100000 + (port) * 0x10000)
#define FELIX_QSYS			0x200000

#define FELIX_SYS_SYSTEM		(FELIX_SYS + 0x00000E00)
#define  FELIX_SYS_SYSTEM_EN		BIT(0)
#define FELIX_SYS_RAM_CTRL		(FELIX_SYS + 0x00000F24)
#define  FELIX_SYS_RAM_CTRL_INIT	BIT(1)

#define FELIX_ES0_TCAM_CTRL		(FELIX_ES0 + 0x000003C0)
#define  FELIX_ES0_TCAM_CTRL_EN		BIT(0)
#define FELIX_IS1_TCAM_CTRL		(FELIX_IS1 + 0x000003C0)
#define  FELIX_IS1_TCAM_CTRL_EN		BIT(0)
#define FELIX_IS2_TCAM_CTRL		(FELIX_IS2 + 0x000003C0)
#define  FELIX_IS2_TCAM_CTRL_EN		BIT(0)

#define FELIX_GMII_CLOCK_CFG(port)	(FELIX_GMII(port) + 0x00000000)
#define  FELIX_GMII_CLOCK_CFG_LINK_1G	1
#define  FELIX_GMII_CLOCK_CFG_LINK_100M	2
#define  FELIX_GMII_CLOCK_CFG_LINK_10M	3
#define FELIX_GMII_MAC_ENA_CFG(port)	(FELIX_GMII(port) + 0x0000001C)
#define  FELIX_GMII_MAX_ENA_CFG_TX	BIT(0)
#define  FELIX_GMII_MAX_ENA_CFG_RX	BIT(4)
#define FELIX_GMII_MAC_IFG_CFG(port)	(FELIX_GMII(port) + 0x0000001C + 0x14)
#define  FELIX_GMII_MAC_IFG_CFG_DEF	0x515

#define FELIX_QSYS_SYSTEM		(FELIX_QSYS + 0x0000F460)
#define FELIX_QSYS_SYSTEM_SW_PORT_MODE(port)	\
					(FELIX_QSYS_SYSTEM + 0x20 + (port) * 4)
#define  FELIX_QSYS_SYSTEM_SW_PORT_ENA		BIT(14)
#define  FELIX_QSYS_SYSTEM_SW_PORT_LOSSY	BIT(9)
#define  FELIX_QSYS_SYSTEM_SW_PORT_SCH(a)	(((a) & 0x3800) << 11)

/* internal MDIO in BAR0 */
#define FELIX_PM_IMDIO_BASE		0x8030

/* Serdes block on LS1028A */
#define FELIX_SERDES_BASE		0x1ea0000L
#define FELIX_SERDES_LNATECR0(lane)	(FELIX_SERDES_BASE + 0x818 + \
					 (lane) * 0x40)
#define  FELIX_SERDES_LNATECR0_ADPT_EQ	0x00003000
#define FELIX_SERDES_SGMIICR1(lane)	(FELIX_SERDES_BASE + 0x1804 + \
					 (lane) * 0x10)
#define  FELIX_SERDES_SGMIICR1_SGPCS	BIT(11)
#define  FELIX_SERDES_SGMIICR1_MDEV(a)	(((a) & 0x1f) << 27)

#define FELIX_PCS_CTRL			0
#define  FELIX_PCS_CTRL_RST		BIT(15)

struct felix_priv {
	void *regs_base;
	void *imdio_base;
	struct mii_dev imdio;
	struct udevice *port[FELIX_PORT_COUNT];
};

/* MDIO wrappers, we're using these to drive internal MDIO to get to serdes */
static int felix_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	struct enetc_mdio_priv priv;

	priv.regs_base = bus->priv;
	return enetc_mdio_read_priv(&priv, addr, devad, reg);
}

static int felix_mdio_write(struct mii_dev *bus, int addr, int devad, int reg,
			    u16 val)
{
	struct enetc_mdio_priv priv;

	priv.regs_base = bus->priv;
	return enetc_mdio_write_priv(&priv, addr, devad, reg, val);
}

/* returns port index for the given udev */
static int felix_port_index(struct udevice *port)
{
	u32 addr;

	if (ofnode_read_u32(port->node, "reg", &addr))
		return -1;

	return (int)addr;
}

/* set up serdes for SGMII */
static int felix_init_sgmii(struct udevice *port, int portno, int if_type)
{
	struct udevice *dev = port->parent;
	struct felix_priv *priv = dev_get_priv(dev);
	int addr = felix_port_index(port);
	bool is2500 = false;
	u16 reg;

	/* set up PCS lane address */
	out_le32(FELIX_SERDES_SGMIICR1(portno), FELIX_SERDES_SGMIICR1_SGPCS |
		 FELIX_SERDES_SGMIICR1_MDEV(portno));

	if (!priv->imdio.priv)
		return 0;

	if (if_type == PHY_INTERFACE_MODE_SGMII_2500)
		is2500 = true;

	/*
	 * Set to SGMII mode, for 1Gbps enable AN, for 2.5Gbps set fixed speed.
	 * Although fixed speed is 1Gbps, we could be running at 2.5Gbps based
	 * on PLL configuration.  Setting 1G for 2.5G here is counter intuitive
	 * but intentional.
	 */
	reg = ENETC_PCS_IF_MODE_SGMII;
	reg |= is2500 ? ENETC_PCS_IF_MODE_SPEED_1G : ENETC_PCS_IF_MODE_SGMII_AN;
	felix_mdio_write(&priv->imdio, addr, MDIO_DEVAD_NONE,
			 ENETC_PCS_IF_MODE, reg);

	/* Dev ability - SGMII */
	felix_mdio_write(&priv->imdio, addr, MDIO_DEVAD_NONE,
			 ENETC_PCS_DEV_ABILITY, ENETC_PCS_DEV_ABILITY_SGMII);

	/* Adjust link timer for SGMII */
	felix_mdio_write(&priv->imdio, addr, MDIO_DEVAD_NONE,
			 ENETC_PCS_LINK_TIMER1, ENETC_PCS_LINK_TIMER1_VAL);
	felix_mdio_write(&priv->imdio, addr, MDIO_DEVAD_NONE,
			 ENETC_PCS_LINK_TIMER2, ENETC_PCS_LINK_TIMER2_VAL);

	reg = ENETC_PCS_CR_DEF_VAL;
	reg |= is2500 ? ENETC_PCS_CR_RST : ENETC_PCS_CR_RESET_AN;
	/* restart PCS AN */
	felix_mdio_write(&priv->imdio, addr, MDIO_DEVAD_NONE,
			 ENETC_PCS_CR, reg);

	return 0;
}

/* set up MAC and serdes for (Q)SXGMII */
static int felix_init_sxgmii(struct udevice *port, int portno)
{
	struct udevice *dev = port->parent;
	struct felix_priv *priv = dev_get_priv(dev);
	int to = 1000;

	/* set up transit equalization control on serdes lane */
	out_le32(FELIX_SERDES_LNATECR0(1), FELIX_SERDES_LNATECR0_ADPT_EQ);

	if (!priv->imdio.priv)
		return 0;

	/*reset lane */
	felix_mdio_write(&priv->imdio, portno, MDIO_MMD_PCS, FELIX_PCS_CTRL,
			 FELIX_PCS_CTRL_RST);
	while (felix_mdio_read(&priv->imdio, portno, MDIO_MMD_PCS,
			       FELIX_PCS_CTRL) & FELIX_PCS_CTRL_RST &&
			--to) {
		mdelay(10);
	}
	if (felix_mdio_read(&priv->imdio, portno, MDIO_MMD_PCS,
			    FELIX_PCS_CTRL) & FELIX_PCS_CTRL_RST)
		dev_dbg(port, "PCS reset time-out\n");

	/* Dev ability - SXGMII */
	felix_mdio_write(&priv->imdio, portno, ENETC_PCS_DEVAD_REPL,
			 ENETC_PCS_DEV_ABILITY, ENETC_PCS_DEV_ABILITY_SXGMII);

	/* Restart PCS AN */
	felix_mdio_write(&priv->imdio, portno, ENETC_PCS_DEVAD_REPL,
			 ENETC_PCS_CR,
			 ENETC_PCS_CR_RST | ENETC_PCS_CR_RESET_AN);
	felix_mdio_write(&priv->imdio, portno, ENETC_PCS_DEVAD_REPL,
			 ENETC_PCS_REPL_LINK_TIMER_1,
			 ENETC_PCS_REPL_LINK_TIMER_1_DEF);
	felix_mdio_write(&priv->imdio, portno, ENETC_PCS_DEVAD_REPL,
			 ENETC_PCS_REPL_LINK_TIMER_2,
			 ENETC_PCS_REPL_LINK_TIMER_2_DEF);

	return 0;
}

/* Apply protocol specific configuration to MAC, serdes as needed */
static void felix_start_pcs(struct udevice *port, int portno)
{
	struct felix_priv *priv = dev_get_priv(port->parent);
	const char *if_str;
	int if_type;

	if_type = PHY_INTERFACE_MODE_NONE;

	/*
	 * set up internal MDIO, this is part of ETH PCI function and is
	 * used to access serdes / internal SoC PHYs.
	 * We don't currently register it as a MDIO bus as it goes away
	 * when the interface is removed, so it can't practically be
	 * used in the console.
	 */
	priv->imdio.read = felix_mdio_read;
	priv->imdio.write = felix_mdio_write;
	priv->imdio.priv = priv->imdio_base + FELIX_PM_IMDIO_BASE;
	strncpy(priv->imdio.name, port->parent->name, MDIO_NAME_LEN);

	if_str = ofnode_read_string(port->node, "phy-mode");
	if (if_str)
		if_type = phy_get_interface_by_name(if_str);
	else
		dev_dbg(port,
			"phy-mode property not found, defaulting to SGMII\n");
	if (if_type < 0)
		if_type = PHY_INTERFACE_MODE_NONE;

	switch (if_type) {
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_SGMII_2500:
	case PHY_INTERFACE_MODE_QSGMII:
		felix_init_sgmii(port, portno, if_type);
		break;
	case PHY_INTERFACE_MODE_XGMII:
	case PHY_INTERFACE_MODE_XFI:
	case PHY_INTERFACE_MODE_USXGMII:
		felix_init_sxgmii(port, portno);
		break;
	};
}

void felix_init(struct udevice *dev)
{
	struct felix_priv *priv = dev_get_priv(dev);
	ofnode port_node;
	void *base = priv->regs_base;
	int supported;
	int to = 100;
	u32 port;

	dev_dbg(dev, "trying to set up L2 switch\n");

	/* Init core memories */
	out_le32(base + FELIX_SYS_RAM_CTRL, FELIX_SYS_RAM_CTRL_INIT);
	while (in_le32(base + FELIX_SYS_RAM_CTRL) & FELIX_SYS_RAM_CTRL_INIT &&
	       --to)
		udelay(10);
	if (in_le32(base + FELIX_SYS_RAM_CTRL) & FELIX_SYS_RAM_CTRL_INIT)
		dev_dbg(dev, "Time-out waiting for switch memories\n");

	/* Start switch core, set up ES0, IS1, IS2 */
	out_le32(base + FELIX_SYS_SYSTEM, FELIX_SYS_SYSTEM_EN);
	out_le32(base + FELIX_ES0_TCAM_CTRL, FELIX_ES0_TCAM_CTRL_EN);
	out_le32(base + FELIX_IS1_TCAM_CTRL, FELIX_IS1_TCAM_CTRL_EN);
	out_le32(base + FELIX_IS2_TCAM_CTRL, FELIX_IS2_TCAM_CTRL_EN);
	udelay(20);

	supported = PHY_GBIT_FEATURES | SUPPORTED_2500baseX_Full;

	/* probe and set up ports */
	dev_for_each_subnode(port_node, dev) {
		const char *port_name;
		struct udevice *port_dev;
		struct phy_device *phy;

		if (!ofnode_is_available(port_node))
			continue;

		if (ofnode_read_u32(port_node, "reg", &port)) {
			dev_dbg(dev, "missing reg in %s\n",
				ofnode_get_name(port_node));
			continue;
		}
		if (port >= FELIX_PORT_COUNT) {
			dev_dbg(dev, "reg outside of range in %s\n",
				ofnode_get_name(port_node));
			continue;
		}

		port_name = ofnode_get_name(port_node);
		if (device_bind_driver_to_node(dev, FELIX_PORT_DRV_NAME,
					       port_name, port_node,
					       &port_dev)) {
			dev_dbg(dev, "can't bind driver to dev %s\n",
				ofnode_get_name(port_node));
			continue;
		}

		priv->port[port] = port_dev;

		/* Set up MAC registers */
		out_le32(base + FELIX_GMII_MAC_ENA_CFG(port),
			 FELIX_GMII_MAX_ENA_CFG_TX | FELIX_GMII_MAX_ENA_CFG_RX);
		out_le32(base + FELIX_GMII_CLOCK_CFG(port),
			 FELIX_GMII_CLOCK_CFG_LINK_1G);
		out_le32(base + FELIX_QSYS_SYSTEM_SW_PORT_MODE(port),
			 FELIX_QSYS_SYSTEM_SW_PORT_ENA |
			 FELIX_QSYS_SYSTEM_SW_PORT_LOSSY |
			 FELIX_QSYS_SYSTEM_SW_PORT_SCH(1));

		out_le32(base + FELIX_GMII_MAC_IFG_CFG(port),
			 FELIX_GMII_MAC_IFG_CFG_DEF);

		felix_start_pcs(port_dev, port);
		phy = dm_eth_phy_connect(port_dev);
		if (!phy) {
			dev_dbg(port_dev, "phy connect failed\n");
			continue;
		}

		phy->supported &= supported;
		phy->advertising &= supported;
		phy_config(phy);
	}
}

/*
 * Probe ENETC driver:
 * - initialize port and station interface BARs
 */
static int felix_probe(struct udevice *dev)
{
	struct felix_priv *priv = dev_get_priv(dev);

	if (ofnode_valid(dev->node) && !ofnode_is_available(dev->node)) {
		dev_dbg(dev, "switch disabled\n");
		return -ENODEV;
	}

	priv->imdio_base = dm_pci_map_bar(dev, PCI_BASE_ADDRESS_0, 0);
	if (!priv->imdio_base) {
		dev_dbg(dev, "failed to map BAR0\n");
		return -EINVAL;
	}

	priv->regs_base = dm_pci_map_bar(dev, PCI_BASE_ADDRESS_4, 0);
	if (!priv->regs_base) {
		dev_dbg(dev, "failed to map BAR4\n");
		return -EINVAL;
	}

	/* register internal MDIO for debug */
	if (!miiphy_get_dev_by_name(dev->name)) {
		struct mii_dev *mii_bus;

		mii_bus = mdio_alloc();
		mii_bus->read = felix_mdio_read;
		mii_bus->write = felix_mdio_write;
		mii_bus->priv = priv->imdio_base + FELIX_PM_IMDIO_BASE;
		strncpy(mii_bus->name, dev->name, MDIO_NAME_LEN);
		mdio_register(mii_bus);
	}

	dm_pci_clrset_config16(dev, PCI_COMMAND, 0, PCI_COMMAND_MEMORY);

	/* set up registers */
	felix_init(dev);

	return 0;
}

/*
 * I/O directly through switch ports is not supported in this driver, use
 * an ethernet interface connected to the switch instead.
 */
static int felix_start(struct udevice *dev)
{
	return -ENOTSUPP;
}

static void felix_stop(struct udevice *dev)
{
}

static const struct eth_ops felix_port_ops = {
	.start	= felix_start,
	.stop = felix_stop,
};

U_BOOT_DRIVER(felix_port) = {
	.name		= FELIX_PORT_DRV_NAME,
	.id		= UCLASS_ETH,
	.ops		= &felix_port_ops,
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
};

static const struct eth_ops felix_ethsw_ops = {
	.start	= felix_start,
	.stop = felix_stop,
};

U_BOOT_DRIVER(felix_ethsw) = {
	.name	= "felix_ethsw",
	.id	= UCLASS_ETH,
	.probe	= felix_probe,
	.ops    = &felix_ethsw_ops,
	.priv_auto_alloc_size = sizeof(struct felix_priv),
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
};

static struct pci_device_id felix_ethsw_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_FELIX_ETHSW) },
	{}
};

U_BOOT_PCI_DEVICE(felix_ethsw, felix_ethsw_ids);
