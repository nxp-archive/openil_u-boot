/*
* Copyright (C) 2016 Freescale Semiconductor Inc.
*
* SPDX-License-Identifier:GPL-2.0+
*/

/**
 * @file
 * @brief PFE utility commands
 */

#include <common.h>
#include <command.h>
#include "../drivers/net/pfe_eth/pfe_eth.h"
#include "../drivers/net/pfe_eth/pfe/pfe.h"
#include "../drivers/net/pfe_eth/pfe_firmware.h"
#include "../drivers/net/pfe_eth/pfe/cbus/class_csr.h"
#include "../drivers/net/pfe_eth/pfe/cbus/gpi.h"
DECLARE_GLOBAL_DATA_PTR;

void hif_rx_desc_disable(void);
int pfe_load_elf(int pe_mask, const struct firmware *fw);
int ls1012a_gemac_initialize(bd_t * bis, int dev_id, char *devname);

static void pfe_command_help(void)
{
	printf("Usage: pfe [start | firmware | load | lib | pe | gemac | gem | gpi | class | tmu | util | hif | status | expt | fftest] <options>\n");
}

static void pfe_command_firmware(int argc, char * const argv[])
{
	if (argc == 3 && strcmp(argv[2], "init") == 0)
	{
		pfe_firmware_init((u8 *)0x80100000, (u8 *)0x80180000, (u8 *)0x80200000);
	}
	else if (argc == 3 && strcmp(argv[2], "exit") == 0)
	{
		pfe_firmware_exit();
	}
	else
	{
		if (argc >= 3 && strcmp(argv[2], "help") != 0)
		{
			printf("Unknown option: %s\n", argv[2]);
		}
		printf("Usage: pfe firmware [init | exit]\n");
	}
}

