/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/inter-core-comm.h>
#include <div64.h>

static void do_icc_show(void)
{
	icc_show();
}

static void do_icc_irq_handle(int src_coreid,
	unsigned long block, unsigned int counts)
{
	if ((*(char *)block) != 0x5a)
		printf(
			"Get the ICC from core %d; block: 0x%lx, bytes: %d, value: 0x%x\n",
			src_coreid, block, counts, (*(char *)block));
}

static void do_icc_irq_register(void)
{
	int coreid = CONFIG_MAX_CPUS;

	if (icc_irq_register(coreid, do_icc_irq_handle))
		printf(
			"ICC register irq handler failed, src_coreid: %d, max_cores: %d\n",
			coreid, CONFIG_MAX_CPUS);
}

static void do_icc_irq_set(unsigned long core_mask, unsigned long irq)
{
	if (irq > 15) {
		printf("Interrupt id num: %lu is invalid, SGI[0 - 15]\n", irq);
		return;
	};

	icc_set_sgi(core_mask, irq);
}

static void do_icc_perf(unsigned long core_mask, unsigned long counts)
{
	int i, k, ret;
	unsigned int dest_core = 0;
	int mycoreid = get_core_id();
	unsigned long block;
	unsigned long bytes = counts;
	unsigned long long start, end, utime;
	unsigned long freq;

	for (i = 0; i < CONFIG_MAX_CPUS; i++) {
		if (((core_mask >> i) & 0x1) && (i != mycoreid))
			dest_core |= 0x1 << i;
	}

	if (counts > ICC_CORE_BLOCK_COUNT * ICC_BLOCK_UNIT_SIZE) {
		printf(
			"ICC performance test error! Max bytes: %d, input bytes: %ld",
			ICC_CORE_BLOCK_COUNT * ICC_BLOCK_UNIT_SIZE, counts);
		return;
	}

	/* for performance test, set all share blocks to 0x5a */
	memset((void *)ICC_CORE_BLOCK_BASE(mycoreid), 0x5a,
	       (ICC_CORE_MEM_SPACE - ICC_RING_DESC_SPACE));

	printf("ICC performance testing ...\n");
	printf("Target cores: 0x%x, bytes: %ld, ", dest_core, counts);

	freq = get_tbclk();
	start = get_ticks();
	while (bytes >= ICC_BLOCK_UNIT_SIZE) {
		block = icc_block_request();
		if (!block) {
			printf(
				"No available block! sent %ld bytes\n",
				(counts - bytes));
			continue;
		} else {
			ret = icc_set_block(dest_core,
					    ICC_BLOCK_UNIT_SIZE, block);
			if (ret) {
				icc_block_free(block);
				continue;
			}
		}
		bytes -= ICC_BLOCK_UNIT_SIZE;
	}

	while (bytes) {
		block = icc_block_request();
		if (!block) {
			printf(
				"No available block! sent %ld bytes\n",
				(counts - bytes));
			continue;
		} else {
			ret = icc_set_block(dest_core, bytes, block);
			if (ret) {
				icc_block_free(block);
				continue;
			}
		}
		bytes = 0;
	}

	while (1) {
		k = 0;
		for (i = 0; i < CONFIG_MAX_CPUS; i++) {
			if (((dest_core >> i) & 0x1) && (i != mycoreid)) {
				if (icc_ring_state(i))
					k++;
			}
		}
		if (!k)
			break;
	}

	end = get_ticks();
	utime = lldiv(1000000 * (end - start), freq);


	printf("ICC performance: %ld bytes to 0x%x cores in %lld us with %lld KB/s\n",
		counts, dest_core, utime, lldiv((((unsigned long long)counts) * 1000), utime));

	printf("\n");
	icc_show();
}

