/*
 *  (C) Copyright 2011
 *  Author : Mindspeed Technologes
 *  
 *  See file CREDITS for list of people who contributed to this
 *  project.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA 02111-1307 USA
 * */


#ifndef _PFE_MOD_H_
#define _PFE_MOD_H_

#include <linux/device.h>

#include "pfe/pfe.h"
#include "pfe/cbus.h"
#include "pfe/cbus/bmu.h"

#include "pfe_driver.h"

struct pfe;


struct pfe {
	unsigned long ddr_phys_baseaddr;
	void *ddr_baseaddr;
	void *cbus_baseaddr;
	void *apb_baseaddr;
	void *iram_baseaddr;
	int hif_irq;
	struct device *dev;
	struct pci_dev *pdev;

#if 0
	struct pfe_ctrl ctrl;
	struct pfe_hif hif;
	struct pfe_eth eth;
#endif
};

extern struct pfe *pfe;

int pfe_probe(struct pfe *pfe);
int pfe_remove(struct pfe *pfe);

#ifndef SZ_1K
#define SZ_1K 1024
#endif

#ifndef SZ_1M
#define SZ_1M (1024 * 1024)
#endif

/* DDR Mapping */
#if !defined(CONFIG_PLATFORM_PCI)
#define UTIL_CODE_BASEADDR	0
#define UTIL_CODE_SIZE		(128 * SZ_1K)
#define UTIL_DDR_DATA_BASEADDR	(UTIL_CODE_BASEADDR + UTIL_CODE_SIZE)
#define UTIL_DDR_DATA_SIZE	(64 * SZ_1K)
#define CLASS_DDR_DATA_BASEADDR	(UTIL_DDR_DATA_BASEADDR + UTIL_DDR_DATA_SIZE)
#define CLASS_DDR_DATA_SIZE	(32 * SZ_1K)
#define TMU_DDR_DATA_BASEADDR	(CLASS_DDR_DATA_BASEADDR + CLASS_DDR_DATA_SIZE)
#define TMU_DDR_DATA_SIZE	(32 * SZ_1K)
#define ROUTE_TABLE_BASEADDR	(TMU_DDR_DATA_BASEADDR + TMU_DDR_DATA_SIZE)
#define ROUTE_TABLE_HASH_BITS	15	/**< 32K entries */
#define ROUTE_TABLE_SIZE	((1 << ROUTE_TABLE_HASH_BITS) * CLASS_ROUTE_SIZE)
#define BMU2_DDR_BASEADDR	(ROUTE_TABLE_BASEADDR + ROUTE_TABLE_SIZE)
#define BMU2_BUF_COUNT		(4096 - 256)			/**< This is to get a total DDR size of 12MiB */
#define BMU2_DDR_SIZE		(DDR_BUF_SIZE * BMU2_BUF_COUNT)
#define TMU_LLM_BASEADDR	(BMU2_DDR_BASEADDR + BMU2_DDR_SIZE)
#define TMU_LLM_QUEUE_LEN	(16 * 256)			/**< Must be power of two and at least 16 * 8 = 128 bytes */
#define TMU_LLM_SIZE		(4 * 16 * TMU_LLM_QUEUE_LEN)	/**< (4 TMU's x 16 queues x queue_len) */

#if (TMU_LLM_BASEADDR + TMU_LLM_SIZE) > 0xC00000
#error DDR mapping above 12MiB
#endif

#else

#define UTIL_CODE_BASEADDR	0
#if defined(CONFIG_UTIL_PE_DISABLED)
#define UTIL_CODE_SIZE		(0 * SZ_1K)
#else
#define UTIL_CODE_SIZE		(8 * SZ_1K)
#endif
#define UTIL_DDR_DATA_BASEADDR	(UTIL_CODE_BASEADDR + UTIL_CODE_SIZE)
#define UTIL_DDR_DATA_SIZE	(0 * SZ_1K)
#define CLASS_DDR_DATA_BASEADDR	(UTIL_DDR_DATA_BASEADDR + UTIL_DDR_DATA_SIZE)
#define CLASS_DDR_DATA_SIZE	(0 * SZ_1K)
#define TMU_DDR_DATA_BASEADDR	(CLASS_DDR_DATA_BASEADDR + CLASS_DDR_DATA_SIZE)
#define TMU_DDR_DATA_SIZE	(0 * SZ_1K)
#define ROUTE_TABLE_BASEADDR	(TMU_DDR_DATA_BASEADDR + TMU_DDR_DATA_SIZE)
#define ROUTE_TABLE_HASH_BITS	5	/**< 32 entries */
#define ROUTE_TABLE_SIZE	((1 << ROUTE_TABLE_HASH_BITS) * CLASS_ROUTE_SIZE)
#define BMU2_DDR_BASEADDR	(ROUTE_TABLE_BASEADDR + ROUTE_TABLE_SIZE)
#define BMU2_BUF_COUNT		8
#define BMU2_DDR_SIZE		(DDR_BUF_SIZE * BMU2_BUF_COUNT)
#define TMU_LLM_BASEADDR	(BMU2_DDR_BASEADDR + BMU2_DDR_SIZE)
#define TMU_LLM_QUEUE_LEN	(16 * 8)			/**< Must be power of two and at least 16 * 8 = 128 bytes */
#define TMU_LLM_SIZE		(4 * 16 * TMU_LLM_QUEUE_LEN)	/**< (4 TMU's x 16 queues x queue_len) */
#define HIF_DESC_BASEADDR	(TMU_LLM_BASEADDR + TMU_LLM_SIZE)
#define HIF_RX_DESC_SIZE	(16*HIF_RX_DESC_NT)
#define HIF_TX_DESC_SIZE	(16*HIF_TX_DESC_NT)
#define HIF_DESC_SIZE		(HIF_RX_DESC_SIZE + HIF_TX_DESC_SIZE)
#define HIF_RX_PKT_DDR_BASEADDR	(HIF_DESC_BASEADDR + HIF_DESC_SIZE)
#define HIF_RX_PKT_DDR_SIZE	(HIF_RX_DESC_NT * DDR_BUF_SIZE)
#define HIF_TX_PKT_DDR_BASEADDR	(HIF_RX_PKT_DDR_BASEADDR + HIF_RX_PKT_DDR_SIZE)
#define HIF_TX_PKT_DDR_SIZE	(HIF_TX_DESC_NT * DDR_BUF_SIZE)
#define ROUTE_BASEADDR		(HIF_TX_PKT_DDR_BASEADDR + HIF_TX_PKT_DDR_SIZE)
#define ROUTE_SIZE		(2 * CLASS_ROUTE_SIZE)

#if (ROUTE_BASEADDR + ROUTE_SIZE) > 0x10000
#error DDR mapping above 64KiB
#endif

#define PFE_HOST_TO_PCI(addr)	(((u32)addr)- ((u32)DDR_BASE_ADDR))
#define PFE_PCI_TO_HOST(addr)	(((u32)addr)+ ((u32)DDR_BASE_ADDR))
#endif

/* LMEM Mapping */
#define BMU1_LMEM_BASEADDR	0
#define BMU1_BUF_COUNT		256
#define BMU1_LMEM_SIZE		(LMEM_BUF_SIZE * BMU1_BUF_COUNT)

#endif /* _PFE_MOD_H */