static void pfe_command_load(int argc, char * const argv[])
{
	if (argc >= 3 && strcmp(argv[2], "elf") == 0)
	{
		if (argc == 5)
		{
			u32 mask;
			unsigned long image_start;
			struct firmware fw;
			mask = simple_strtoul(argv[3], NULL, 0);
			image_start = simple_strtoul(argv[4], NULL, 16);
			fw.data = (u8 *)image_start;
			pfe_load_elf(mask, &fw);
		}
		else
		{
			printf("Usage: pfe load elf <pe_mask> <image_start>\n");
		}
	}
	else
	{
		if (argc >= 3 && strcmp(argv[2], "help") != 0)
		{
			printf("Unknown option: %s\n", argv[2]);
		}
		printf("Usage: pfe load elf <parameters>\n");
	}
}
#if 0
static void pfe_command_lib(int argc, char *argv[])
{
	if (argc >= 3 && strcmp(argv[2], "init") == 0)
	{
		if (argc == 3)
			pfe_lib_init((void *)COMCERTO_AXI_HFE_CFG_BASE, (void *)CONFIG_DDR_BASEADDR, CONFIG_DDR_PHYS_BASEADDR);
		else if (argc == 6)
		{
			u32 cbus_base;
			u32 ddr_base;
			u32 ddr_phys_base;
			cbus_base = simple_strtoul(argv[3], NULL, 16);
			ddr_base = simple_strtoul(argv[4], NULL, 16);
			ddr_phys_base = simple_strtoul(argv[5], NULL, 16);
			pfe_lib_init((void *)cbus_base, (void *)ddr_base, ddr_phys_base);
		}
		else
		{
			printf("Usage: pfe lib init [<cbus_base> <ddr_base> <ddr_phys_base>]\n");
		}
	}
	else
	{
		if (argc >= 3 && strcmp(argv[2], "help") != 0)
		{
			printf("Unknown option: %s\n", argv[2]);
		}
		printf("Usage: pfe lib init <parameters>\n");
	}
}
#endif
static void pfe_command_pe(int argc, char * const argv[])
{
	if (argc >= 3 && strcmp(argv[2], "pmem") == 0)
	{
		if (argc >= 4 && strcmp(argv[3], "read") == 0)
		{
			int i;
			int num;
			int id;
			u32 addr;
			u32 size;
			u32 val;
			if (argc == 7)
				num = simple_strtoul(argv[6], NULL, 0);
			else if (argc == 6)
				num = 1;
			else
			{
				printf("Usage: pfe pe pmem read <id> <addr> [<num>]\n");
				return;
			}
			id = simple_strtoul(argv[4], NULL, 0);
			addr = simple_strtoul(argv[5], NULL, 16);
			size = 4;
			for (i = 0; i < num; i++, addr += 4)
			{
				val = pe_pmem_read(id, addr, size);
				val = be32_to_cpu(val);
				if(!(i&3)) printf("%08x: ", addr);
				printf("%08x%s", val, i == num - 1 || (i & 3) == 3 ? "\n" : " ");
			}
		}
		else
		{
			printf("Usage: pfe pe pmem read <parameters>\n");
		}
	}
	else if (argc >= 3 && strcmp(argv[2], "dmem") == 0)
	{
		if (argc >= 4 && strcmp(argv[3], "read") == 0)
		{
			int i;
			int num;
			int id;
			u32 addr;
			u32 size;
			u32 val;
			if (argc == 7)
				num = simple_strtoul(argv[6], NULL, 0);
			else if (argc == 6)
				num = 1;
			else
			{
				printf("Usage: pfe pe dmem read <id> <addr> [<num>]\n");
				return;
			}
			id = simple_strtoul(argv[4], NULL, 0);
			addr = simple_strtoul(argv[5], NULL, 16);
			size = 4;
			for (i = 0; i < num; i++, addr += 4)
			{
				val = pe_dmem_read(id, addr, size);
				val = be32_to_cpu(val);
				if(!(i&3)) printf("%08x: ", addr);
				printf("%08x%s", val, i == num - 1 || (i & 3) == 3 ? "\n" : " ");
			}
		}
		else if (argc >= 4 && strcmp(argv[3], "write") == 0)
		{
			int id;
			u32 val;
			u32 addr;
			u32 size;
			if (argc != 7)
			{
				printf("Usage: pfe pe dmem write <id> <val> <addr>\n");
				return;
			}
			id = simple_strtoul(argv[4], NULL, 0);
			val = simple_strtoul(argv[5], NULL, 16);
			val = cpu_to_be32(val);
			addr = simple_strtoul(argv[6], NULL, 16);
			size = 4;
			pe_dmem_write(id, val, addr, size);
		}
		else
		{
			printf("Usage: pfe pe dmem [read | write] <parameters>\n");
		}
	}
	else if (argc >= 3 && strcmp(argv[2], "lmem") == 0)
	{
		if (argc >= 4 && strcmp(argv[3], "read") == 0)
		{
			int i;
			int num;
			u32 val;
			u32 offset;
			if (argc == 6)
				num = simple_strtoul(argv[5], NULL, 0);
			else if (argc == 5)
				num = 1;
			else
			{
				printf("Usage: pfe pe lmem read <offset> [<num>]\n");
				return;
			}
			offset = simple_strtoul(argv[4], NULL, 16);
			for (i = 0; i < num; i++, offset += 4)
			{
				pe_lmem_read(&val, 4, offset);
				val = be32_to_cpu(val);
				printf("%08x%s", val, i == num - 1 || (i & 7) == 7 ? "\n" : " ");
			}
		}
		else if (argc >= 4 && strcmp(argv[3], "write") == 0)
		{
			u32 val;
			u32 offset;
			if (argc != 6)
			{
				printf("Usage: pfe pe lmem write <val> <offset>\n");
				return;
			}
			val = simple_strtoul(argv[4], NULL, 16);
			val = cpu_to_be32(val);
			offset = simple_strtoul(argv[5], NULL, 16);
			pe_lmem_write(&val, 4, offset);
		}
		else
		{
			printf("Usage: pfe pe lmem [read | write] <parameters>\n");
		}
	}
	else
	{
		if (strcmp(argv[2], "help") != 0)
		{
			printf("Unknown option: %s\n", argv[2]);
		}
		printf("Usage: pfe pe <parameters>\n");
	}
	//void pe_mem_memcpy_to32(int id, u32 mem_access_addr, const void *src, unsigned int len)
	//void pe_dmem_memcpy_to32(int id, u32 dst, const void *src, unsigned int len)
	//void pe_pmem_memcpy_to32(int id, u32 dst, const void *src, unsigned int len)
	//int pe_load_elf_section(int id, const void *data, Elf32_Shdr *shdr)
}