static void do_icc_send(unsigned long core_mask,
	unsigned long data, unsigned long counts)
{
	unsigned long dest_core = 0;
	unsigned long bytes, block;
	int mycoreid = get_core_id();
	int i, k, ret;

	for (i = 0; i < CONFIG_MAX_CPUS; i++) {
		if (((core_mask >> i) & 0x1) && (i != mycoreid))
			dest_core |= 0x1 << i;
	}
	if (!dest_core) {
		printf("dest_core error\n");
		return;
	}

	if (counts > ICC_CORE_BLOCK_COUNT * ICC_BLOCK_UNIT_SIZE) {
		printf(
			"ICC send error! Max bytes: %d, input bytes: %ld",
			ICC_CORE_BLOCK_COUNT * ICC_BLOCK_UNIT_SIZE, counts);
		return;
	}

	bytes = counts;

	printf("ICC send testing ...\n");
	printf("Target cores: 0x%lx, bytes: %ld\n", dest_core, counts);

	while (bytes >= ICC_BLOCK_UNIT_SIZE) {
		block = icc_block_request();
		if (!block) {
			printf(
				"No available block! sent %ld bytes\n",
				(counts - bytes));
			continue;
		} else {
			memset((void *)block, data, ICC_BLOCK_UNIT_SIZE);
			ret = icc_set_block(dest_core,
					    ICC_BLOCK_UNIT_SIZE, block);
			if (ret) {
				printf(
					"The ring is full! sent %ld bytes\n",
					(counts - bytes));
				icc_block_free(block);
				continue;
			}
		}
		bytes -= ICC_BLOCK_UNIT_SIZE;
	}

	while (bytes) {
		block = icc_block_request();
		if (!block) {
			printf(
				"No available block! sent %ld bytes\n",
				(counts - bytes));
			continue;
		} else {
			memset((void *)block, data, bytes);
			ret = icc_set_block(dest_core, bytes, block);
			if (ret) {
				printf(
					"The ring is full! sent %ld bytes\n",
					(counts - bytes));
				icc_block_free(block);
				continue;
			}
		}
		bytes = 0;
	}

	while (1) {
		k = 0;
		for (i = 0; i < CONFIG_MAX_CPUS; i++) {
			if (((dest_core >> i) & 0x1) && (i != mycoreid)) {
				if (icc_ring_state(i))
					k++;
			}
		}
		if (!k)
			break;
	}

	printf(
		"ICC send: %ld bytes to 0x%lx cores success\n",
		counts, dest_core);

	printf("\n");
	icc_show();
}

static int
icc_cmd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	do_icc_irq_register();

	if (argc == 2) {
		if (strncmp(argv[1], "show", 4) == 0)
			do_icc_show();
		else
			return CMD_RET_USAGE;

		return 0;
	}

	if (argc == 4) {
		if (strncmp(argv[1], "irq", 3) == 0)
			do_icc_irq_set(simple_strtoul(argv[2], NULL, 16),
				       simple_strtoul(argv[3], NULL, 10));
		else if (strncmp(argv[1], "perf", 4) == 0)
			do_icc_perf(simple_strtoul(argv[2], NULL, 16),
				    simple_strtoul(argv[3], NULL, 0));
		else
			return CMD_RET_USAGE;

		return 0;
	}

	if (argc == 5) {
		if (strncmp(argv[1], "send", 4) == 0)
			do_icc_send(simple_strtoul(argv[2], NULL, 16),
				    simple_strtoul(argv[3], NULL, 0),
				    simple_strtoul(argv[4], NULL, 10));
		else
			return CMD_RET_USAGE;

		return 0;
	}
	return CMD_RET_USAGE;
}

#ifdef CONFIG_SYS_LONGHELP
static char icc_help_text[] =
	"show				- Show all icc rings status at this core\n"
	"icc perf <core_mask> <counts>		- ICC performance to cores <core_mask> with <counts> bytes\n"
	"icc send <core_mask> <data> <counts>	- Send <counts> <data> to cores <core_mask>\n"
	"icc irq <core_mask> <irq>		- Send SGI <irq> ID[0 - 15] to <core_mask>\n"
	"";
#endif

U_BOOT_CMD(
	icc, CONFIG_SYS_MAXARGS, 0, icc_cmd,
	"Inter-core communication via SGI interrupt", icc_help_text
);
