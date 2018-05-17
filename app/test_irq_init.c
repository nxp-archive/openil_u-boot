/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/interrupt-gic.h>

#ifdef CONFIG_TARGET_LS1021AIOT
#define CONFIG_TEST_HW_IRQ 195
#elif defined(CONFIG_TARGET_LS1043ARDB) || defined(CONFIG_TARGET_LS1046ARDB)
#define CONFIG_TEST_HW_IRQ 163
#endif

static void test_core_handler_ack(int hw_irq, int src_coreid)
{
	printf(
			"SGI signal: Core[%u] ack irq : %d from core : %d\r\n",
			get_core_id(), hw_irq, src_coreid);

	return;
}

static void test_core_handler_hw_ack(int hw_irq, int src_coreid)
{
	printf(
			"hardware IRQ: Core[%u] ack irq : %d\r\n",
			get_core_id(), hw_irq);

	return;
}


static void test_core_hw_irq_init(u32 coreid, u32 hw_irq)
{
	gic_irq_register(hw_irq, test_core_handler_hw_ack);
	gic_set_type(hw_irq);
	gic_set_target(1 << coreid, hw_irq);
}

void test_irq_init(void)
{
	int coreid = 1;
	/* irq 0-15 are used for SGI, irq 8 is used for IPC */
	gic_irq_register(9, test_core_handler_ack);
	printf("IRQ 9 has been registered as SGI\n");
	/* irq 195-201 are used for hardware interrupt */
	test_core_hw_irq_init(coreid, CONFIG_TEST_HW_IRQ);
	printf("IRQ %d has been registered as HW IRQ\n",
	       CONFIG_TEST_HW_IRQ);

	/* set a SGI signal */
	asm volatile("dsb st");
	gic_set_sgi(1<<coreid, 9);
	asm volatile("sev");

	return;
}