#if 0
static void pfe_command_gemac(int argc, char *argv[])
{
void gemac_init(void *base, void *cfg)
void gemac_set_speed(void *base, MAC_SPEED gem_speed)
void gemac_set_duplex(void *base, int duplex)
void gemac_set_mode(void *base, int mode)
void gemac_reset(void *base)
void gemac_enable(void *base)
void gemac_disable(void *base)
void gemac_set_address(void *base, SPEC_ADDR *addr)
SPEC_ADDR gemac_get_address(void *base)
void gemac_set_laddr1(void *base, MAC_ADDR *address)
void gemac_set_laddr2(void *base, MAC_ADDR *address)
void gemac_set_laddr3(void *base, MAC_ADDR *address)
void gemac_set_laddr4(void *base, MAC_ADDR *address)
void gemac_set_laddrN(void *base, MAC_ADDR *address, unsigned int entry_index)
void gemac_allow_broadcast(void *base)
void gemac_no_broadcast(void *base)
void gemac_enable_unicast(void *base)
void gemac_disable_unicast(void *base)
void gemac_enable_multicast(void *base)
void gemac_disable_multicast(void *base)
void gemac_enable_fcs_rx(void *base)
void gemac_disable_fcs_rx(void *base)
void gemac_enable_1536_rx(void *base)
void gemac_disable_1536_rx(void *base)
void gemac_enable_pause_rx(void *base)
void gemac_disable_pause_rx(void *base)
void gemac_set_config(void *base, GEMAC_CFG *cfg)
unsigned int * gemac_get_stats(void *base)
}
#endif

#if 0
static void pfe_command_gem(int argc, char *argv[])
{
MAC_ADDR gem_get_laddr1(void *base)
MAC_ADDR gem_get_laddr2(void *base)
MAC_ADDR gem_get_laddr3(void *base)
MAC_ADDR gem_get_laddr4(void *base)
MAC_ADDR gem_get_laddrN(void *base, unsigned int entry_index)
}
#endif

#if 0
static void pfe_command_gpi(int argc, char *argv[])
{
void gpi_init(void *base, GPI_CFG *cfg)
void gpi_reset(void *base)
void gpi_enable(void *base)
void gpi_disable(void *base)
void gpi_set_config(void *base, GPI_CFG *cfg)
}
#endif

#if 1
static void pfe_command_class(int argc, char * const argv[])
{
	if (argc >= 3 && strcmp(argv[2], "init") == 0)
	{
		CLASS_CFG cfg;
		if (argc == 3)
		{
#define CONFIG_DDR_PHYS_BASEADDR	0x03800000
			cfg.route_table_hash_bits = ROUTE_TABLE_HASH_BITS;
			cfg.route_table_baseaddr = CONFIG_DDR_PHYS_BASEADDR + ROUTE_TABLE_BASEADDR;
		}
		else if (argc == 5)
		{
			cfg.route_table_hash_bits = simple_strtoul(argv[3], NULL, 16);
			cfg.route_table_baseaddr = simple_strtoul(argv[4], NULL, 16);
		}
		else
		{
			printf("Usage: pfe class init <route_table_hash_bits> <route_table_baseaddr>\n");
		}
		class_init(&cfg);
	}
	else if (argc == 3 && strcmp(argv[2], "reset") == 0)
	{
		class_reset();
	}
	else if (argc == 3 && strcmp(argv[2], "enable") == 0)
	{
		class_enable();
	}
	else if (argc == 3 && strcmp(argv[2], "disable") == 0)
	{
		class_disable();
	}
	else if (argc >= 3 && strcmp(argv[2], "config") == 0)
	{
		CLASS_CFG cfg;
		if (argc == 3)
		{
			cfg.route_table_hash_bits = ROUTE_TABLE_HASH_BITS;
			cfg.route_table_baseaddr = CONFIG_DDR_PHYS_BASEADDR + ROUTE_TABLE_BASEADDR;
		}
		else if (argc == 5)
		{
			cfg.route_table_hash_bits = simple_strtoul(argv[3], NULL, 16);
			cfg.route_table_baseaddr = simple_strtoul(argv[4], NULL, 16);
		}
		else
		{
			printf("Usage: pfe class config <route_table_hash_bits> <route_table_baseaddr>\n");
		}
		class_set_config(&cfg);
	}
	else if (argc >= 3 && strcmp(argv[2], "bus") == 0)
	{
		if (argc >= 4 && strcmp(argv[3], "read") == 0)
		{
			u32 addr;
			u32 size;
			u32 val;
			if (argc != 6)
			{
				printf("Usage: pfe class bus read <addr> <size>\n");
				return;
			}
			addr = simple_strtoul(argv[4], NULL, 16);
			size = simple_strtoul(argv[5], NULL, 16);
			val = class_bus_read(addr, size);
			printf("%08x\n", val);
		}
		else if (argc >= 4 && strcmp(argv[3], "write") == 0)
		{
			u32 val;
			u32 addr;
			u32 size;
			if (argc != 7)
			{
				printf("Usage: pfe class bus write <val> <addr> <size>\n");
				return;
			}
			val = simple_strtoul(argv[4], NULL, 16);
			addr = simple_strtoul(argv[5], NULL, 16);
			size = simple_strtoul(argv[6], NULL, 16);
			class_bus_write(val, addr, size);
		}
		else
		{
			printf("Usage: pfe class bus [read | write] <parameters>\n");
		}
	}
	else
	{
		if (argc >= 3 && strcmp(argv[2], "help") != 0)
		{
			printf("Unknown option: %s\n", argv[2]);
		}
		printf("Usage: pfe class [init | reset | enable | disable | config | bus] <parameters>\n");
	}
}

