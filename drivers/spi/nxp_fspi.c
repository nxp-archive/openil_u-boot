// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018 NXP
 *
 * NXP Flex Serial Peripheral Interface (FSPI) driver
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/io.h>
#include <linux/sizes.h>
#include <linux/mtd/spi-nor.h>
#include <dm.h>
#include <errno.h>
#include "nxp_fspi.h"

DECLARE_GLOBAL_DATA_PTR;

/* default SCK frequency, unit: HZ */
#define NXP_FSPI_DEFAULT_SCK_FREQ	50000000

/* FSPI max chipselect signals number */
#define NXP_FSPI_MAX_CHIPSELECT_NUM     4

#define NXP_FSPI_MAX_TIMEOUT_AHBCMD	0xFF
#define NXP_FSPI_MAX_TIMEOUT_IPCMD	0xFF
#define NXP_FSPI_SER_CLK_DIV		0x00

#define FLASH_TYPE_NOR			0
#define FLASH_TYPE_NAND			1

#define FSPI_FLAG_REGMAP_ENDIAN_BIG	BIT(0)

#define FSPI_RX_MAX_IPBUF_SIZE		0x200 /* 64 * 64bits  */
#define FSPI_TX_MAX_IPBUF_SIZE		0x400 /* 128 * 64bits */
#define FSPI_RX_MAX_AHBBUF_SIZE		0x800 /* 256 * 64bits */
#define FSPI_TX_MAX_AHBBUF_SIZE		0x40  /* 8 * 64bits   */

#define TX_IPBUF_SIZE		FSPI_TX_MAX_IPBUF_SIZE
#define RX_IPBUF_SIZE		FSPI_RX_MAX_IPBUF_SIZE
#define RX_AHBBUF_SIZE		FSPI_RX_MAX_AHBBUF_SIZE
#define TX_AHBBUF_SIZE		FSPI_TX_MAX_AHBBUF_SIZE

/* SEQID */
#define SEQID_READ		0
#define SEQID_WREN		1
#define SEQID_PP		2
#define SEQID_FAST_READ		3
#define SEQID_RDSR		4
#define SEQID_SE		5
#define SEQID_CHIP_ERASE	6
#define SEQID_RDID		7
#define SEQID_BE_4K		8
#define SEQID_RDFSR		9
#define SEQID_ENTR4BYTE		10
#define SEQID_OCTAL_READ	19
#ifdef CONFIG_SPI_NAND_FLASH
#define SEQID_RDID_NAND		11
#define SEQID_GET_FEATURE_NAND	12
#define SEQID_PAGE_READ		13
#define SEQID_CACHE_READ	14
#define SEQID_BLOCK_ERASE	15
#define SEQID_PROGRAM_LOAD	16
#define SEQID_PROGRAM_EXECUTE	17
#define SEQID_SET_FEATURE_NAND	18
#endif

#ifdef CONFIG_SPI_NAND_FLASH
#define FSPI_CMD_GET_FEATURE	0x0f	/* Get features command for NAND */
#define FSPI_CMD_PAGE_READ	0x13	/* Read a page to NAND cache */
#define FSPI_CMD_CACHE_READ	0x03	/* Read data from NAND cache */
#define FSPI_CMD_BLOCK_ERASE	0xd8	/* Erase a block on NAND flash */
#define FSPI_CMD_PRGRM_LOADx1	0x02	/* Load data to cache */
#define FSPI_CMD_PRGRM_EXEC	0x10	/* Write data from cache to NAND */
#define FSPI_CMD_SET_FEATURE	0x1f	/* Set NAND flash configuration */
#endif

/* Number of SPI lines supported for Rx and Tx lines */
#define FSPI_SINGLE_MODE	0x1
#define FSPI_OCTAL_MODE		0x8
#define FSPI_CMD_OCTAL_READ_4B	0x7c	/* Octal Read (CMD-ADDR-DATA : 1-1-8) */
#define NXP_FSPI_DEFAULT_SPI_RX_BUS_WIDTH	FSPI_SINGLE_MODE
#define NXP_FSPI_DEFAULT_SPI_TX_BUS_WIDTH	FSPI_SINGLE_MODE

#ifdef CONFIG_DM_SPI

/* AHB initialization parameters for FLSHXNCR2 registers */
#define AHBWR_ADDITIONAL_LUT	0x1	/* Number of additional LUTs to be
					 * fired along with AWRSEQID LUT
					 */

/**
 * struct nxp_fspi_platdata - platform data for NXP FSPI
 *
 * @flags: Flags for FSPI FSPI_FLAG_...
 * @speed_hz: Default SCK frequency
 * @reg_base: Base address of FSPI registers
 * @amba_base: Base address of FSPI memory mapping
 * @amba_total_size: size of FSPI memory mapping
 * @memmap_phy: Physical base address of FSPI memory mapping
 * @flash_num: Number of active slave devices
 * @num_chipselect: Number of FSPI chipselect signals
 */
struct nxp_fspi_platdata {
	u32 flags; /*future use*/
	u32 speed_hz;
	fdt_addr_t reg_base;
	fdt_addr_t amba_base;
	fdt_size_t amba_total_size;
	fdt_size_t memmap_phy;
	u32 flash_num;
	u32 num_chipselect;
	u32 flash_type[4];
	u32 fspi_rx_bus_width;
	u32 fspi_tx_bus_width;
};

/**
 * struct nxp_fspi_priv - private data for NXP FSPI
 *
 * @flags: Flags for FSPI FSPI_FLAG_...
 * @bus_clk: FSPI input clk frequency
 * @speed_hz: Default SCK frequency
 * @cur_seqid: current LUT table sequence id
 * @sf_addr: flash access offset
 * @amba_base: Base address of FSPI memory mapping of every CS
 * @amba_total_size: size of FSPI memory mapping
 * @cur_amba_base: Base address of FSPI memory mapping of current CS
 * @flash_num: Number of active slave devices
 * @num_chipselect: Number of FSPI chipselect signals
 * @memmap_phy: Physical base address of FSPI memory mapping
 * @regs: Point to FSPI register structure for I/O access
 */
struct nxp_fspi_priv {
	u32 flags; /*future use*/
	u32 bus_clk;
	u32 speed_hz;
	u32 cur_seqid;
	u32 sf_addr;
	u32 amba_base[NXP_FSPI_MAX_CHIPSELECT_NUM];
	u32 amba_total_size;
	u32 cur_amba_base;
	u32 flash_num;
	u32 num_chipselect;
	u32 memmap_phy;
	u32 current_cs;
	u32 flash_type[4];
	u32 fspi_rx_bus_width;
	u32 fspi_tx_bus_width;
	struct nxp_fspi_regs *regs;
};

