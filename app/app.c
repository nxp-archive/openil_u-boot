// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2020 NXP
 *
 */

#include <config.h>
#include <command.h>
#include <common.h>
#include <malloc.h>
#include <environment.h>
#include <linux/types.h>
#include "test.h"
#include "../cmd/wavplayer.h"

void core1_main(void)
{
#ifndef CONFIG_TARGET_LX2160ARDB
#ifndef CONFIG_TARGET_MX6SABRESD
#ifndef CONFIG_TARGET_LS1028ARDB
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
#else
	test_icc_func_init();
#ifdef CONFIG_ENETC_COREID_SET
	test_net();
#endif
#endif
#endif
#endif
#ifdef CONFIG_TARGET_MX6SABRESD
	test_icc_func_init();
#endif

#ifdef CONFIG_TARGET_LX2160ARDB
	test_icc_func_init();
#endif

#if defined(CONFIG_TARGET_LS1028ARDB) && defined(CONFIG_CMD_WAVPLAYER)
	test_wavplay();
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

void core4_main(void)
{
	test_icc_func_init();

	return;
}

void core5_main(void)
{
	test_icc_func_init();

	return;
}

void core6_main(void)
{
	test_icc_func_init();

	return;
}

void core7_main(void)
{
	test_icc_func_init();

	return;
}

void core8_main(void)
{
	test_icc_func_init();

	return;
}

void core9_main(void)
{
	test_icc_func_init();

	return;
}

void core10_main(void)
{
	test_icc_func_init();

	return;
}

void core11_main(void)
{
	test_icc_func_init();

	return;
}

void core12_main(void)
{
	test_icc_func_init();

	return;
}

void core13_main(void)
{
	test_icc_func_init();

	return;
}

void core14_main(void)
{
	test_icc_func_init();

	return;
}

void core15_main(void)
{
	test_icc_func_init();

	return;
}
