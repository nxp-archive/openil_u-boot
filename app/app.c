// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2019 NXP
 *
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
	test_gpio();
	return;
}

void core2_main(void)
{
	return;
}

void core3_main(void)
{
	return;
}