static void pfe_command_tmu(int argc, char * const argv[])
{
	if (argc >= 3 && strcmp(argv[2], "init") == 0)
	{
		if (argc == 5)
		{
			TMU_CFG cfg;
			cfg.llm_base_addr = simple_strtoul(argv[3], NULL, 16);
			cfg.llm_queue_len = simple_strtoul(argv[4], NULL, 16);
			tmu_init(&cfg);
		}
		else
		{
			printf("Usage: pfe tmu init <llm_base_addr> <llm_queue_len>\n");
		}
	}
	else if (argc >= 3 && strcmp(argv[2], "enable") == 0)
	{
		if (argc == 4)
		{
			u32 mask;
			mask = simple_strtoul(argv[3], NULL, 16);
			tmu_enable(mask);
		}
		else
		{
			printf("Usage: pfe tmu enable <pe_mask>\n");
		}
	}
	else if (argc >= 3 && strcmp(argv[2], "disable") == 0)
	{
		if (argc == 4)
		{
			u32 mask;
			mask = simple_strtoul(argv[3], NULL, 16);
			tmu_disable(mask);
		}
		else
		{
			printf("Usage: pfe tmu disable <pe_mask>\n");
		}
	}
	else
	{
		if (argc >= 3 && strcmp(argv[2], "help") != 0)
		{
			printf("Unknown option: %s\n", argv[2]);
		}
		printf("Usage: pfe tmu [init | enable | disable] <parameters>\n");
	}
}
#endif

/** qm_read_drop_stat
 * This function is used to read the drop statistics from the TMU
 * hw drop counter.  Since the hw counter is always cleared afer
 * reading, this function maintains the previous drop count, and
 * adds the new value to it.  That value can be retrieved by
 * passing a pointer to it with the total_drops arg.
 *
 * @param tmu           TMU number (0 - 3)
 * @param queue         queue number (0 - 15)
 * @param total_drops   pointer to location to store total drops (or NULL)
 * @param do_reset      if TRUE, clear total drops after updating
 *
 */

u32 qm_read_drop_stat(u32 tmu, u32 queue, u32 *total_drops, int do_reset)
{
#define NUM_QUEUES		16
	static u32 qtotal[TMU_MAX_ID + 1][NUM_QUEUES];
	u32 val;
	writel((tmu << 8) | queue, TMU_TEQ_CTRL);
	writel((tmu << 8) | queue, TMU_LLM_CTRL);
	val = readl(TMU_TEQ_DROP_STAT);
	qtotal[tmu][queue] += val;
	if (total_drops)
		*total_drops = qtotal[tmu][queue];
	if (do_reset)
		qtotal[tmu][queue] = 0;
	return val;
}

static ssize_t tmu_queue_stats(char *buf, int tmu, int queue)
{
	ssize_t len = 0;
	u32 drops;

	printf("%d-%02d, ", tmu, queue);

	drops = qm_read_drop_stat(tmu, queue, NULL, 0);

	/* Select queue */
	writel((tmu << 8) | queue, TMU_TEQ_CTRL);
	writel((tmu << 8) | queue, TMU_LLM_CTRL);

	printf("(teq) drop: %10u, tx: %10u (llm) head: %08x, tail: %08x, drop: %10u\n",
			drops, readl(TMU_TEQ_TRANS_STAT),
			readl(TMU_LLM_QUE_HEADPTR), readl(TMU_LLM_QUE_TAILPTR),
			readl(TMU_LLM_QUE_DROPCNT));

	return len;
}


static ssize_t tmu_queues(char *buf, int tmu)
{
	ssize_t len = 0;
	int queue;

	for (queue = 0; queue < 16; queue++)
		len += tmu_queue_stats(buf + len, tmu, queue);

	return len;
}