static u32 fspi_read32(u32 flags, u32 *addr)
{
	return flags & FSPI_FLAG_REGMAP_ENDIAN_BIG ?
		in_be32(addr) : in_le32(addr);
}

static void fspi_write32(u32 flags, u32 *addr, u32 val)
{
	flags & FSPI_FLAG_REGMAP_ENDIAN_BIG ?
		out_be32(addr, val) : out_le32(addr, val);
}

void fspi_lut_lock(struct nxp_fspi_priv *priv, u8 lock)
{
	struct nxp_fspi_regs *regs = priv->regs;

	fspi_write32(priv->flags, &regs->lutkey, FSPI_LUTKEY_VALUE);

	if (lock)
		fspi_write32(priv->flags, &regs->lutcr, FSPI_LUTCR_LOCK);
	else
		fspi_write32(priv->flags, &regs->lutcr, FSPI_LUTCR_UNLOCK);
}

static void fspi_set_lut(struct nxp_fspi_priv *priv)
{
	struct nxp_fspi_regs *regs = priv->regs;
	u32 lut_base;

	/* Unlock the LUT */
	fspi_lut_lock(priv, 0);

	/* READ */
	lut_base = SEQID_READ * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINOR_OP_READ) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(0) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Write Enable */
	lut_base = SEQID_WREN * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(SPINOR_OP_WREN) |
		PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Page Program */
	lut_base = SEQID_PP * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINOR_OP_PP_4B) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR32BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(0) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Fast Read */
	lut_base = SEQID_FAST_READ * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINOR_OP_READ_FAST_4B) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) |
		     OPRND1(ADDR32BIT) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(8) | PAD0(LUT_PAD1) | INSTR0(LUT_DUMMY) |
		     OPRND1(0) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read Status */
	lut_base = SEQID_RDSR * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(SPINOR_OP_RDSR) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Erase a sector */
	lut_base = SEQID_SE * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINOR_OP_SE_4B) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR32BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Erase the whole chip */
	lut_base = SEQID_CHIP_ERASE * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINOR_OP_CHIP_ERASE) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* READ ID */
	lut_base = SEQID_RDID * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(SPINOR_OP_RDID) |
		PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(8) |
		PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* SUB SECTOR 4K ERASE */
	lut_base = SEQID_BE_4K * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(SPINOR_OP_BE_4K) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));

	/* Read Flag Status */
	lut_base = SEQID_RDFSR * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(SPINOR_OP_RDFSR) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Enter 4-Byte Address Mode */
	lut_base = SEQID_ENTR4BYTE * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINOR_OP_EN4B) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

#ifdef CONFIG_SPI_NAND_FLASH
	/* Read ID for NAND */
	lut_base = SEQID_RDID_NAND * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINOR_OP_RDID) | PAD0(LUT_PAD1) | INSTR0(LUT_CMD) |
		     OPRND1(8) | PAD1(LUT_PAD1) | INSTR1(LUT_DUMMY));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(1) | PAD0(LUT_PAD1) | INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Get feature */
	lut_base = SEQID_GET_FEATURE_NAND * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_GET_FEATURE) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR8BIT) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(8) | PAD0(LUT_PAD1) | INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Page read */
	lut_base = SEQID_PAGE_READ * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_PAGE_READ) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read from cache */
	lut_base = SEQID_CACHE_READ * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_CACHE_READ) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR16BIT) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(8) | PAD0(LUT_PAD1) | INSTR0(LUT_DUMMY) |
		     OPRND1(0) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Block Erase */
	lut_base = SEQID_BLOCK_ERASE * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_BLOCK_ERASE) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Program Load */
	lut_base = SEQID_PROGRAM_LOAD * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_PRGRM_LOADx1) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR16BIT) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(0x10) | PAD0(LUT_PAD1) | INSTR0(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Program Execute */
	lut_base = SEQID_PROGRAM_EXECUTE * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_PRGRM_EXEC) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Set feature */
	lut_base = SEQID_SET_FEATURE_NAND * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_SET_FEATURE) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR8BIT) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(1) | PAD0(LUT_PAD1) | INSTR0(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);
#endif

	/* 8-Bit Read */
	lut_base = SEQID_OCTAL_READ * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_OCTAL_READ_4B) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) |
		     OPRND1(ADDR32BIT) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_ADDR));

	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(8) | PAD0(LUT_PAD8) | INSTR0(LUT_DUMMY) |
		     OPRND1(0) | PAD1(LUT_PAD8) |
		     INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Lock the LUT */
	fspi_lut_lock(priv, 1);
}

void fspi_module_disable(struct nxp_fspi_priv *priv, u8 disable)
{
	u32 mcr0;

	mcr0 = fspi_read32(priv->flags, &priv->regs->mcr0);
	if (disable)
		mcr0 |= FSPI_MCR0_MDIS_MASK;
	else
		mcr0 &= ~FSPI_MCR0_MDIS_MASK;
	fspi_write32(priv->flags, &priv->regs->mcr0, mcr0);
}

static void fspi_op_write_cmd(struct nxp_fspi_priv *priv)
{
	struct nxp_fspi_regs *regs = priv->regs;
	u32 seqid;

	/*
	 * Increase write cmds list as required.
	 * For now below write cmds support is there:
	 * 'Write Enable' and 'Enter 4 Byte'
	 */
	if (priv->cur_seqid == SPINOR_OP_WREN)
		seqid = SEQID_WREN;
	else if (priv->cur_seqid == SPINOR_OP_EN4B)
		seqid = SEQID_ENTR4BYTE;
	else
		return;

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FSPI_IPTXFCR_CLRIPTXF_MASK);

	/* set address value in IPCR0 register. */
	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	/*Push SEQID in IPCR1 */
	fspi_write32(priv->flags, &regs->ipcr1,
		     (seqid << FSPI_IPCR1_ISEQID_SHIFT) | 0);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FSPI_INTR_IPCMDDONE_MASK))
		;

	fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPCMDDONE_MASK);
}

#if defined(CONFIG_SYS_NXP_FSPI_AHB)

/*
 * If we have changed the content of the flash by writing or erasing,
 * we need to invalidate the AHB buffer. If we do not do so, we may read out
 * the wrong data. The spec tells us to reset the AHB domain and Serial Flash
 * domain at the same time.
 */
static inline void fspi_ahb_invalid(struct nxp_fspi_priv *priv)
{
	struct nxp_fspi_regs *regs = priv->regs;
	u32 reg;

	reg = fspi_read32(priv->flags, &regs->mcr0);
	reg |= FSPI_MCR0_SWRESET_MASK;
	fspi_write32(priv->flags, &regs->mcr0, reg);

	/*
	 * The minimum delay : 1 AHB + 2 SFCK clocks.
	 * Delay 1 us is enough.
	 * udelay(1); // TODO check delay is required or not.
	 */
	while ((fspi_read32(priv->flags, &regs->mcr0) & 1))
		;
}

