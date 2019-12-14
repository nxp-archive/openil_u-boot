// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2009-2010, 2013 Freescale Semiconductor, Inc.
 *	Jun-jie Zhang <b18070@freescale.com>
 *	Mingkai Hu <Mingkai.hu@freescale.com>
 */

#include <common.h>
#include <miiphy.h>
#include <phy.h>
#include <miiphy.h>
#include <fsl_mdio.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <tsec.h>

void tsec_local_mdio_write(struct tsec_mii_mng __iomem *phyregs, int port_addr,
		int dev_addr, int regnum, int value)
{
	int timeout = 1000000;

	out_be32(&phyregs->miimadd, (port_addr << 8) | (regnum & 0x1f));
	out_be32(&phyregs->miimcon, value);
	/* Memory barrier */
	mb();

	while ((in_be32(&phyregs->miimind) & MIIMIND_BUSY) && timeout--)
		;
}

int tsec_local_mdio_read(struct tsec_mii_mng __iomem *phyregs, int port_addr,
		int dev_addr, int regnum)
{
	int value;
	int timeout = 1000000;

	/* Put the address of the phy, and the register number into MIIMADD */
	out_be32(&phyregs->miimadd, (port_addr << 8) | (regnum & 0x1f));

	/* Clear the command register, and wait */
	out_be32(&phyregs->miimcom, 0);
	/* Memory barrier */
	mb();

	/* Initiate a read command, and wait */
	out_be32(&phyregs->miimcom, MIIMCOM_READ_CYCLE);
	/* Memory barrier */
	mb();

	/* Wait for the the indication that the read is done */
	while ((in_be32(&phyregs->miimind) & (MIIMIND_NOTVALID | MIIMIND_BUSY))
			&& timeout--)
		;

	/* Grab the value read from the PHY */
	value = in_be32(&phyregs->miimstat);

	return value;
}

int fsl_pq_mdio_reset(struct tsec_mii_mng __iomem *regs)
{
	/* Reset MII (due to new addresses) */
	out_be32(&regs->miimcfg, MIIMCFG_RESET_MGMT);

	out_be32(&regs->miimcfg, MIIMCFG_INIT_VALUE);

	while (in_be32(&regs->miimind) & MIIMIND_BUSY)
		;

	return 0;
}

#ifndef CONFIG_DM_MDIO
int tsec_phy_read(struct mii_dev *bus, int addr, int dev_addr, int regnum)
{
	struct tsec_mii_mng __iomem *phyregs =
		(struct tsec_mii_mng __iomem *)bus->priv;

	return tsec_local_mdio_read(phyregs, addr, dev_addr, regnum);
}

int tsec_phy_write(struct mii_dev *bus, int addr, int dev_addr, int regnum,
			u16 value)
{
	struct tsec_mii_mng __iomem *phyregs =
		(struct tsec_mii_mng __iomem *)bus->priv;

	tsec_local_mdio_write(phyregs, addr, dev_addr, regnum, value);

	return 0;
}

static int tsec_mdio_reset(struct mii_dev *bus)
{
	return fsl_pq_mdio_reset(bus->priv);
}

int fsl_pq_mdio_init(bd_t *bis, struct fsl_pq_mdio_info *info)
{
	struct mii_dev *bus = mdio_alloc();

	if (!bus) {
		printf("Failed to allocate FSL MDIO bus\n");
		return -1;
	}

	bus->read = tsec_phy_read;
	bus->write = tsec_phy_write;
	bus->reset = tsec_mdio_reset;
	strcpy(bus->name, info->name);

	bus->priv = (void *)info->regs;

	return mdio_register(bus);
}
#endif

#ifdef CONFIG_DM_MDIO
static int dm_fsl_pq_mdio_read(struct udevice *dev, int addr, int devad,
			       int reg)
{
	struct fsl_pq_mdio_info *info = dev_get_priv(dev);

	return tsec_local_mdio_read(info->regs, addr, devad, reg);
}

static int dm_fsl_pq_mdio_write(struct udevice *dev, int addr, int devad,
				int reg, u16 val)
{
	struct fsl_pq_mdio_info *info = dev_get_priv(dev);

	tsec_local_mdio_write(info->regs, addr, devad, reg, val);

	return 0;
}

static int fsl_pq_mdio_probe(struct udevice *dev)
{
	struct fsl_pq_mdio_info *info = dev_get_priv(dev);
	fdt_addr_t reg;

	reg = devfdt_get_addr(dev);
	info->regs = map_physmem(reg + TSEC_MDIO_REGS_OFFSET, 0, MAP_NOCACHE);

	return fsl_pq_mdio_reset(info->regs);
}

static const struct mdio_ops fsl_pq_mdio_ops = {
	.read			= dm_fsl_pq_mdio_read,
	.write			= dm_fsl_pq_mdio_write,
};

static const struct udevice_id fsl_pq_mdio_ids[] = {
	{ .compatible = "fsl,etsec2-mdio" },
	{ }
};

U_BOOT_DRIVER(fsl_pq_mdio) = {
	.name			= "fsl_pq_mdio",
	.id			= UCLASS_MDIO,
	.of_match		= fsl_pq_mdio_ids,
	.probe			= fsl_pq_mdio_probe,
	.ops			= &fsl_pq_mdio_ops,
	.priv_auto_alloc_size	= sizeof(struct fsl_pq_mdio_info),
};
#endif