void hif_status(void)
{
	printf("hif:\n");

	printf("  tx curr bd:    %x\n", readl(HIF_TX_CURR_BD_ADDR));
	printf("  tx status:     %x\n", readl(HIF_TX_STATUS));
	printf("  tx dma status: %x\n", readl(HIF_TX_DMA_STATUS));

	printf("  rx curr bd:    %x\n", readl(HIF_RX_CURR_BD_ADDR));
	printf("  rx status:     %x\n", readl(HIF_RX_STATUS));
	printf("  rx dma status: %x\n", readl(HIF_RX_DMA_STATUS));

	printf("hif nocopy:\n");

	printf("  tx curr bd:    %x\n", readl(HIF_NOCPY_TX_CURR_BD_ADDR));
	printf("  tx status:     %x\n", readl(HIF_NOCPY_TX_STATUS));
	printf("  tx dma status: %x\n", readl(HIF_NOCPY_TX_DMA_STATUS));

	printf("  rx curr bd:    %x\n", readl(HIF_NOCPY_RX_CURR_BD_ADDR));
	printf("  rx status:     %x\n", readl(HIF_NOCPY_RX_STATUS));
	printf("  rx dma status: %x\n", readl(HIF_NOCPY_RX_DMA_STATUS));
}

static void  gpi(int id, void *base)
{
        u32 val;

        printf("gpi%d:\n  ", id);

        printf("  tx under stick: %x\n", readl(base + GPI_FIFO_STATUS));
        val = readl(base + GPI_FIFO_DEBUG);
        printf("  tx pkts:        %x\n", (val >> 23) & 0x3f);
        printf("  rx pkts:        %x\n", (val >> 18) & 0x3f);
        printf("  tx bytes:       %x\n", (val >> 9) & 0x1ff);
        printf("  rx bytes:       %x\n", (val >> 0) & 0x1ff);
        printf("  overrun:        %x\n", readl(base + GPI_OVERRUN_DROPCNT));
}

void  bmu(int id, void *base)
{
	printf("bmu: %d\n", id);
	printf("  buf size:  %x\n", (1 << readl(base + BMU_BUF_SIZE)));
	printf("  buf count: %x\n", readl(base + BMU_BUF_CNT));
	printf("  buf rem:   %x\n", readl(base + BMU_REM_BUF_CNT));
	printf("  buf curr:  %x\n", readl(base + BMU_CURR_BUF_CNT));
	printf("  free err:  %x\n", readl(base + BMU_FREE_ERR_ADDR));
}

#define	PESTATUS_ADDR_CLASS	0x800
#define PEMBOX_ADDR_CLASS	0x890
#define	PESTATUS_ADDR_TMU	0x80
#define PEMBOX_ADDR_TMU		0x290
#define	PESTATUS_ADDR_UTIL	0x0

