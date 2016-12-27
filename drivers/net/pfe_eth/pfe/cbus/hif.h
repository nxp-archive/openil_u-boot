/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef _HIF_H_
#define _HIF_H_

/** @file hif.h.
 * hif - PFE hif block control and status register.
 * Mapped on CBUS and accessible from all PE's and ARM.
 */
#define HIF_VERSION	(HIF_BASE_ADDR + 0x00)
#define HIF_TX_CTRL	(HIF_BASE_ADDR + 0x04)
#define HIF_TX_CURR_BD_ADDR	(HIF_BASE_ADDR + 0x08)
#define HIF_TX_ALLOC	(HIF_BASE_ADDR + 0x0c)
#define HIF_TX_BDP_ADDR	(HIF_BASE_ADDR + 0x10)
#define HIF_TX_STATUS	(HIF_BASE_ADDR + 0x14)
#define HIF_RX_CTRL	(HIF_BASE_ADDR + 0x20)
#define HIF_RX_BDP_ADDR	(HIF_BASE_ADDR + 0x24)
#define HIF_RX_STATUS	(HIF_BASE_ADDR + 0x30)
#define HIF_INT_SRC	(HIF_BASE_ADDR + 0x34)
#define HIF_INT_ENABLE	(HIF_BASE_ADDR + 0x38)
#define HIF_POLL_CTRL	(HIF_BASE_ADDR + 0x3c)
#define HIF_RX_CURR_BD_ADDR	(HIF_BASE_ADDR + 0x40)
#define HIF_RX_ALLOC	(HIF_BASE_ADDR + 0x44)
#define HIF_TX_DMA_STATUS	(HIF_BASE_ADDR + 0x48)
#define HIF_RX_DMA_STATUS	(HIF_BASE_ADDR + 0x4c)
#define HIF_INT_COAL	(HIF_BASE_ADDR + 0x50)
#define HIF_AXI_CTRL	(HIF_BASE_ADDR + 0x54)

/* HIF_INT_SRC/ HIF_INT_ENABLE control bits */
#define HIF_INT			(1 << 0)
#define HIF_RXBD_INT		(1 << 1)
#define HIF_RXPKT_INT		(1 << 2)
#define HIF_TXBD_INT		(1 << 3)
#define HIF_TXPKT_INT		(1 << 4)

/* HIF_TX_CTRL bits */
#define HIF_CTRL_DMA_EN		(1<<0)
#define HIF_CTRL_BDP_POLL_CTRL_EN	(1<<1)
#define HIF_CTRL_BDP_CH_START_WSTB	(1<<2)

/* HIF_RX_STATUS bits */
#define BDP_CSR_RX_DMA_ACTV	(1<<16)

/* HIF_INT_ENABLE bits */
#define HIF_INT_EN	(1 << 0)
#define HIF_RXBD_INT_EN	(1 << 1)
#define HIF_RXPKT_INT_EN	(1 << 2)
#define HIF_TXBD_INT_EN	(1 << 3)
#define HIF_TXPKT_INT_EN	(1 << 4)

/* HIF_POLL_CTRL bits*/
#define HIF_RX_POLL_CTRL_CYCLE	0x0400
#define HIF_TX_POLL_CTRL_CYCLE	0x0400

/* Buffer descriptor control bits */
#define BD_CTRL_BUFLEN_MASK	(0xffff)
#define BD_BUF_LEN(x)	(x & BD_CTRL_BUFLEN_MASK)
#define BD_CTRL_CBD_INT_EN	(1 << 16)
#define BD_CTRL_PKT_INT_EN	(1 << 17)
#define BD_CTRL_LIFM	(1 << 18)
#define BD_CTRL_LAST_BD	(1 << 19)
#define BD_CTRL_DIR	(1 << 20)
#define BD_CTRL_PKT_XFER	(1 << 24)
#define BD_CTRL_DESC_EN	(1 << 31)
#define BD_CTRL_PARSE_DISABLE	(1 << 25)
#define BD_CTRL_BRFETCH_DISABLE	(1 << 26)
#define BD_CTRL_RTFETCH_DISABLE	(1 << 27)

#endif /* _HIF_H_ */
