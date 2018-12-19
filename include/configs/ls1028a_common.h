/*
 * Copyright 2017-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#ifndef __L1028A_COMMON_H
#define __L1028A_COMMON_H

/* SPL build */
#ifdef CONFIG_SPL_BUILD
#define SPL_NO_FMAN
#define SPL_NO_DSPI
#define SPL_NO_PCIE
#define SPL_NO_ENV
#define SPL_NO_MISC
#define SPL_NO_USB
#define SPL_NO_SATA
#define SPL_NO_QE
#define SPL_NO_EEPROM
#endif
#if (defined(CONFIG_SPL_BUILD) && defined(CONFIG_NAND_BOOT))
#define SPL_NO_MMC
#endif
#if (defined(CONFIG_SPL_BUILD) && defined(CONFIG_SD_BOOT_QSPI))
#define SPL_NO_IFC
#endif

#define CONFIG_REMAKE_ELF
#define CONFIG_FSL_LAYERSCAPE
#define CONFIG_MP

#include <asm/arch/stream_id_lsch3.h>
#include <asm/arch/config.h>
#include <asm/arch/soc.h>

/* Link Definitions */
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_FSL_OCRAM_BASE + 0xfff0)

#define CONFIG_SUPPORT_RAW_INITRD

#define CONFIG_SKIP_LOWLEVEL_INIT

#define CONFIG_VERY_BIG_RAM
#define CONFIG_SYS_DDR_SDRAM_BASE	0x80000000UL
#define CONFIG_SYS_FSL_DDR_SDRAM_BASE_PHY	0
#define CONFIG_SYS_SDRAM_BASE		CONFIG_SYS_DDR_SDRAM_BASE
#define CONFIG_SYS_DDR_BLOCK2_BASE	0x2080000000ULL
#define CONFIG_SYS_FSL_DDR_MAIN_NUM_CTRLS	1

/*
 * SMP Definitinos
 */
#define CPU_RELEASE_ADDR		secondary_boot_func

/* Generic Timer Definitions */
#define COUNTER_FREQUENCY		25000000	/* 25MHz */

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 2048 * 1024)

/* I2C */
#define CONFIG_SYS_I2C

/* Serial Port */
#define CONFIG_CONS_INDEX       1
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE     1
#define CONFIG_SYS_NS16550_CLK          (get_bus_freq(0) / 2)

#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/* ENETC */
#ifdef CONFIG_FSL_ENETC
#define CFG_ENETC_PHYS_ADDR 0x1f0000000ULL
#define CFG_ENETC_PHYS_SIZE 0x10000000UL
#endif

/* Miscellaneous configurable options */
#define CONFIG_SYS_LOAD_ADDR	(CONFIG_SYS_DDR_SDRAM_BASE + 0x10000000)

/* Physical Memory Map */
#define CONFIG_CHIP_SELECTS_PER_CTRL	4

#define CONFIG_NR_DRAM_BANKS		2

#define CONFIG_HWCONFIG
#define HWCONFIG_BUFFER_SIZE		128

#ifndef SPL_NO_ENV
/* Allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE

#define CONFIG_SYS_FLASH_BASE		0x20000000
#define CONFIG_ENV_SECT_SIZE 		0x20000
#define CONFIG_ENV_OFFSET		0x300000        /* 3MB */
#define CONFIG_ENV_ADDR			(CONFIG_SYS_FLASH_BASE + \
						CONFIG_ENV_OFFSET)

/* Initial environment variables */
#define CONFIG_EXTRA_ENV_SETTINGS		\
	"hwconfig=fsl_ddr:bank_intlv=auto\0"	\
	"fdt_high=0xffffffffffffffff\0"		\
	"initrd_high=0xffffffffffffffff\0"	\
	"load_addr=0xa0000000\0"		\
	"kernel_start=0x1000000\0"		\
	"kernel_addr_nor=0x21000000\0"		\
	"kernel_size=0x2800000\0"		\
	"console=ttyS0,115200\0"		\
	"qspi_bootcmd=echo Trying load from qspi..;"	\
		"sf probe && sf read $load_addr "	\
		"$kernel_start $kernel_size; bootm $load_addr\0"

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND "run qspi_bootcmd"
#endif

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE		512	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE /* Boot args buffer */

#ifndef SPL_NO_MISC
#ifndef CONFIG_CMDLINE_EDITING
#define CONFIG_CMDLINE_EDITING		1
#endif
#endif

#define CONFIG_SYS_MAXARGS		64	/* max command args */

#define CONFIG_SYS_BOOTM_LEN   (64 << 20)      /* Increase max gunzip size */

#endif /* __L1028A_COMMON_H */
