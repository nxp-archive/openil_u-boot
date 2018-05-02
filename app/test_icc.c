
/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

static void do_test_icc_irq_handle(int src_coreid,
	unsigned long block, unsigned int counts)
{
	if ((*(char *)block) != 0x5a)
		printf(
			"Get the ICC from core %d; block: 0x%lx, bytes: %d, value: 0x%x\n",
			src_coreid, block, counts, (*(char *)block));
}

void test_icc_func_init(void)
{
	icc_irq_register(CONFIG_MAX_CPUS, do_test_icc_irq_handle);
}