static void pfe_pe_status(int argc, char * const argv[])
{
	int do_clear = 0;
	u32 id;
	u32 dmem_addr;
	u32 cpu_state;
	u32 activity_counter;
	u32 rx;
	u32 tx;
	u32 drop;
	char statebuf[5];
	u32 class_debug_reg = 0;
#ifdef CONFIG_PFE_WARN_WA
	u32 debug_indicator;
	u32 debug[16];
	int j;
#endif
	if (argc == 4 && strcmp(argv[3], "clear") == 0)
		do_clear = 1;

	for (id = CLASS0_ID; id < MAX_PE; id++)
	{
#if !defined(CONFIG_UTIL_PE_DISABLED)
		if (id == UTIL_ID)
		{
			printf("util:\n");
			dmem_addr = PESTATUS_ADDR_UTIL;
		}
		else if (id >= TMU0_ID)
#else
		if (id >= TMU0_ID)
#endif
		{
			if (id == TMU2_ID)
				continue;
			if (id == TMU0_ID)
				printf("tmu:\n");
			dmem_addr = PESTATUS_ADDR_TMU;
		}
		else
		{
			if (id == CLASS0_ID)
				printf("class:\n");
			dmem_addr = PESTATUS_ADDR_CLASS;
			class_debug_reg = readl(CLASS_PE0_DEBUG + id * 4);
		}
		cpu_state = pe_dmem_read(id, dmem_addr, 4);
		dmem_addr += 4;
		memcpy(statebuf, (char *)&cpu_state, 4);
		statebuf[4] = '\0';
		activity_counter = pe_dmem_read(id, dmem_addr, 4);
		dmem_addr += 4;
		rx = pe_dmem_read(id, dmem_addr, 4);
		if (do_clear)
			pe_dmem_write(id, 0, dmem_addr, 4);
		dmem_addr += 4;
		tx = pe_dmem_read(id, dmem_addr, 4);
		if (do_clear)
			pe_dmem_write(id, 0, dmem_addr, 4);
		dmem_addr += 4;
		drop = pe_dmem_read(id, dmem_addr, 4);
		if (do_clear)
			pe_dmem_write(id, 0, dmem_addr, 4);
		dmem_addr += 4;
#if !defined(CONFIG_UTIL_PE_DISABLED)
		if (id == UTIL_ID)
		{
			printf("state=%4s ctr=%08x rx=%x tx=%x\n",
					statebuf, cpu_to_be32(activity_counter),
					cpu_to_be32(rx), cpu_to_be32(tx));
		}
		else
#endif
		if (id >= TMU0_ID)
		{
			printf("%d: state=%4s ctr=%08x rx=%x qstatus=%x\n",
					id - TMU0_ID, statebuf, cpu_to_be32(activity_counter),
					cpu_to_be32(rx), cpu_to_be32(tx));
		}
		else
		{
			printf("%d: pc=1%04x ldst=%04x state=%4s ctr=%08x rx=%x tx=%x drop=%x\n",
					id - CLASS0_ID, class_debug_reg & 0xFFFF, class_debug_reg >> 16,
					statebuf, cpu_to_be32(activity_counter),
					cpu_to_be32(rx), cpu_to_be32(tx), cpu_to_be32(drop));
		}
#ifdef CONFIG_PFE_WARN_WA
		debug_indicator = pe_dmem_read(id, dmem_addr, 4);
		dmem_addr += 4;
		if (debug_indicator == cpu_to_be32('DBUG'))
		{
			int last = 0;
			for (j = 0; j < 16; j++)
			{
				debug[j] = pe_dmem_read(id, dmem_addr, 4);
				if (debug[j])
				{
					last = j + 1;
					if (do_clear)
						pe_dmem_write(id, 0, dmem_addr, 4);
				}
				dmem_addr += 4;
			}
			for (j = 0; j < last; j++)
			{
				printf("%08x%s", cpu_to_be32(debug[j]), (j & 0x7) == 0x7 || j == last - 1 ? "\n" : " ");
			}
		}
#endif
	}

}

static void pfe_command_status(int argc, char * const argv[])
{

	if (argc >= 3 && strcmp(argv[2], "pe") == 0)
	{
		pfe_pe_status(argc, argv);
	}
	else if (argc == 3 && strcmp(argv[2], "bmu") == 0)
	{
		bmu(1, BMU1_BASE_ADDR);
		bmu(2, BMU2_BASE_ADDR);
	}
	else if (argc == 3 && strcmp(argv[2], "hif") == 0)
	{
		hif_status();
	}
	else if (argc == 3 && strcmp(argv[2], "gpi") == 0)
	{
		gpi(0, EGPI1_BASE_ADDR);
		gpi(1, EGPI2_BASE_ADDR);
		gpi(3, HGPI_BASE_ADDR);
	}
	else if (argc == 3 && strcmp(argv[2], "tmu0_queues") == 0)
	{
		tmu_queues(NULL, 0);
	}
	else if (argc == 3 && strcmp(argv[2], "tmu1_queues") == 0)
	{
		tmu_queues(NULL, 1);
	}
	else if (argc == 3 && strcmp(argv[2], "tmu3_queues") == 0)
	{
		tmu_queues(NULL, 3);
	}
	else
		printf("Usage: pfe status [pe <clear> | bmu | gpi | hif | tmuX_queues ]\n");

	return;
}


#define EXPT_DUMP_ADDR 0x1fa8
#define EXPT_REG_COUNT 20
static const char *register_names[EXPT_REG_COUNT] = {
		"  pc", "ECAS", " EID", "  ED",
		"  sp", "  r1", "  r2", "  r3",
		"  r4", "  r5", "  r6", "  r7",
		"  r8", "  r9", " r10", " r11",
		" r12", " r13", " r14", " r15"
};

static void pfe_command_expt(int argc, char * const argv[])
{
	unsigned int id, i, val, addr;

	if (argc == 3)
	{
		id = simple_strtoul(argv[2], NULL, 0);
		addr = EXPT_DUMP_ADDR;
		printf("Exception information for PE %d:\n", id);
		for (i = 0; i < EXPT_REG_COUNT; i++)
		{
			val = pe_dmem_read(id, addr, 4);
			val = be32_to_cpu(val);
			printf("%s:%08x%s", register_names[i], val, (i & 3) == 3 ? "\n" : " ");
			addr += 4;
		}
	}
	else
	{
		printf("Usage: pfe expt <id>\n");
	}
}

