/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <serial.h>
#include <libfdt.h>
#include <fdt_support.h>
#include <fdtdec.h>
#include <asm/sections.h>
#include <dm/of_extra.h>
#include <linux/ctype.h>

#ifdef CONFIG_USB_COREID_SET
int fdt_baremetal_setup_usb(void)
{
	void *blob = gd->fdt_blob;
	int node_offset;
	int start_offset = -1;
	int i;
	int usb_core[CONFIG_USB_CTL_NUM];
	int ret;

	if (CONFIG_USB_CTL_NUM >= 1)
		usb_core[0] = CONFIG_USB_USB0_COREID;
	if (CONFIG_USB_CTL_NUM >= 2)
		usb_core[1] = CONFIG_USB_USB1_COREID;
	if (CONFIG_USB_CTL_NUM >= 3)
		usb_core[2] = CONFIG_USB_USB2_COREID;
	if (CONFIG_USB_CTL_NUM >= 4)
		usb_core[3] = -1;

	for (i = 0; i < CONFIG_USB_CTL_NUM; i++) {
		node_offset = fdt_node_offset_by_compatible(blob,
				start_offset, "fsl,layerscape-dwc3");
		if (node_offset > 0) {
			if (get_core_id() != usb_core[i])
				ret = fdt_set_node_status(blob, node_offset,
					FDT_STATUS_DISABLED, 0);
			else
				ret = fdt_set_node_status(blob, node_offset,
					FDT_STATUS_OKAY, 0);

			start_offset = node_offset;
		} else
			break;
	}
	return 0;
}
#endif

int fdt_baremetal_setup(void)
{
	int ret;
#ifdef CONFIG_USB_COREID_SET
	ret = fdt_baremetal_setup_usb();
#endif
	return 0;
}