/* Write data to AHB Buffer. */
static inline void fspi_ahb_write(struct nxp_fspi_priv *priv, void *txbuf, u32 len)
{
	u32 tx_size;
	void *tx_addr;

	tx_addr = (void *)(uintptr_t)(priv->memmap_phy +
			priv->cur_amba_base +
			priv->sf_addr);
	debug("FSPI AHB Write Invoked from:[0x%02x len:%d]\n",
			(priv->memmap_phy + priv->cur_amba_base + priv->sf_addr), len);

	while (len > 0) {
		tx_size = (len > 8) ? 8 : len;

		memcpy(tx_addr, txbuf, tx_size);

		tx_addr = (void *)((char *)tx_addr + tx_size);
		txbuf = (void *)((char *)txbuf + tx_size);
		len -= tx_size;
		udelay(105);
	}
}

/* Read out the data from the AHB buffer. */
static inline void fspi_ahb_read(struct nxp_fspi_priv *priv, u8 *rxbuf, int len)
{
	void *rx_addr = NULL;

	debug("FSPI AHB Read Invoked\n");
	rx_addr = (void *)(uintptr_t)(priv->memmap_phy +
				      priv->cur_amba_base +
				      priv->sf_addr);

	/* Read out the data directly from the AHB buffer. */
	memcpy(rxbuf, rx_addr, len);
}

/*
 * There are two different ways to read out the data from the flash:
 * the "IP Command Read" and the "AHB Command Read".
 * "AHB Command Read" is faster then the "IP Command Read".
 *
 * After we set up the registers for the "AHB Command Read", we can use
 * the memcpy to read the data directly.
 */
static void fspi_init_ahb(struct nxp_fspi_priv *priv)
{
	struct nxp_fspi_regs *regs = priv->regs;
	int i;

	/* AHB configuration for access buffer 0~7. */
	for (i = 0; i < 7; i++)
		fspi_write32(priv->flags, &regs->ahbrxbuf0cr0 + i, 0);

	/*
	 * Set ADATSZ with the maximum AHB buffer size to improve the read
	 * performance
	 */
	fspi_write32(priv->flags, &regs->ahbrxbuf7cr0,
		     FSPI_RX_MAX_AHBBUF_SIZE / 8 |
			FSPI_AHBBUFXCR0_PREFETCHEN_MASK);

	fspi_write32(priv->flags, &regs->ahbcr, FSPI_AHBCR_PREFETCHEN_MASK);

	/*
	 * Set default lut sequence for AHB Read, bit[4-0] in flsha1cr2 reg.
	 * Parallel mode is disabled.
	 */
	if (priv->fspi_rx_bus_width == FSPI_OCTAL_MODE) {
		/* Flash supports octal read */
		fspi_write32(priv->flags, &regs->flsha1cr2, SEQID_OCTAL_READ <<
			FSPI_FLSHXCR2_ARDSEQID_SHIFT | SEQID_WREN <<
			FSPI_FLSHXCR2_AWRSEQID_SHIFT |
			AHBWR_ADDITIONAL_LUT <<
			FSPI_FLSHXCR2_AWRSEQNUM_SHIFT);
	} else {
		fspi_write32(priv->flags, &regs->flsha1cr2, SEQID_FAST_READ <<
			FSPI_FLSHXCR2_ARDSEQID_SHIFT | SEQID_WREN <<
			FSPI_FLSHXCR2_AWRSEQID_SHIFT |
			AHBWR_ADDITIONAL_LUT <<
			FSPI_FLSHXCR2_AWRSEQNUM_SHIFT);
	}
}
#endif

static void fspi_op_rdxx(struct nxp_fspi_priv *priv, u32 *rxbuf, u32 len)
{
	struct nxp_fspi_regs *regs = priv->regs;
	u32 iprxfcr = 0;
	u32 data, size;
	int i;

	iprxfcr = fspi_read32(priv->flags, &regs->iprxfcr);
	/* IP RX FIFO would be read by processor */
	iprxfcr &= ~FSPI_IPRXFCR_RXDMAEN_MASK;
	/* Invalid data entries in IP RX FIFO */
	iprxfcr = iprxfcr | FSPI_IPRXFCR_CLRIPRXF_MASK;
	fspi_write32(priv->flags, &regs->iprxfcr, iprxfcr);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	if (priv->cur_seqid == SPINOR_OP_RDID) {
#ifdef CONFIG_SPI_NAND_FLASH
		if (priv->flash_type[priv->current_cs] == FLASH_TYPE_NAND)
			fspi_write32(priv->flags, &regs->ipcr1,
				     (SEQID_RDID_NAND << FSPI_IPCR1_ISEQID_SHIFT) |
				     (u16)len);
		else
#endif
			fspi_write32(priv->flags, &regs->ipcr1,
				     (SEQID_RDID << FSPI_IPCR1_ISEQID_SHIFT) |
				     (u16)len);
	} else if (priv->cur_seqid == SPINOR_OP_RDSR)
		fspi_write32(priv->flags, &regs->ipcr1,
			     (SEQID_RDSR << FSPI_IPCR1_ISEQID_SHIFT) |
			     (u16)len);
	else if (priv->cur_seqid == SPINOR_OP_RDFSR)
		fspi_write32(priv->flags, &regs->ipcr1,
			     (SEQID_RDFSR << FSPI_IPCR1_ISEQID_SHIFT) |
			     (u16)len);

	/* Trigger the IP command */
	fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FSPI_INTR_IPCMDDONE_MASK))
		;

	i = 0;
	while ((RX_IPBUF_SIZE >= len) && (len > 0)) {
		data = fspi_read32(priv->flags, &regs->rfdr[i]);
		size = (len < 4) ? len : 4;
		memcpy(rxbuf, &data, size);
		len -= size;
		rxbuf++;
		i++;
	}

	/* Rx FIFO invalidation needs to be done prior w1c of INTR.IPRXWA bit */
	fspi_write32(priv->flags, &regs->iprxfcr, FSPI_IPRXFCR_CLRIPRXF_MASK);
	fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPRXWA_MASK);
	fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPCMDDONE_MASK);
}

