/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 *
 * PCIe DT fixup for NXP Layerscape SoCs
 * Author: Wasim Khan <wasim.khan@nxp.com>
 *
 */
#ifndef _PCIE_LAYERSCAPE_FIXUP_COMMON_H_
#define _PCIE_LAYERSCAPE_FIXUP_COMMON_H_

#include <common.h>

void ft_pci_setup_ls(void *blob, bd_t *bd);

#if defined(CONFIG_FSL_LAYERSCAPE)
#ifdef CONFIG_PCIE_LAYERSCAPE_GEN4
void ft_pci_setup_ls_gen4(void *blob, bd_t *bd);
#else
static void ft_pci_setup_ls_gen4(void *blob, bd_t *bd)
{
}
#endif
#endif /* CONFIG_FSL_LAYERSCAPE */

#endif //_PCIE_LAYERSCAPE_FIXUP_COMMON_H_
