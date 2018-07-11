/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <bootretry.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <dm.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <pci.h>
#include <netdev.h>
#include <net.h>
#include <linux/delay.h>

#define ipaddr		"192.168.1.1"
#define server_ip	"192.168.1.2"

#ifdef CONFIG_DM_PCI
static const struct pci_flag_info {
	uint flag;
	const char *name;
} pci_flag_info[] = {
	{ PCI_REGION_IO, "io" },
	{ PCI_REGION_PREFETCH, "prefetch" },
	{ PCI_REGION_SYS_MEMORY, "sysmem" },
	{ PCI_REGION_RO, "readonly" },
	{ PCI_REGION_IO, "io" },
};

static void pci_show_regions(struct udevice *bus)
{
	struct pci_controller *hose = dev_get_uclass_priv(bus);
	const struct pci_region *reg;
	int i, j;

	if (!hose) {
		printf("Bus '%s' is not a PCI controller\n", bus->name);
		return;
	}

	printf("#   %-16s %-16s %-16s  %s\n", "Bus start", "Phys start", "Size",
	       "Flags");
	for (i = 0, reg = hose->regions; i < hose->region_count; i++, reg++) {
		printf("%d   %#016llx %#016llx %#016llx  ", i,
		       (unsigned long long)reg->bus_start,
		       (unsigned long long)reg->phys_start,
		       (unsigned long long)reg->size);
		if (!(reg->flags & PCI_REGION_TYPE))
			printf("mem ");
		for (j = 0; j < ARRAY_SIZE(pci_flag_info); j++) {
			if (reg->flags & pci_flag_info[j].flag)
				printf("%s ", pci_flag_info[j].name);
		}
		printf("\n");
	}
}

/**
 * pci_header_show_brief() - Show the short-form PCI device header
 *
 * Reads and prints the header of the specified PCI device in short form.
 *
 * @dev: PCI device to show
 */
static void pci_header_show_brief(struct udevice *dev)
{
	ulong vendor, device;
	ulong class, subclass;

	dm_pci_read_config(dev, PCI_VENDOR_ID, &vendor, PCI_SIZE_16);
	dm_pci_read_config(dev, PCI_DEVICE_ID, &device, PCI_SIZE_16);
	dm_pci_read_config(dev, PCI_CLASS_CODE, &class, PCI_SIZE_8);
	dm_pci_read_config(dev, PCI_CLASS_SUB_CODE, &subclass, PCI_SIZE_8);

	printf("0x%.4lx     0x%.4lx     %-23s 0x%.2lx\n",
	       vendor, device,
		pci_class_str(class), subclass);
}

static void pciinfo(struct udevice *bus, bool short_listing)
{
	struct udevice *dev;

	pciinfo_header(bus->seq, short_listing);

	for (device_find_first_child(bus, &dev);
	dev;
	device_find_next_child(&dev)) {
		struct pci_child_platdata *pplat;

		pplat = dev_get_parent_platdata(dev);
		if (short_listing) {
			printf("%02x.%02x.%02x   ", bus->seq,
			       PCI_DEV(pplat->devfn), PCI_FUNC(pplat->devfn));
			pci_header_show_brief(dev);
		} else {
			printf("\nFound PCI device %02x.%02x.%02x:\n", bus->seq,
			       PCI_DEV(pplat->devfn), PCI_FUNC(pplat->devfn));
			pci_header_show(dev);
		}
	}
}

#else

/**
 * pci_header_show_brief() - Show the short-form PCI device header
 *
 * Reads and prints the header of the specified PCI device in short form.
 *
 * @dev: Bus+Device+Function number
 */
void pci_header_show_brief(pci_dev_t dev)
{
	u16 vendor, device;
	u8 class, subclass;

	pci_read_config_word(dev, PCI_VENDOR_ID, &vendor);
	pci_read_config_word(dev, PCI_DEVICE_ID, &device);
	pci_read_config_byte(dev, PCI_CLASS_CODE, &class);
	pci_read_config_byte(dev, PCI_CLASS_SUB_CODE, &subclass);

	printf("0x%.4x     0x%.4x     %-23s 0x%.2x\n",
	       vendor, device,
	       pci_class_str(class), subclass);
}

/**
 * pciinfo() - Show a list of devices on the PCI bus
 *
 * Show information about devices on PCI bus. Depending on @short_pci_listing
 * the output will be more or less exhaustive.
 *
 * @bus_num: The number of the bus to be scanned
 * @short_pci_listing: true to use short form, showing only a brief header
 * for each device
 */
void pciinfo(int bus_num, int short_pci_listing)
{
	struct pci_controller *hose = pci_bus_to_hose(bus_num);
	int device;
	int function;
	unsigned char header_type;
	unsigned short vendor_id;
	pci_dev_t dev;
	int ret;

	if (!hose)
		return;

	pciinfo_header(bus_num, short_pci_listing);

	for (device = 0; device < PCI_MAX_PCI_DEVICES; device++) {
		header_type = 0;
		vendor_id = 0;
		for (function = 0; function < PCI_MAX_PCI_FUNCTIONS;
			function++) {
			/*
			 * If this is not a multi-function device, we skip
			 * the rest.
			 */
			if (function && !(header_type & 0x80))
				break;

			dev = PCI_BDF(bus_num, device, function);

			if (pci_skip_dev(hose, dev))
				continue;

			ret = pci_read_config_word(dev, PCI_VENDOR_ID,
						   &vendor_id);
			if (ret)
				goto error;
			if ((vendor_id == 0xFFFF) || (vendor_id == 0x0000))
				continue;

			if (!function) {
				pci_read_config_byte(dev, PCI_HEADER_TYPE,
						     &header_type);
			}

			if (short_pci_listing) {
				printf("%02x.%02x.%02x   ", bus_num, device,
				       function);
				pci_header_show_brief(dev);
			} else {
				printf("\nFound PCI device %02x.%02x.%02x:\n",
				       bus_num, device, function);
				pci_header_show(dev);
			}
		}
	}

	return;
error:
	printf("Cannot read bus configuration: %d\n", ret);
}
#endif

void test_pcie(void)
{
	struct udevice *dev, *bus;
	int busnum = 0;
	int ret = 0;

	/* Init PCIe bus */
	printf("\nPCIe controller Init ...\n");
	pci_init();

	ret = uclass_get_device_by_seq(UCLASS_PCI, busnum, &bus);
	if (ret) {
		printf("No such bus\n");
		return;
	}

	printf("\nPCIe controller regions:\n");
	pci_show_regions(bus);

#ifdef CONFIG_DM_PCI
	printf("\nPCIe bus information:\n");
	pciinfo(bus, 1);

	ret = uclass_get_device_by_seq(UCLASS_PCI, busnum + 1, &bus);
	if (ret) {
		printf("No such bus\n");
		return;
	}
	pciinfo(bus, 1);
#else
	printf("\nPCIe bus information:\n");
	pciinfo(busnum, 1);
	pciinfo(busnum + 1, 1);
#endif
	if (pci_eth_init(gd->bd) <= 0)
		printf("Not found Net device\n");
	else {
		printf("Found Net device, start to ping ...\n");

		net_ping_ip = string_to_ip(server_ip);
		net_ip = string_to_ip(ipaddr);

		if (net_ping_ip.s_addr == 0)
			return;
		udelay(3000);
		if (net_loop(PING) < 0)
			printf("ping Failed; host %s is not alive\n",
					server_ip);
		else
			printf("host %s is alive\n", server_ip);
	}
}