#ifndef CONFIG_SYS_NXP_FSPI_AHB
/* If AHB read interface not defined, read data from ip interface. */
static void fspi_op_read(struct nxp_fspi_priv *priv, u32 *rxbuf, u32 len)
{
	if (priv->flash_type[priv->current_cs] == FLASH_TYPE_NAND)
		return;

	struct nxp_fspi_regs *regs = priv->regs;
	int i, j;
	int size, rx_size, wm_size, temp_size;
	u32 to_or_from, data = 0;

	to_or_from = priv->sf_addr + priv->cur_amba_base;

	debug("Read [to_or_frm:0x%02x]\n", to_or_from);
	/* invalid the RXFIFO */
	fspi_write32(priv->flags, &regs->iprxfcr, FSPI_IPRXFCR_CLRIPRXF_MASK);

	while (len > 0) {
		fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

		rx_size = (len > RX_IPBUF_SIZE) ? RX_IPBUF_SIZE : len;

		if (priv->fspi_rx_bus_width == FSPI_OCTAL_MODE) {
			/* Flash supports octal read */
			fspi_write32(priv->flags, &regs->ipcr1,
				     (SEQID_OCTAL_READ <<
				      FSPI_IPCR1_ISEQID_SHIFT) | (u16)rx_size);
		} else {
			/* Flash supports single bit read */
			fspi_write32(priv->flags, &regs->ipcr1,
				     (SEQID_FAST_READ <<
				      FSPI_IPCR1_ISEQID_SHIFT) | (u16)rx_size);
		}

		to_or_from += rx_size;
		len -= rx_size;

		/* Trigger the command */
		fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

		/* Default value of water mark level is 8 bytes. */
		wm_size = 8;

		size = rx_size / wm_size;
		for (i = 0; i < size; ++i) {
			/* Wait for RXFIFO available*/
			while (!(fspi_read32(priv->flags, &regs->intr)
				 & FSPI_INTR_IPRXWA_MASK))
				;

			temp_size = wm_size;
			j = 0;
			while (temp_size > 0) {
				data = 0;
				data = fspi_read32(priv->flags,
						   &regs->rfdr[j++]);
				temp_size = wm_size - 4 * j;
				memcpy(rxbuf, &data, 4);
				rxbuf++;
			}

			/* move the FIFO pointer */
			fspi_write32(priv->flags, &regs->intr,
				     FSPI_INTR_IPRXWA_MASK);
		}

		size = rx_size % 8;

		if (size) {
			/* Wait for data filled*/
			while (!(fspi_read32(priv->flags, &regs->iprxfsts)
				 & FSPI_IPRXFSTS_FILL_MASK))
				;

			temp_size = 0;
			data = 0;
			j = 0;
			while (size > 0) {
				data = 0;
				data = fspi_read32(priv->flags,
						   &regs->rfdr[j++]);
				temp_size = (size < 4) ? size : 4;
				memcpy(rxbuf, &data, temp_size);
				size -= temp_size;
				rxbuf++;
			}
		}

		/* invalid the RXFIFO */
		fspi_write32(priv->flags, &regs->iprxfcr,
			     FSPI_IPRXFCR_CLRIPRXF_MASK);
		/* move the FIFO pointer */
		fspi_write32(priv->flags, &regs->intr,
			     FSPI_INTR_IPRXWA_MASK);
		fspi_write32(priv->flags, &regs->intr,
			     FSPI_INTR_IPCMDDONE_MASK);
	}
}
#endif

static void fspi_op_erase(struct nxp_fspi_priv *priv)
{
	if (priv->flash_type[priv->current_cs] == FLASH_TYPE_NAND)
		return;

	struct nxp_fspi_regs *regs = priv->regs;
	u32 to_or_from = 0;

	to_or_from = priv->sf_addr + priv->cur_amba_base;
	fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

	/*TODO WREN should be diffrent cmd*/
	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_WREN << FSPI_IPCR1_ISEQID_SHIFT) | 0);
	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FSPI_INTR_IPCMDDONE_MASK))
		;

	fspi_write32(priv->flags, &regs->intr, FSPI_INTEN_IPCMDDONEEN_MASK);

	if (priv->cur_seqid == SPINOR_OP_BE_4K) {
		fspi_write32(priv->flags, &regs->ipcr1,
			     (SEQID_BE_4K << FSPI_IPCR1_ISEQID_SHIFT) | 0);
	} else if (priv->cur_seqid == SPINOR_OP_SE_4B) {
		fspi_write32(priv->flags, &regs->ipcr1,
			     (SEQID_SE << FSPI_IPCR1_ISEQID_SHIFT) | 0);
	}
	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FSPI_INTR_IPCMDDONE_MASK))
		;

	fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPCMDDONE_MASK);
}

#ifndef CONFIG_SYS_NXP_FSPI_AHB
static void fspi_op_write(struct nxp_fspi_priv *priv, u8 *txbuf, u32 len)
{
	if (priv->flash_type[priv->current_cs] == FLASH_TYPE_NAND)
		return;

	struct nxp_fspi_regs *regs = priv->regs;
	u32 seqid;
	int i, j;
	int size = 0, tx_size = 0, wm_size = 0, temp_size = 0;
	u32 to_or_from = 0, data = 0;

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FSPI_IPTXFCR_CLRIPTXF_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);
	/*TODO: move WREN to new cmd*/
	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_WREN << FSPI_IPCR1_ISEQID_SHIFT) | 0);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FSPI_INTR_IPCMDDONE_MASK))
		;

	fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPCMDDONE_MASK);

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FSPI_IPTXFCR_CLRIPTXF_MASK);

	to_or_from = priv->sf_addr + priv->cur_amba_base;

	debug("Write [to_or_frm:0x%02x]\n", to_or_from);
	while (len > 0) {
		/* Default is page programming */
		seqid = SEQID_PP;
		fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

		tx_size = (len > TX_IPBUF_SIZE) ? TX_IPBUF_SIZE : len;

		to_or_from += tx_size;
		len -= tx_size;

		/* Default value of water mark level is 8 bytes. */
		wm_size = 8;

		size = tx_size / wm_size;
		for (i = 0; i < size; i++) {
			/* Wait for TXFIFO empty*/
			while (!(fspi_read32(priv->flags, &regs->intr)
				 & FSPI_INTR_IPTXWE_MASK))
				;

			temp_size = wm_size;
			j = 0;
			while (temp_size > 0) {
				data = 0;
				memcpy(&data, txbuf, 4);
				fspi_write32(priv->flags, &regs->tfdr[j++],
					     data);
				temp_size = wm_size - 4 * j;
				txbuf += 4;
			}

			fspi_write32(priv->flags, &regs->intr,
				     FSPI_INTR_IPTXWE_MASK);
		}

		size = tx_size % 8;
		if (size) {
			/* Wait for TXFIFO empty*/
			while (!(fspi_read32(priv->flags, &regs->intr)
				 & FSPI_INTR_IPTXWE_MASK))
				;

			temp_size = 0;
			j = 0;
			while (size > 0) {
				data = 0;
				temp_size = (size < 4) ? size : 4;
				memcpy(&data, txbuf, temp_size);
				fspi_write32(priv->flags, &regs->tfdr[j++],
					     data);
				size -= temp_size;
				txbuf += temp_size;
			}
			fspi_write32(priv->flags, &regs->intr,
				     FSPI_INTR_IPTXWE_MASK);
		}

		fspi_write32(priv->flags, &regs->ipcr1,
			     (seqid << FSPI_IPCR1_ISEQID_SHIFT) | tx_size);

		/* Trigger the command */
		fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

		/* Wait for command done */
		while (!(fspi_read32(priv->flags, &regs->intr)
			 & FSPI_INTR_IPCMDDONE_MASK))
			;

		fspi_write32(priv->flags, &regs->iptxfcr,
			     FSPI_IPTXFCR_CLRIPTXF_MASK);
		fspi_write32(priv->flags, &regs->intr,
			     FSPI_INTR_IPCMDDONE_MASK);
	}
}
#endif

