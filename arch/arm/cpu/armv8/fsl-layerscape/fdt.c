/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <libfdt.h>
#include <fdt_support.h>
#include <phy.h>
#ifdef CONFIG_FSL_LSCH3
#include <asm/arch/fdt.h>
#endif
#ifdef CONFIG_FSL_ESDHC
#include <fsl_esdhc.h>
#endif
#ifdef CONFIG_SYS_DPAA_FMAN
#include <fsl_fman.h>
#endif
#ifdef CONFIG_MP
#include <asm/arch/mp.h>
#endif
#include <fsl_sec.h>
#include <asm/arch-fsl-layerscape/soc.h>
#ifdef CONFIG_ARMV8_SEC_FIRMWARE_SUPPORT
#include <asm/armv8/sec_firmware.h>
#endif

int fdt_fixup_phy_connection(void *blob, int offset, phy_interface_t phyc)
{
	return fdt_setprop_string(blob, offset, "phy-connection-type",
					 phy_string_for_interface(phyc));
}

#ifdef CONFIG_MP
void ft_fixup_cpu(void *blob)
{
	int off;
	__maybe_unused u64 spin_tbl_addr = (u64)get_spin_tbl_addr();
	fdt32_t *reg;
	int addr_cells;
	u64 val, core_id;
	size_t *boot_code_size = &(__secondary_boot_code_size);
#if defined(CONFIG_ARMV8_SEC_FIRMWARE_SUPPORT) && defined(CONFIG_ARMV8_PSCI)
	int node;
	u32 psci_ver;

	/* Check the psci version to determine if the psci is supported */
	psci_ver = sec_firmware_support_psci_version();
	if (psci_ver == 0xffffffff) {
		/* remove psci DT node */
		node = fdt_path_offset(blob, "/psci");
		if (node >= 0)
			goto remove_psci_node;

		node = fdt_node_offset_by_compatible(blob, -1, "arm,psci");
		if (node >= 0)
			goto remove_psci_node;

		node = fdt_node_offset_by_compatible(blob, -1, "arm,psci-0.2");
		if (node >= 0)
			goto remove_psci_node;

		node = fdt_node_offset_by_compatible(blob, -1, "arm,psci-1.0");
		if (node >= 0)
			goto remove_psci_node;

remove_psci_node:
		if (node >= 0)
			fdt_del_node(blob, node);
	} else {
		return;
	}
#endif
	off = fdt_path_offset(blob, "/cpus");
	if (off < 0) {
		puts("couldn't find /cpus node\n");
		return;
	}
	of_bus_default_count_cells(blob, off, &addr_cells, NULL);

	off = fdt_node_offset_by_prop_value(blob, -1, "device_type", "cpu", 4);
	while (off != -FDT_ERR_NOTFOUND) {
		reg = (fdt32_t *)fdt_getprop(blob, off, "reg", 0);
		if (reg) {
			core_id = of_read_number(reg, addr_cells);
			if (core_id  == 0 || (is_core_online(core_id))) {
				val = spin_tbl_addr;
				val += id_to_core(core_id) *
				       SPIN_TABLE_ELEM_SIZE;
				val = cpu_to_fdt64(val);
				fdt_setprop_string(blob, off, "enable-method",
						   "spin-table");
				fdt_setprop(blob, off, "cpu-release-addr",
					    &val, sizeof(val));
			} else {
				debug("skipping offline core\n");
			}
		} else {
			puts("Warning: found cpu node without reg property\n");
		}
		off = fdt_node_offset_by_prop_value(blob, off, "device_type",
						    "cpu", 4);
	}

	fdt_add_mem_rsv(blob, (uintptr_t)&secondary_boot_code,
			*boot_code_size);
}
#endif

void fsl_fdt_disable_usb(void *blob)
{
	int off;
	/*
	 * SYSCLK is used as a reference clock for USB. When the USB
	 * controller is used, SYSCLK must meet the additional requirement
	 * of 100 MHz.
	 */
	if (CONFIG_SYS_CLK_FREQ != 100000000) {
		off = fdt_node_offset_by_compatible(blob, -1, "snps,dwc3");
		while (off != -FDT_ERR_NOTFOUND) {
			fdt_status_disabled(blob, off);
			off = fdt_node_offset_by_compatible(blob, off,
							    "snps,dwc3");
		}
	}
}

