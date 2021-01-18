// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2021 NXP
 *
 */

#include <config.h>
#include <command.h>
#include <common.h>
#include <malloc.h>
#include <env.h>
#include <linux/types.h>
#include "test.h"

void core1_main(void)
{
#ifdef CONFIG_I2C_COREID_SET
	test_i2c();
#endif
#ifdef CONFIG_IRQ_COREID_SET
	test_irq_init();
#endif
#ifdef CONFIG_GPIO_COREID_SET
	test_gpio();
#endif
#ifdef CONFIG_ICC_COREID_SET
	test_icc_func_init();
#endif
#ifdef CONFIG_QSPI_COREID_SET
	test_qspi();
#endif
#ifdef CONFIG_FMAN_COREID_SET
	test_net();
#endif

#ifdef CONFIG_USB_COREID_SET
	test_usb();
#endif

	return;
}

void core2_main(void)
{
#ifdef CONFIG_ICC_COREID_SET
	test_icc_func_init();
#endif

#ifdef CONFIG_USB_COREID_SET
	test_usb();
#endif

	return;
}

void core3_main(void)
{
#ifdef CONFIG_ICC_COREID_SET
	test_icc_func_init();
#endif

#ifdef CONFIG_USB_COREID_SET
	test_usb();
#endif

	return;
}