#ifdef CONFIG_SPI_NAND_FLASH
static void fspi_get_feature(struct nxp_fspi_priv *priv, u32 *rxbuf)
{
	if (priv->flash_type[priv->current_cs] == FLASH_TYPE_NOR)
		return;

	struct nxp_fspi_regs *regs = priv->regs;
	int i = 0;
	u32 data, to_or_from;

	debug("fspi_get_feature called\n");
	to_or_from = priv->sf_addr + priv->cur_amba_base;
	/* invalid the RXFIFO */
	fspi_write32(priv->flags, &regs->iprxfcr, FSPI_IPRXFCR_CLRIPRXF_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_GET_FEATURE_NAND << FSPI_IPCR1_ISEQID_SHIFT) | 1);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FSPI_INTR_IPCMDDONE_MASK))
		;

		data = fspi_read32(priv->flags, &regs->rfdr[i]);
		memcpy(rxbuf, &data, 1);
	/* invalid the RXFIFO */
	fspi_write32(priv->flags, &regs->iprxfcr, FSPI_IPRXFCR_CLRIPRXF_MASK);
	/* move the FIFO pointer */
	fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPRXWA_MASK);
	fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPCMDDONE_MASK);
}

static void fspi_set_feature(struct nxp_fspi_priv *priv, u8 *txbuf)
{
	if (priv->flash_type[priv->current_cs] == FLASH_TYPE_NOR)
		return;

	struct nxp_fspi_regs *regs = priv->regs;
	int i = 0;
	u8 data;
	u32 to_or_from;

	to_or_from = priv->sf_addr + priv->cur_amba_base;
	debug("Set feature [register address:0x%x]\n", to_or_from);
	debug("Set feature [value:0x%x]\n", *txbuf);

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FSPI_IPTXFCR_CLRIPTXF_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

	memcpy(&data, txbuf, 1);
	fspi_write32(priv->flags, &regs->tfdr[i++], data);
	fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPTXWE_MASK);
	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_SET_FEATURE_NAND << FSPI_IPCR1_ISEQID_SHIFT) | 1);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr) &
		 FSPI_INTR_IPCMDDONE_MASK));

	fspi_write32(priv->flags, &regs->iptxfcr, FSPI_IPTXFCR_CLRIPTXF_MASK);
	fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPCMDDONE_MASK);
}

static void fspi_nand_read(struct nxp_fspi_priv *priv, u32 *rxbuf, u32 len)
{
	if (priv->flash_type[priv->current_cs] == FLASH_TYPE_NOR)
		return;

	struct nxp_fspi_regs *regs = priv->regs;
	int i, j;
	int size, rx_size, wm_size, temp_size;
	u32 to_or_from, data = 0;

	to_or_from = priv->sf_addr + priv->cur_amba_base;

	/* invalid the RXFIFO */
	fspi_write32(priv->flags, &regs->iprxfcr, FSPI_IPRXFCR_CLRIPRXF_MASK);

	if (priv->cur_seqid == FSPI_CMD_PAGE_READ) {
		fspi_write32(priv->flags, &regs->ipcr0, to_or_from);
		debug("fspi_nand_read: page_read: to_or_from: %x\n", to_or_from);
		fspi_write32(priv->flags, &regs->ipcr1,
			     (SEQID_PAGE_READ << FSPI_IPCR1_ISEQID_SHIFT) | 0);

		/* Trigger the command */
		fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

		while (!(fspi_read32(priv->flags, &regs->intr)
			 & FSPI_INTR_IPCMDDONE_MASK))
			;

		fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPCMDDONE_MASK);
	} else {
		while (len > 0) {
			fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

			debug("fspi_nand_read: cache read: to_or_from: %x\n", to_or_from);
			rx_size = (len > RX_IPBUF_SIZE) ? RX_IPBUF_SIZE : len;

			fspi_write32(priv->flags, &regs->ipcr1,
				     (SEQID_CACHE_READ <<
				      FSPI_IPCR1_ISEQID_SHIFT) | (u16)rx_size);

			to_or_from += rx_size;
			len -= rx_size;

			/* Trigger the command */
			fspi_write32(priv->flags, &regs->ipcmd,
				     FSPI_IPCMD_TRG_MASK);

			/* Default value of water mark level is 8 bytes. */
			wm_size = 8;

			size = rx_size / wm_size;
			for (i = 0; i < size; ++i) {
				/* Wait for RXFIFO available*/
				while (!(fspi_read32(priv->flags, &regs->intr)
					 & FSPI_INTR_IPRXWA_MASK))
					;

				temp_size = wm_size;
				j = 0;
				while (temp_size > 0) {
					data = 0;
					data = fspi_read32(priv->flags,
							   &regs->rfdr[j++]);
					temp_size = wm_size - 4 * j;
					memcpy(rxbuf, &data, 4);
					rxbuf++;
				}

				/* move the FIFO pointer */
				fspi_write32(priv->flags, &regs->intr,
					     FSPI_INTR_IPRXWA_MASK);
			}

			size = rx_size % 8;

			if (size) {
				/* Wait for data filled*/
				while (!(fspi_read32(priv->flags,
						     &regs->iprxfsts)
					 & FSPI_IPRXFSTS_FILL_MASK))
					;

				temp_size = 0;
				data = 0;
				j = 0;
				while (size > 0) {
					data = 0;
					data = fspi_read32(priv->flags,
							   &regs->rfdr[j++]);
					temp_size = (size < 4) ? size : 4;
					memcpy(rxbuf, &data, temp_size);
					size -= temp_size;
					rxbuf++;
				}
			}

			/* invalid the RXFIFO */
			fspi_write32(priv->flags, &regs->iprxfcr,
				     FSPI_IPRXFCR_CLRIPRXF_MASK);
			/* move the FIFO pointer */
			fspi_write32(priv->flags, &regs->intr,
				     FSPI_INTR_IPRXWA_MASK);
			fspi_write32(priv->flags, &regs->intr,
				     FSPI_INTR_IPCMDDONE_MASK);
		}
	}
}

