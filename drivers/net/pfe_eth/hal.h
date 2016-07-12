/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef _HAL_H_
#define _HAL_H_

#if defined(CONFIG_PLATFORM_PCI)
/* For ChipIT */

#include <linux/types.h>
#include <linux/elf.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/firmware.h>


#define free(x)  kfree(x)
#define xzalloc(x)  kmalloc(x, GFP_DMA)
#define printf  printk

/* #define dprint(fmt, arg...)	printk(fmt, ##arg) */
#define dprint(fmt, arg...)

#else

#include <linux/types.h>
#include <elf.h>
#include <common.h>
/* #include <errno.h> */
#include <asm/byteorder.h>
#include <miiphy.h>
#include <malloc.h>
#include <asm/io.h>


#include "pfe_eth.h"

#endif


#endif /* _HAL_H_ */

