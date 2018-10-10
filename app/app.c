/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <command.h>
#include <common.h>
#include <malloc.h>
#include <environment.h>
#include <linux/types.h>
#include "test.h"

void core1_main(void)
{
	test_i2c();
	test_irq_init();
#ifdef CONFIG_TARGET_LS1021AIOT
	test_gpio();
#endif
	test_icc_func_init();
#if defined(CONFIG_TARGET_LS1021AIOT) || defined(CONFIG_TARGET_LS1046ARDB)
	test_qspi();
#endif

#ifdef CONFIG_FMAN_COREID_SET
	test_net();
#endif

#ifdef CONFIG_USB_COREID_SET
	test_usb();
#endif

#ifdef CONFIG_PCIE_COREID_SET
	test_pcie();
#endif

#if CONFIG_FS_FLEXCAN
	test_flexcan();
#endif

	return;
}


void core2_main(void)
{
	test_icc_func_init();

#ifdef CONFIG_USB_COREID_SET
	test_usb();
#endif

#ifdef CONFIG_PCIE_COREID_SET
	test_pcie();
#endif

	return;
}

void core3_main(void)
{
	test_icc_func_init();

#ifdef CONFIG_USB_COREID_SET
	test_usb();
#endif

	return;
}