static void fspi_nand_erase(struct nxp_fspi_priv *priv)
{
	if (priv->flash_type[priv->current_cs] == FLASH_TYPE_NOR)
		return;

	struct nxp_fspi_regs *regs = priv->regs;
	u32 to_or_from = 0;

	to_or_from = priv->sf_addr + priv->cur_amba_base;
	fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

	fspi_write32(priv->flags, &regs->ipcr1, (SEQID_BLOCK_ERASE <<
						 FSPI_IPCR1_ISEQID_SHIFT) | 0);
	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FSPI_INTR_IPCMDDONE_MASK))
		;

	fspi_write32(priv->flags, &regs->intr, FSPI_INTR_IPCMDDONE_MASK);
}

static void fspi_nand_write(struct nxp_fspi_priv *priv, u8 *txbuf, u32 len)
{
	if (priv->flash_type[priv->current_cs] == FLASH_TYPE_NOR)
		return;

	struct nxp_fspi_regs *regs = priv->regs;
	int i, j;
	int size = 0, tx_size = 0, wm_size = 0, temp_size = 0;
	u32 to_or_from = 0, data = 0;

	if (priv->cur_seqid == FSPI_CMD_PRGRM_EXEC) {
		to_or_from = priv->sf_addr + priv->cur_amba_base;
		debug("Write: Prog Exec: [to_or_frm:0x%02x]\n", to_or_from);

		fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

		fspi_write32(priv->flags, &regs->ipcr1,
			     (SEQID_PROGRAM_EXECUTE << FSPI_IPCR1_ISEQID_SHIFT) | 0);

		/* Trigger the command */
		fspi_write32(priv->flags, &regs->ipcmd, FSPI_IPCMD_TRG_MASK);

		/* Wait for command done */
		while (!(fspi_read32(priv->flags, &regs->intr)
			 & FSPI_INTR_IPCMDDONE_MASK))
			;
		fspi_write32(priv->flags, &regs->iptxfcr,
			     FSPI_IPTXFCR_CLRIPTXF_MASK);
		fspi_write32(priv->flags, &regs->intr,
			     FSPI_INTR_IPCMDDONE_MASK);
	} else {
		to_or_from = priv->sf_addr + priv->cur_amba_base;
		debug("Write: Prog Load: [to_or_frm:0x%02x]\n", to_or_from);

		/* invalid the TXFIFO first */
		fspi_write32(priv->flags, &regs->iptxfcr, FSPI_IPTXFCR_CLRIPTXF_MASK);

		while (len > 0) {
			fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

			tx_size = (len > TX_IPBUF_SIZE) ? TX_IPBUF_SIZE : len;

			to_or_from += tx_size;
			len -= tx_size;

			/* Default value of water mark level is 8 bytes. */
			wm_size = 8;

			size = tx_size / wm_size;
			for (i = 0; i < size; i++) {
				/* Wait for TXFIFO empty*/
				while (!(fspi_read32(priv->flags, &regs->intr)
					 & FSPI_INTR_IPTXWE_MASK))
					;

				temp_size = wm_size;
				j = 0;
				while (temp_size > 0) {
					data = 0;
					memcpy(&data, txbuf, 4);
					fspi_write32(priv->flags,
						     &regs->tfdr[j++], data);
					temp_size = wm_size - 4 * j;
					txbuf += 4;
				}

				fspi_write32(priv->flags, &regs->intr,
					     FSPI_INTR_IPTXWE_MASK);
			}

			size = tx_size % 8;
			if (size) {
				/* Wait for TXFIFO empty*/
				while (!(fspi_read32(priv->flags, &regs->intr)
					 & FSPI_INTR_IPTXWE_MASK))
					;

				temp_size = 0;
				j = 0;
				while (size > 0) {
					data = 0;
					temp_size = (size < 4) ? size : 4;
					memcpy(&data, txbuf, temp_size);
					fspi_write32(priv->flags,
						     &regs->tfdr[j++], data);
					size -= temp_size;
					txbuf += temp_size;
				}
				fspi_write32(priv->flags, &regs->intr,
					     FSPI_INTR_IPTXWE_MASK);
			}

			fspi_write32(priv->flags, &regs->ipcr1,
				     (SEQID_PROGRAM_LOAD << FSPI_IPCR1_ISEQID_SHIFT)
				     | tx_size);

			/* Trigger the command */
			fspi_write32(priv->flags, &regs->ipcmd,
				     FSPI_IPCMD_TRG_MASK);

			/* Wait for command done */
			while (!(fspi_read32(priv->flags, &regs->intr)
				 & FSPI_INTR_IPCMDDONE_MASK))
				;

			fspi_write32(priv->flags, &regs->iptxfcr,
				     FSPI_IPTXFCR_CLRIPTXF_MASK);
			fspi_write32(priv->flags, &regs->intr,
				     FSPI_INTR_IPCMDDONE_MASK);
		}

	}
}
#endif

