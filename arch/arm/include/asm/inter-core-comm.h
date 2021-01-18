// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2021 NXP
 *
 */
#ifndef _ARM_INTER_CORE_COMM_H
#define _ARM_INTER_CORE_COMM_H

#define ICC_RING_ENTRY 128	/* number of descriptor for each ring */
#define ICC_BLOCK_UNIT_SIZE (4 * 1024)	/* size of each block */

/* 2M space for core's ring and desc struct */
#define ICC_RING_DESC_SPACE (2 * 1024 * 1024)

#define ICC_CORE_MEM_SPACE (CONFIG_SYS_DDR_SDRAM_SHARE_SIZE / \
		CONFIG_MAX_CPUS) /* share memory size for each core icc */
#define ICC_CORE_MEM_BASE(x) (CONFIG_SYS_DDR_SDRAM_SHARE_BASE + \
		(x) * ICC_CORE_MEM_SPACE) /* share memory base for core x */
/* the ring struct addr of core x ring y */
#define ICC_CORE_RING_BASE(x, y) (ICC_CORE_MEM_BASE(x) + (y) * \
		sizeof(struct icc_ring))
#define ICC_CORE_DESC_BASE(x) (ICC_CORE_MEM_BASE(x) + CONFIG_MAX_CPUS * \
		sizeof(struct icc_ring)) /* the desc struct addr of core x */
#ifdef	CONFIG_ARCH_LX2160A
#define ICC_TRG_CORE (*(volatile unsigned *) \
		(CONFIG_SYS_DDR_SDRAM_SHARE_BASE \
		+ CONFIG_SYS_DDR_SDRAM_SHARE_SIZE - 8))
#endif

/*
 * The core x block memory base addr for icc data transfer.
 * The beginning 2M space of core x icc memory is for
 * core x ring and desc struct.
 */
#define ICC_CORE_BLOCK_BASE(x) (ICC_CORE_MEM_BASE(x) + ICC_RING_DESC_SPACE)
#define ICC_CORE_BLOCK_END(x) (ICC_CORE_MEM_BASE(x) + ICC_CORE_MEM_SPACE)
#define ICC_CORE_BLOCK_COUNT	((ICC_CORE_MEM_SPACE - \
			ICC_RING_DESC_SPACE)/ICC_BLOCK_UNIT_SIZE)

/*
 * ICC uses the number 8 SGI interrupt.
 * 0-7 are used by Linux SMP.
 */

#define ICC_SGI 8

struct icc_desc {
	unsigned long block_addr;	/* block address */
	unsigned int byte_count;	/* available bytes in the block */
};

struct icc_ring {
	/* which core created the ring */
	unsigned int src_coreid;
	/* which core the ring sends SGI to */
	unsigned int dest_coreid;
	/* which interrupt (SGI) be used */
	unsigned int interrupt;
	/* number of descriptor */
	unsigned int desc_num;
	/* pointer of the first descriptor */
	struct icc_desc *desc;
	/* desc ready to be sent, modified by producer */
	unsigned int desc_head;
	/* desc should be handled, modified by consumer */
	unsigned int desc_tail;
	/* statistic: add failed counts, ring full */
	unsigned long busy_counts;
	/* statistic: total interrupt number triggered */
	unsigned long interrupt_counts;
};

/*
 * check the state of ring for one destination core.
 * return: 0 - empty
 *         !0 - the working block address currently
 */
unsigned long icc_ring_state(int coreid);

/*
 * Request a block which is ICC_BLOCK_UNIT_SIZE size.
 *
 * return 0:failed, !0:block address can be used
 */
unsigned long icc_block_request(void);

/*
 * Send a SGI interrupt to cores from core_mask.
 * core_mask: the bitmask to set the cores.
 *		0x1 = core 0
 *		0x2 = core 1
 *		0x3 = core 0 and core 1
 *		NOTE: cannot send SGI to self-core
 * hw_irq: SGI interrupt, must be [0 - 15].
 */
void icc_set_sgi(int core_mask, unsigned int hw_irq);

/*
 * Free a block requested.
 *
 * Be careful if the destination cores are working on
 * this block.
 */
void icc_block_free(unsigned long block);

/*
 * Register icc callback handler.
 * The application can register different handler for
 * icc from different source core (src_coreid).
 *	- For example 4 cores system (0, 1, 2, 3)
 *	  core 2 can register 3 callback handlers
 *	  for icc come from core 0, core 1, core 3
 *	  with src_coreid value 0, 1, 3.
 *	  Core 2 also can register only one handler
 *	  for all other 3 cores with src_coreid value
 *	  4.
 *
 * int src_coreid: which core the icc coming from for the irq_handle
 *			- CONFIG_MAX_CPUS: the irq_handle for all cores
 *			  except self-core.
 *
 * void (*irq_handle)(int, unsigned long, unsigned int): the callback irq_handle
 *	irq_handle parameters:
 *		int: which core the icc coming from
 *		unsigned long: the start address of icc data
 *		unsigned int: the count of byte should be handled
 *
 * return: 0:success, -1:failed
 */
int icc_irq_register(int src_coreid,
	void (*irq_handle)(int, unsigned long, unsigned int));

/*
 * Send the data in the block to a core or multi-core.
 * This will trigger the SGI interrupt
 *
 * int core_mask: 0x110 including core 1, core 2, not including core 0, core 3
 * int byte_count: how many bytes are available in the block
 * void *block: The address of block
 *
 * return 0:success, -1:failed
 */
int icc_set_block(int core_mask, unsigned int byte_count, unsigned long block);

/* Show the icc ring state */
void icc_show(void);

#endif /* _ARM_INTER_CORE_COMM_H */