#ifdef CONFIG_HAS_FEATURE_GIC4K_ALIGN
/* Fixup gic node align with 4K */
static void fdt_fixup_gic(void *blob)
{
	int offset, err;
	u64 reg[8];
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	unsigned int val;
	struct ccsr_scfg __iomem *scfg = (void *)CONFIG_SYS_FSL_SCFG_ADDR;

	val = gur_in32(&gur->svr) & 0xff;

	if (val == REV1_1) {
		val = scfg_in32(&scfg->gic_align) & (0x01 << GIC_ADDR_BIT);
		if (!val)
			return;
	}

	offset = fdt_subnode_offset(blob, 0, "interrupt-controller@1400000");
	if (offset < 0) {
		printf("WARNING: fdt_subnode_offset can't find node %s: %s\n",
		       "interrupt-controller@1400000", fdt_strerror(offset));
		return;
	}

	reg[0] = cpu_to_fdt64(GICD_BASE_4K);
	reg[1] = cpu_to_fdt64(GICD_SIZE_4K);
	reg[2] = cpu_to_fdt64(GICC_BASE_4K);
	reg[3] = cpu_to_fdt64(GICC_SIZE_4K);
	reg[4] = cpu_to_fdt64(GICH_BASE_4K);
	reg[5] = cpu_to_fdt64(GICH_SIZE_4K);
	reg[6] = cpu_to_fdt64(GICV_BASE_4K);
	reg[7] = cpu_to_fdt64(GICV_SIZE_4K);

	err = fdt_setprop(blob, offset, "reg", reg, sizeof(reg));
	if (err < 0) {
		printf("WARNING: fdt_setprop can't set %s from node %s: %s\n",
		       "reg", "interrupt-controller@1400000",
		       fdt_strerror(err));
		return;
	}

	return;
}
#endif
#ifdef CONFIG_HAS_FEATURE_ENHANCED_MSI
static int _fdt_fixup_msi_subnode(void *blob, int parentoffset,
				const char *name, int irq_no)
{
	int err, offset;
	u32 tmp[3];

	offset = fdt_subnode_offset(blob, parentoffset, name);
	if (offset < 0) {
		printf("WARNING: fdt_subnode_offset can't find %s: %s\n",
		       name, fdt_strerror(offset));
		return 0;
	}

	tmp[0] = cpu_to_fdt32(0x0);
	tmp[1] = cpu_to_fdt32(irq_no);
	tmp[2] = cpu_to_fdt32(0x4);

	err = fdt_setprop(blob, offset, "interrupts", tmp, sizeof(tmp));
	if (err < 0) {
		printf("WARNING: fdt_setprop can't set %s from node %s: %s\n",
		       "interrupts", name, fdt_strerror(err));
		return 0;
	}

	return 1;
}

static int _fdt_fixup_pci_msi(void *blob, const char *name)
{
	int offset, len, err;
	void *p;
	int val;
	u32 tmp[4][8];

	offset = fdt_path_offset(blob, name);
	if (offset < 0) {
		printf("WARNING: fdt_path_offset can't find path %s: %s\n",
		       name, fdt_strerror(offset));
		return 0;
	}

	p = (char *)fdt_getprop(blob, offset, "interrupt-map", &len);
	if (!p || len != sizeof(tmp)) {
		printf("WARNING: fdt_getprop can't get %s from node %s\n",
		       "interrupt-map", name);
		return 0;
	}

	memcpy((char *)tmp, p, len);
	val = fdt32_to_cpu(tmp[0][6]);
	tmp[1][6] = cpu_to_fdt32(val + 1);
	tmp[2][6] = cpu_to_fdt32(val + 2);
	tmp[3][6] = cpu_to_fdt32(val + 3);

	err = fdt_setprop(blob, offset, "interrupt-map", tmp, sizeof(tmp));
	if (err < 0) {
		printf("WARNING: fdt_setprop can't set %s from node %s: %s.\n",
		       "interrupt-map", name, fdt_strerror(err));
		return 0;
	}
	return 1;
}

/* Fixup msi to v1_0*/

static void fdt_fixup_msi(void *blob)
{
	int nodeoffset;
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	unsigned int val;

	val = gur_in32(&gur->svr) & 0xff;
	if (val == REV1_1)
		return;

	nodeoffset = fdt_path_offset(blob, "/soc/msi-controller");
	if (nodeoffset < 0) {
		printf("WARNING: fdt_path_offset can't find path %s: %s\n",
		       "/soc/msi-controller", fdt_strerror(nodeoffset));
		return;
	}
	_fdt_fixup_msi_subnode(blob, nodeoffset, "msi0@1571000", 116);
	_fdt_fixup_msi_subnode(blob, nodeoffset, "msi1@1572000", 126);
	_fdt_fixup_msi_subnode(blob, nodeoffset, "msi2@1573000", 160);

	_fdt_fixup_pci_msi(blob, "/soc/pcie@3400000");
	_fdt_fixup_pci_msi(blob, "/soc/pcie@3500000");
	_fdt_fixup_pci_msi(blob, "/soc/pcie@3600000");
}
#endif

void ft_cpu_setup(void *blob, bd_t *bd)
{
#ifdef CONFIG_FSL_LSCH2
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	unsigned int svr = in_be32(&gur->svr);

	/* delete crypto node if not on an E-processor */
	if (!IS_E_PROCESSOR(svr))
		fdt_fixup_crypto_node(blob, 0);
#if CONFIG_SYS_FSL_SEC_COMPAT >= 4
	else {
		ccsr_sec_t __iomem *sec;

		sec = (void __iomem *)CONFIG_SYS_FSL_SEC_ADDR;
		fdt_fixup_crypto_node(blob, sec_in32(&sec->secvid_ms));
	}
#endif
#endif

#ifdef CONFIG_MP
	ft_fixup_cpu(blob);
#endif

#ifdef CONFIG_SYS_NS16550
	do_fixup_by_compat_u32(blob, "fsl,ns16550",
			       "clock-frequency", CONFIG_SYS_NS16550_CLK, 1);
#endif

	do_fixup_by_compat_u32(blob, "fixed-clock",
			       "clock-frequency", CONFIG_SYS_CLK_FREQ, 1);

#ifdef CONFIG_PCI
	ft_pci_setup(blob, bd);
#endif

#ifdef CONFIG_FSL_ESDHC
	fdt_fixup_esdhc(blob, bd);
#endif

#ifdef CONFIG_SYS_DPAA_FMAN
	fdt_fixup_fman_firmware(blob);
#endif
	fsl_fdt_disable_usb(blob);

#ifdef CONFIG_HAS_FEATURE_GIC4K_ALIGN
	fdt_fixup_gic(blob);
#endif
#ifdef CONFIG_HAS_FEATURE_ENHANCED_MSI
	fdt_fixup_msi(blob);
#endif
}