static int fspi_xfer(struct nxp_fspi_priv *priv, unsigned int bitlen,
		     const void *dout, void *din, unsigned long flags)
{
	u32 bytes = DIV_ROUND_UP(bitlen, 8);
	u32 txbuf;
	u8  *tempbuf;

	if (dout) {
		if (flags & SPI_XFER_BEGIN) {
			priv->cur_seqid = *(u8 *)dout;
			/*
			 * For 4-byte support address, received bytes are 5.
			 * SEQID in 1st byte and then address in rest 4 bytes.
			 */
			if ((NXP_FSPI_FLASH_SIZE > SZ_16M) && (bytes > 4)) {
				memcpy(&txbuf, dout+1, 4);
				priv->sf_addr = swab32(txbuf) & GENMASK(27, 0);
			} else if (bytes == 4) {
				memcpy(&txbuf, dout, 4);
				priv->sf_addr = swab32(txbuf) & GENMASK(23, 0);
			} else if (bytes == 3) {
				memcpy(&txbuf, dout, 4);
				priv->sf_addr = (swab32(txbuf) >> 8) &
					GENMASK(15, 0);
			} else if (bytes == 2) {
				memcpy(&txbuf, dout, 2);
				priv->sf_addr = swab16(txbuf) & GENMASK(7, 0);
			}

			tempbuf = (u8 *)&txbuf;
			debug("txbuf: 0x%02x  0x%02x  0x%02x  0x%02x\n",
			      tempbuf[0], tempbuf[1], tempbuf[2], tempbuf[3]);
		}

		if (flags == SPI_XFER_END) {
#ifdef CONFIG_SPI_NAND_FLASH
			if (priv->cur_seqid == FSPI_CMD_PRGRM_LOADx1)
				fspi_nand_write(priv, (void *)dout, bytes);
			else if (priv->cur_seqid == FSPI_CMD_SET_FEATURE)
				fspi_set_feature(priv, (void *)dout);
#endif

			/*priv->sf_addr = wr_sfaddr;*/
			debug("sf_addr:[0x%02x]\n", priv->sf_addr);
#ifdef CONFIG_SYS_NXP_FSPI_AHB
			fspi_ahb_write(priv, (void *)dout, bytes);
#else
			fspi_op_write(priv, (u8 *)dout, bytes);
#endif
			return 0;
		}

		if ((priv->cur_seqid == SPINOR_OP_WREN) ||
		    (priv->cur_seqid == SPINOR_OP_EN4B)) {
			debug("FSPI Write [%x] cmd Invoked\n", priv->cur_seqid);
			fspi_op_write_cmd(priv);
			return 0;
		}

		if (priv->cur_seqid == SPINOR_OP_BE_4K ||
		    priv->cur_seqid == SPINOR_OP_SE_4B) {
			debug("FSPI Erase Invoked\n");
			fspi_op_erase(priv);
		}

#ifdef CONFIG_SPI_NAND_FLASH
		if (priv->cur_seqid == FSPI_CMD_PAGE_READ)
			fspi_nand_read(priv, NULL, bytes);

		if (priv->cur_seqid == FSPI_CMD_BLOCK_ERASE)
			fspi_nand_erase(priv);

		if (priv->cur_seqid == FSPI_CMD_PRGRM_EXEC) {
			fspi_nand_write(priv, (void *)dout, bytes);
		}
#endif
		debug("FSPI [%x] Cmd\n", priv->cur_seqid);
	}

	if (din) {
		if ((priv->cur_seqid == SPINOR_OP_RDID) ||
		    (priv->cur_seqid == SPINOR_OP_RDSR) ||
		    (priv->cur_seqid == SPINOR_OP_RDFSR))
			fspi_op_rdxx(priv, din, bytes);
		else if (priv->cur_seqid == SPINOR_OP_READ_FAST ||
			 priv->cur_seqid == SPINOR_OP_READ_FAST_4B)
#ifdef CONFIG_SYS_NXP_FSPI_AHB
			fspi_ahb_read(priv, din, bytes);
#else
			fspi_op_read(priv, din, bytes);
#endif
#ifdef CONFIG_SPI_NAND_FLASH
		else if (priv->cur_seqid == FSPI_CMD_GET_FEATURE)
			fspi_get_feature(priv, din);

		else if (priv->cur_seqid == FSPI_CMD_CACHE_READ) {
			fspi_nand_read(priv, din, bytes);
		}
#endif
		else
			debug("FSPI [%x] Cmd not supported\n",
			      priv->cur_seqid);
	}

#ifdef CONFIG_SYS_NXP_FSPI_AHB
	if ((priv->cur_seqid == SPINOR_OP_PP) ||
	    (priv->cur_seqid == SPINOR_OP_BE_4K))
		fspi_ahb_invalid(priv);
#endif

	return 0;
}

static int nxp_fspi_child_pre_probe(struct udevice *dev)
{
	struct spi_slave *slave = dev_get_parent_priv(dev);

	slave->max_write_size = TX_IPBUF_SIZE;
	debug("FSPI Child Pre Probe set Max write to 0x%x\n",
	      slave->max_write_size);

	return 0;
}

static int nxp_fspi_probe(struct udevice *bus)
{
	struct nxp_fspi_platdata *plat = dev_get_platdata(bus);
	struct nxp_fspi_priv *priv = dev_get_priv(bus);
	struct dm_spi_bus *dm_spi_bus;
	u32 flash_size;
	u32 mcrx;

	dm_spi_bus = bus->uclass_priv;

	dm_spi_bus->max_hz = plat->speed_hz;

	priv->regs = (struct nxp_fspi_regs *)(uintptr_t)plat->reg_base;
	priv->flags = plat->flags;

	priv->speed_hz = plat->speed_hz;
	priv->amba_base[0] = plat->amba_base;
	priv->amba_total_size = plat->amba_total_size;
	priv->memmap_phy = plat->memmap_phy;
	priv->flash_num = plat->flash_num;
	priv->num_chipselect = plat->num_chipselect;
	priv->fspi_rx_bus_width = plat->fspi_rx_bus_width;
	priv->fspi_tx_bus_width = plat->fspi_tx_bus_width;
	for (int i = 0; i < plat->flash_num; i++)
		priv->flash_type[i] = plat->flash_type[i];

	debug("%s: regs=<0x%llx> <0x%llx, 0x%llx>\n",
	      __func__,
	      (u64)priv->regs,
	      (u64)priv->amba_base[0],
	      (u64)priv->amba_total_size);

	debug("max-frequency=%d, flags=0x%x, rx_width=0x%x, tx_width=0x%x\n",
	      priv->speed_hz,
	      plat->flags,
	      priv->fspi_rx_bus_width,
	      priv->fspi_tx_bus_width);

	/*Send Software Reset to controller*/
	fspi_write32(priv->flags, &priv->regs->mcr0,
		     FSPI_MCR0_SWRESET_MASK);
	/*Wait till controller come out of reset*/
	while (FSPI_MCR0_SWRESET_MASK & fspi_read32(priv->flags,
						    &priv->regs->mcr0))
		;

	/* configure controller in stop mode */
	fspi_module_disable(priv, 1);

	mcrx = fspi_read32(priv->flags, &priv->regs->mcr0);
	/*Timeout wait cycle for AHB command grant*/
	mcrx |= (NXP_FSPI_MAX_TIMEOUT_AHBCMD << FSPI_MCR0_AHBGRANTWAIT_SHIFT) &
						(FSPI_MCR0_AHBGRANTWAIT_MASK);
	/*Time out wait cycle for IP command grant*/
	mcrx |= (NXP_FSPI_MAX_TIMEOUT_IPCMD << FSPI_MCR0_IPGRANTWAIT_SHIFT) &
						(FSPI_MCR0_IPGRANTWAIT_MASK);
	/*TODO: for now it is hardcoded val, but we will read this from DT*/
	mcrx |= (NXP_FSPI_SER_CLK_DIV << FSPI_MCR0_SERCLKDIV_SHIFT) &
						(FSPI_MCR0_SERCLKDIV_MASK);
	/* Default set to IP mode */
	mcrx &= ~FSPI_MCR0_ARDFEN_MASK;

	/* Enable the module and set to IP mode. */
	mcrx = 0xFFFF0000;

	fspi_write32(priv->flags, &priv->regs->mcr0, mcrx);

	/* Reset the DLL register to default value */
	fspi_write32(priv->flags, &priv->regs->dllacr, 0x0100);
	fspi_write32(priv->flags, &priv->regs->dllbcr, 0x0100);

	/*
	 * Need to Reset SAMEDEVICEEN bit in mcr2, when we add support for
	 * different flashes.
	 */

	/* Flash Size in KByte */
	flash_size = (NXP_FSPI_FLASH_SIZE * NXP_FSPI_FLASH_NUM) / SZ_1K;


	/*TODO: for now, we get size form macros. Will evolve this and
	 * read size of all flash from device-tree*/
	fspi_write32(priv->flags, &priv->regs->flsha1cr0,
		     flash_size);
	fspi_write32(priv->flags, &priv->regs->flsha2cr0,
		     0);
	fspi_write32(priv->flags, &priv->regs->flshb1cr0,
		     0);
	fspi_write32(priv->flags, &priv->regs->flshb2cr0,
		     0);

	fspi_set_lut(priv);

#ifdef CONFIG_SYS_NXP_FSPI_AHB
	fspi_init_ahb(priv);
#endif

	/*Clear Module Disable mode*/
	fspi_module_disable(priv, 0);

	return 0;
}

