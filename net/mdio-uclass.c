// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2019
 * Alex Marginean, NXP
 */

#include <common.h>
#include <dm.h>
#include <miiphy.h>
#include <dm/device-internal.h>
#include <dm/uclass-internal.h>

void dm_mdio_probe_devices(void)
{
	struct udevice *it;
	struct uclass *uc;

	uclass_get(UCLASS_MDIO, &uc);
	uclass_foreach_dev(it, uc) {
		device_probe(it);
	}
}

static int dm_mdio_post_bind(struct udevice *dev)
{
	/*
	 * MDIO command doesn't like spaces in names, don't allow them to keep
	 * it happy
	 */
	if (strchr(dev->name, ' ')) {
		debug("\nError: MDIO device name \"%s\" has a space!\n",
		      dev->name);
		return -EINVAL;
	}

	return 0;
}

/*
 * Following read/write/reset functions are registered with legacy MII code.
 * These are called for PHY operations by upper layers and we further call the
 * DM MDIO driver functions.
 */
static int mdio_read(struct mii_dev *mii_bus, int addr, int devad, int reg)
{
	struct udevice *dev = mii_bus->priv;

	return mdio_get_ops(dev)->read(dev, addr, devad, reg);
}

static int mdio_write(struct mii_dev *mii_bus, int addr, int devad, int reg,
		      u16 val)
{
	struct udevice *dev = mii_bus->priv;

	return mdio_get_ops(dev)->write(dev, addr, devad, reg, val);
}

static int mdio_reset(struct mii_dev *mii_bus)
{
	struct udevice *dev = mii_bus->priv;

	if (mdio_get_ops(dev)->reset)
		return mdio_get_ops(dev)->reset(dev);
	else
		return 0;
}

static int dm_mdio_post_probe(struct udevice *dev)
{
	struct mdio_perdev_priv *pdata = dev_get_uclass_priv(dev);

	pdata->mii_bus = mdio_alloc();
	pdata->mii_bus->read = mdio_read;
	pdata->mii_bus->write = mdio_write;
	pdata->mii_bus->reset = mdio_reset;
	pdata->mii_bus->priv = dev;
	strncpy(pdata->mii_bus->name, dev->name, MDIO_NAME_LEN);

	return mdio_register(pdata->mii_bus);
}

static int dm_mdio_pre_remove(struct udevice *dev)
{
	struct mdio_perdev_priv *pdata = dev_get_uclass_priv(dev);
	struct mdio_ops *ops = mdio_get_ops(dev);

	if (ops->reset)
		ops->reset(dev);
	mdio_unregister(pdata->mii_bus);
	mdio_free(pdata->mii_bus);

	return 0;
}

struct phy_device *dm_mdio_phy_connect(struct udevice *dev, int addr,
				       struct udevice *ethdev,
				       phy_interface_t interface)
{
	struct mdio_perdev_priv *pdata = dev_get_uclass_priv(dev);

	if (device_probe(dev))
		return 0;

	return phy_connect(pdata->mii_bus, addr, ethdev, interface);
}

static struct phy_device *dm_eth_connect_phy_handle(struct udevice *dev,
						    phy_interface_t if_type)
{
	u32 phy_phandle, phy_addr;
	struct udevice *mdio_dev;
	struct phy_device *phy;
	ofnode phy_node;

	if (ofnode_read_u32(dev->node, "phy-handle", &phy_phandle)) {
		dev_dbg(dev, "phy-handle missing in ethernet node\n");
		return NULL;
	}

	phy_node = ofnode_get_by_phandle(phy_phandle);
	if (!ofnode_valid(phy_node)) {
		dev_dbg(dev, "invalid phy node\n");
		return NULL;
	}

	if (ofnode_read_u32(phy_node, "reg", &phy_addr)) {
		dev_dbg(dev, "missing reg property in phy node\n");
		return NULL;
	}

	if (uclass_get_device_by_ofnode(UCLASS_MDIO,
					ofnode_get_parent(phy_node),
					&mdio_dev)) {
		dev_dbg(dev, "can't find MDIO bus for node %s\n",
			ofnode_get_name(ofnode_get_parent(phy_node)));
		return NULL;
	}

	phy = dm_mdio_phy_connect(mdio_dev, phy_addr, dev, if_type);
	phy->node = phy_node;

	return phy;
}

static struct phy_device *dm_eth_connect_mdio_handle(struct udevice *dev,
						     phy_interface_t if_type)
{
	u32 mdio_phandle;
	ofnode mdio_node;
	struct udevice *mdio_dev;
	struct phy_device *phy;
	uint mask = (uint)-1;
	struct mdio_perdev_priv *pdata;

	if (ofnode_read_u32(dev->node, "mdio-handle", &mdio_phandle)) {
		dev_dbg(dev, "mdio-handle missing in ethernet node\n");
		return NULL;
	}

	mdio_node = ofnode_get_by_phandle(mdio_phandle);
	if (!ofnode_valid(mdio_node)) {
		dev_dbg(dev, "invalid mdio node\n");
		return NULL;
	}

	if (uclass_get_device_by_ofnode(UCLASS_MDIO, mdio_node, &mdio_dev)) {
		dev_dbg(dev, "can't find MDIO bus for node %s\n",
			ofnode_get_name(mdio_node));
		return NULL;
	}
	pdata = dev_get_uclass_priv(mdio_dev);

	phy = phy_find_by_mask(pdata->mii_bus, mask, if_type);
	if (phy)
		phy_connect_dev(phy, dev);
	else
		dev_dbg(dev, "no PHY detected on bus\n");

	return phy;
}

/* Connect to a PHY linked in eth DT node */
struct phy_device *dm_eth_phy_connect(struct udevice *dev)
{
	const char *if_type_str;
	phy_interface_t if_type;
	struct phy_device *phy;

	if (!ofnode_valid(dev->node)) {
		debug("%s: supplied eth dev has no DT node!\n", dev->name);
		return NULL;
	}

	/*
	 * The sequence should be as follows:
	 * - if there is a phy-handle property, follow that,
	 * - if there is a mdio-handle property, follow that and scan for the
	 *   PHY,
	 * - if the above came out empty, just return NULL.
	 */

	if_type = PHY_INTERFACE_MODE_NONE;
	if_type_str = ofnode_read_string(dev->node, "phy-mode");
	if (if_type_str)
		if_type = phy_get_interface_by_name(if_type_str);
	if (if_type < 0)
		if_type = PHY_INTERFACE_MODE_NONE;

	if (if_type == PHY_INTERFACE_MODE_NONE)
		dev_dbg(dev, "can't find interface mode, default to NONE\n");

	phy = dm_eth_connect_phy_handle(dev, if_type);

	if (!phy)
		phy = dm_eth_connect_mdio_handle(dev, if_type);

	if (!phy)
		return NULL;

	phy->interface = if_type;

	return phy;
}

UCLASS_DRIVER(mdio) = {
	.id = UCLASS_MDIO,
	.name = "mdio",
	.post_bind  = dm_mdio_post_bind,
	.post_probe = dm_mdio_post_probe,
	.pre_remove = dm_mdio_pre_remove,
	.per_device_auto_alloc_size = sizeof(struct mdio_perdev_priv),
};