static void pfe_command_util(int argc, char * const argv[])
{
	if (argc == 3 && strcmp(argv[2], "init") == 0)
	{
		UTIL_CFG cfg;
		util_init(&cfg);
	}
	else if (argc == 3 && strcmp(argv[2], "reset") == 0)
	{
		util_reset();
	}
	else if (argc == 3 && strcmp(argv[2], "enable") == 0)
	{
		util_enable();
	}
	else if (argc == 3 && strcmp(argv[2], "disable") == 0)
	{
		util_disable();
	}
	else if (argc >= 3 && strcmp(argv[2], "bus") == 0)
		{
			if (argc >= 4 && strcmp(argv[3], "read") == 0)
			{
				u32 addr;
				u32 size;
				u32 val;
				if (argc != 6)
				{
					printf("Usage: pfe util bus read <addr> <size>\n");
					return;
				}
				addr = simple_strtoul(argv[4], NULL, 16);
				size = simple_strtoul(argv[5], NULL, 16);
				val = util_bus_read(addr, size);
				printf("%08x\n", val);
			}
			else if (argc >= 4 && strcmp(argv[3], "write") == 0)
			{
				u32 val;
				u32 addr;
				u32 size;
				if (argc != 7)
				{
					printf("Usage: pfe util bus write <val> <addr> <size>\n");
					return;
				}
				val = simple_strtoul(argv[4], NULL, 16);
				addr = simple_strtoul(argv[5], NULL, 16);
				size = simple_strtoul(argv[6], NULL, 16);
				util_bus_write(val, addr, size);
			}
			else
			{
				printf("Usage: pfe util bus [read | write] <parameters>\n");
			}
		}
	else
	{
		if (argc >= 3 && strcmp(argv[2], "help") != 0)
		{
			printf("Unknown option: %s\n", argv[2]);
		}
		printf("Usage: pfe util [init | reset | enable | disable | bus] <parameters>\n");
	}
}

#if 0
static void pfe_command_hif(int argc, char *argv[])
{
void hif_nocpy_init(void)
void hif_init(void)
void hif_tx_enable(void)
void hif_tx_disable(void)
void hif_rx_enable(void)
void hif_rx_disable(void)
}
#endif
#define ROUTE_TABLE_START	(CONFIG_DDR_PHYS_BASEADDR+ROUTE_TABLE_BASEADDR)
static void pfe_command_fftest(int argc, char * const argv[])
{
	bd_t *bd = gd->bd;
	struct eth_device *edev_eth0;
	struct eth_device *edev_eth1;

	// open eth0 and eth1 
	edev_eth0 = eth_get_dev_by_name("pfe_eth0");
	if (!edev_eth0)
	{
		printf("Cannot access eth0\n");
		return;
	}

	if (eth_write_hwaddr(edev_eth0, "eth", edev_eth0->index))
		puts("\nWarning: failed to set MAC address for c2000_gemac0\n");

	if (edev_eth0->state != ETH_STATE_ACTIVE)
	{
		if (edev_eth0->init(edev_eth0, bd) < 0) {
			printf("eth0 init failed\n");
			return;
		}
		edev_eth0->state = ETH_STATE_ACTIVE;
	}

	edev_eth1 = eth_get_dev_by_name("pfe_eth1");
	if (!edev_eth1)
	{
		printf("Cannot access eth1\n");
		return;
	}

	if (eth_write_hwaddr(edev_eth1, "eth", edev_eth1->index))
		puts("\nWarning: failed to set MAC address for c2000_gemac1\n");

	if (edev_eth1->state != ETH_STATE_ACTIVE)
	{
		if (edev_eth1->init(edev_eth1, bd) < 0) {
			printf("eth1 init failed\n");
			return;
		}
		edev_eth1->state = ETH_STATE_ACTIVE;
	}

}

#ifdef CONFIG_CMD_PFE_START
static void pfe_command_start(int argc, char * const argv[])
{
	printf("Starting PFE \n");
	ls1012a_gemac_initialize(gd->bd, 0 , "pfe_eth0");
	ls1012a_gemac_initialize(gd->bd, 1 , "pfe_eth1");
}
#endif