static int nxp_fspi_ofdata_to_platdata(struct udevice *bus)
{
	struct fdt_resource res_regs, res_mem;
	struct nxp_fspi_platdata *plat = bus->platdata;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(bus);
	int ret, flash_num = 0, subnode, len;

	if (fdtdec_get_bool(blob, node, "big-endian"))
		plat->flags |= FSPI_FLAG_REGMAP_ENDIAN_BIG;

	ret = fdt_get_named_resource(blob, node, "reg", "reg-names",
				     "FSPI", &res_regs);
	if (ret) {
		debug("Error: can't get regs base addresses(ret = %d)!\n", ret);
		return -ENOMEM;
	}
	ret = fdt_get_named_resource(blob, node, "reg", "reg-names",
				     "FSPI-memory", &res_mem);
	if (ret) {
		debug("Error: can't get AMBA base addresses(ret = %d)!\n", ret);
		return -ENOMEM;
	}

	/* Count flash numbers */
	fdt_for_each_subnode(subnode, blob, node) {
		if (!strcmp(fdt_getprop(blob, subnode, "compatible", &len),
			    "spi-nand-flash"))
			plat->flash_type[flash_num] = 1;
		else
			plat->flash_type[flash_num] = 0;
		plat->fspi_rx_bus_width =
			fdtdec_get_int(blob, subnode,
				       "fspi-rx-bus-width",
				       NXP_FSPI_DEFAULT_SPI_RX_BUS_WIDTH);
		plat->fspi_tx_bus_width =
			fdtdec_get_int(blob, subnode,
				       "fspi-tx-bus-width",
				       NXP_FSPI_DEFAULT_SPI_TX_BUS_WIDTH);
		++flash_num;
	}

	if (flash_num == 0) {
		debug("Error: Missing flashes!\n");
		return -ENODEV;
	}

	plat->speed_hz = fdtdec_get_int(blob, node, "spi-max-frequency",
					NXP_FSPI_DEFAULT_SCK_FREQ);
	plat->num_chipselect = fdtdec_get_int(blob, node, "num-cs",
					      NXP_FSPI_MAX_CHIPSELECT_NUM);

	plat->reg_base = res_regs.start;
	plat->amba_base = 0;
	plat->memmap_phy = res_mem.start;
	plat->amba_total_size = res_mem.end - res_mem.start + 1;
	plat->flash_num = flash_num;

	debug("%s: regs=<0x%llx> <0x%llx, 0x%llx>\n",
	      __func__,
	      (u64)plat->reg_base,
	      (u64)plat->amba_base,
	      (u64)plat->amba_total_size);

	debug("max-frequency=%d, endianness=%s\n",
	      plat->speed_hz,
	      plat->flags & FSPI_FLAG_REGMAP_ENDIAN_BIG ? "be" : "le");

	return 0;
}

static int nxp_fspi_claim_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct nxp_fspi_priv *priv = dev_get_priv(bus);
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

	debug("FSPI Bus Claim for CS [%x]\n", slave_plat->cs);

	priv->current_cs = slave_plat->cs;
	priv->cur_amba_base = priv->amba_base[0] +
			      NXP_FSPI_FLASH_SIZE * slave_plat->cs;

	return 0;
}

static int nxp_fspi_release_bus(struct udevice *dev)
{
	debug("FSPI release bus\n");
	return 0;
}

static int nxp_fspi_xfer(struct udevice *dev, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	struct udevice *bus = dev->parent;
	struct nxp_fspi_priv *priv = dev_get_priv(bus);

	return fspi_xfer(priv, bitlen, dout, din, flags);
}

static int nxp_fspi_set_speed(struct udevice *bus, uint speed)
{
	debug("FSPI speed change is not supported\n");
	return 0;
}

static int nxp_fspi_set_mode(struct udevice *bus, uint mode)
{
	debug("FSPI Mode change is not supported\n");
	return 0;
}

static const struct dm_spi_ops nxp_fspi_ops = {
	.claim_bus	= nxp_fspi_claim_bus,
	.release_bus	= nxp_fspi_release_bus,
	.xfer		= nxp_fspi_xfer,
	.set_speed	= nxp_fspi_set_speed,
	.set_mode	= nxp_fspi_set_mode,
};

static const struct udevice_id nxp_fspi_ids[] = {
	{ .compatible = "nxp,dn-fspi" },
	{ }
};

U_BOOT_DRIVER(nxp_fspi) = {
	.name				= "nxp_fspi",
	.id				= UCLASS_SPI,
	.of_match			= nxp_fspi_ids,
	.ops				= &nxp_fspi_ops,
	.ofdata_to_platdata		= nxp_fspi_ofdata_to_platdata,
	.platdata_auto_alloc_size	= sizeof(struct nxp_fspi_platdata),
	.priv_auto_alloc_size		= sizeof(struct nxp_fspi_priv),
	.probe				= nxp_fspi_probe,
	.child_pre_probe		= nxp_fspi_child_pre_probe,
};
#endif