#ifdef PFE_LS1012A_RESET_WA
/*This function sends a dummy packet to HIF through TMU3 */
static void send_dummy_pkt_to_hif(void)
{
	u32 buf;
	static u32 dummy_pkt[] =  {
		0x4200800a, 0x01000003, 0x00018100, 0x00000000,
		0x33221100, 0x2b785544, 0xd73093cb, 0x01000608,
		0x04060008, 0x2b780200, 0xd73093cb, 0x0a01a8c0,
		0x33221100, 0xa8c05544, 0x00000301, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0xbe86c51f };

	/*Allocate BMU2 buffer */
	buf = readl(BMU2_BASE_ADDR + BMU_ALLOC_CTRL);

	debug("Sending a dummy pkt to HIF %x\n", buf);
	buf += 0x80;
	memcpy((void *)DDR_PFE_TO_VIRT(buf), dummy_pkt, sizeof(dummy_pkt));
	/*Write length and pkt to TMU*/
	writel(0x03000042, TMU_PHY_INQ_PKTPTR);
	writel(buf, TMU_PHY_INQ_PKTINFO);

}

static void pfe_command_stop(int argc, char * const argv[])
{
	int id, hif_stop_loop = 10;
	u32 rx_status;
	printf("Stopping PFE... \n");

	/*Mark all descriptors as LAST_BD */
	hif_rx_desc_disable();

	/*If HIF Rx BDP is busy send a dummy packet */
	do {
		rx_status = readl(HIF_RX_STATUS);
		if (rx_status & BDP_CSR_RX_DMA_ACTV)
			send_dummy_pkt_to_hif();
		udelay(10);
	} while (hif_stop_loop--);

	if(readl(HIF_RX_STATUS) & BDP_CSR_RX_DMA_ACTV)
		printf("Unable to stop HIF\n");

	/*Disable Class PEs */

	for (id = CLASS0_ID; id <= CLASS_MAX_ID; id++)
	{
		/*Inform PE to stop */
		pe_dmem_write(id, cpu_to_be32(1), PEMBOX_ADDR_CLASS, 4);
		udelay(10);

		/*Read status */
		if(!pe_dmem_read(id, PEMBOX_ADDR_CLASS+4, 4))
			printf("Failed to stop PE%d\n", id);
	}
	/*Disable TMU PEs */
	for (id = TMU0_ID; id <= TMU_MAX_ID; id++)
	{
		if(id == TMU2_ID) continue;

		/*Inform PE to stop */
		pe_dmem_write(id, 1, PEMBOX_ADDR_TMU, 4);
		udelay(10);

		/*Read status */
		if(!pe_dmem_read(id, PEMBOX_ADDR_TMU+4, 4))
			printf("Failed to stop PE%d\n", id);
	}
}
#endif

static int pfe_command(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	if (argc == 1 || strcmp(argv[1], "help") == 0)
	{
		pfe_command_help();
		return CMD_RET_SUCCESS;
	}
	if (strcmp(argv[1], "firmware") == 0)
		pfe_command_firmware(argc, argv);
	else if (strcmp(argv[1], "load") == 0)
		pfe_command_load(argc, argv);
#if 0
	else if (strcmp(argv[1], "lib") == 0)
		pfe_command_lib(argc, argv);
#endif
	else if (strcmp(argv[1], "pe") == 0)
		pfe_command_pe(argc, argv);
#if 0
	else if (strcmp(argv[1], "gemac") == 0)
		pfe_command_gemac(argc, argv);
	else if (strcmp(argv[1], "gem") == 0)
		pfe_command_gem(argc, argv);
	else if (strcmp(argv[1], "gpi") == 0)
		pfe_command_gpi(argc, argv);
#endif
#if 1
	else if (strcmp(argv[1], "class") == 0)
		pfe_command_class(argc, argv);
	else if (strcmp(argv[1], "tmu") == 0)
		pfe_command_tmu(argc, argv);
#endif
	else if (strcmp(argv[1], "status") == 0)
		pfe_command_status(argc, argv);
	else if (strcmp(argv[1], "expt") == 0)
		pfe_command_expt(argc, argv);
	else if (strcmp(argv[1], "util") == 0)
		pfe_command_util(argc, argv);
#if 0
	else if (strcmp(argv[1], "hif") == 0)
		pfe_command_hif(argc, argv);
#endif
	else if (strcmp(argv[1], "fftest") == 0)
		pfe_command_fftest(argc, argv);
#ifdef CONFIG_CMD_PFE_START
	else if (strcmp(argv[1], "start") == 0)
		pfe_command_start(argc, argv);
#endif
#ifdef PFE_LS1012A_RESET_WA
	else if (strcmp(argv[1], "stop") == 0)
		pfe_command_stop(argc, argv);
#endif
	else
	{
		printf("Unknown option: %s\n", argv[1]);
		pfe_command_help();
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}


U_BOOT_CMD(
	pfe,	7,	1,	pfe_command,
	"Performs PFE lib utility functions",
	"Usage: \n"
	"pfe <options>"
);
