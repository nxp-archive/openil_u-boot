/*
 * freescale flexcan Support
 *
 * Copyright 2018 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _FLEXCAN_H_
#define _FLEXCAN_H_

#include <stdbool.h>
#include <common.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <asm/interrupt-gic.h>

/************************************
CAN pins
CAN3_RX:GPIO3_23
CAN4_RX:GPIO3_22
CAN3_Tx:GPIO3_16
CAN4_Tx:GPIO3_15
************************************/

#define FLEXCAN_ERRNUM_FREEZE			1
#define FLEXCAN_ERRNUM_UNFREEZE			2
#define FLEXCAN_ERRNUM_ENABLE			3
#define FLEXCAN_ERRNUM_RESET			4

/* 8 for RX fifo and 2 error handling */
#define FLEXCAN_NAPI_WEIGHT		(8 + 2)

/* FLEXCAN module configuration register (CANMCR) bits */
#define FLEXCAN_MCR_MDIS				BIT(31)
#define FLEXCAN_MCR_FRZ					BIT(30)
#define FLEXCAN_MCR_FEN					BIT(29)
#define FLEXCAN_MCR_HALT				BIT(28)
#define FLEXCAN_MCR_NOT_RDY				BIT(27)
#define FLEXCAN_MCR_WAK_MSK				BIT(26)
#define FLEXCAN_MCR_SOFTRST				BIT(25)
#define FLEXCAN_MCR_FRZ_ACK				BIT(24)
#define FLEXCAN_MCR_SUPV				BIT(23)
#define FLEXCAN_MCR_SLF_WAK				BIT(22)
#define FLEXCAN_MCR_WRN_EN				BIT(21)
#define FLEXCAN_MCR_LPM_ACK				BIT(20)
#define FLEXCAN_MCR_WAK_SRC				BIT(19)
#define FLEXCAN_MCR_DOZE				BIT(18)
#define FLEXCAN_MCR_SRX_DIS				BIT(17)
#define FLEXCAN_MCR_BCC					BIT(16)
#define FLEXCAN_MCR_LPRIO_EN				BIT(13)
#define FLEXCAN_MCR_AEN					BIT(12)
#define FLEXCAN_MCR_MAXMB(x)				((x) & 0x7f)
#define FLEXCAN_MCR_IDAM_A				(0x0 << 8)
#define FLEXCAN_MCR_IDAM_B				(0x1 << 8)
#define FLEXCAN_MCR_IDAM_C				(0x2 << 8)
#define FLEXCAN_MCR_IDAM_D				(0x3 << 8)

/* FLEXCAN control register (CANCTRL) bits */
#define FLEXCAN_CTRL_PRESDIV(x)				(((x) & 0xff) << 24)
#define FLEXCAN_CTRL_RJW(x)				(((x) & 0x03) << 22)
#define FLEXCAN_CTRL_PSEG1(x)				(((x) & 0x07) << 19)
#define FLEXCAN_CTRL_PSEG2(x)				(((x) & 0x07) << 16)
#define FLEXCAN_CTRL_BOFF_MSK				BIT(15)
#define FLEXCAN_CTRL_ERR_MSK				BIT(14)
#define FLEXCAN_CTRL_CLK_SRC				BIT(13)
#define FLEXCAN_CTRL_LPB				BIT(12)
#define FLEXCAN_CTRL_TWRN_MSK				BIT(11)
#define FLEXCAN_CTRL_RWRN_MSK				BIT(10)
#define FLEXCAN_CTRL_SMP				BIT(7)
#define FLEXCAN_CTRL_BOFF_REC				BIT(6)
#define FLEXCAN_CTRL_TSYN				BIT(5)
#define FLEXCAN_CTRL_LBUF				BIT(4)
#define FLEXCAN_CTRL_LOM				BIT(3)
#define FLEXCAN_CTRL_PROPSEG(x)				((x) & 0x07)
#define FLEXCAN_CTRL_ERR_BUS				(FLEXCAN_CTRL_ERR_MSK)
#define FLEXCAN_CTRL_ERR_STATE \
	(FLEXCAN_CTRL_TWRN_MSK | FLEXCAN_CTRL_RWRN_MSK | \
	 FLEXCAN_CTRL_BOFF_MSK)
#define FLEXCAN_CTRL_ERR_ALL \
	(FLEXCAN_CTRL_ERR_BUS | FLEXCAN_CTRL_ERR_STATE)

#define FLEXCAN_CTRL_BITTIME_CLEAR			(0x0000FFF8)

/* FLEXCAN control register 2 (CTRL2) bits */
#define FLEXCAN_CTRL2_ECRWRE				BIT(29)
#define FLEXCAN_CTRL2_WRMFRZ				BIT(28)
#define FLEXCAN_CTRL2_RFFN(x)				(((x) & 0x0f) << 24)
#define FLEXCAN_CTRL2_TASD(x)				(((x) & 0x1f) << 19)
#define FLEXCAN_CTRL2_MRP				BIT(18)
#define FLEXCAN_CTRL2_RRS				BIT(17)
#define FLEXCAN_CTRL2_EACEN				BIT(16)

/* FLEXCAN memory error control register (MECR) bits */
#define FLEXCAN_MECR_ECRWRDIS				BIT(31)
#define FLEXCAN_MECR_HANCEI_MSK				BIT(19)
#define FLEXCAN_MECR_FANCEI_MSK				BIT(18)
#define FLEXCAN_MECR_CEI_MSK				BIT(16)
#define FLEXCAN_MECR_HAERRIE				BIT(15)
#define FLEXCAN_MECR_FAERRIE				BIT(14)
#define FLEXCAN_MECR_EXTERRIE				BIT(13)
#define FLEXCAN_MECR_RERRDIS				BIT(9)
#define FLEXCAN_MECR_ECCDIS				BIT(8)
#define FLEXCAN_MECR_NCEFAFRZ				BIT(7)

/* FLEXCAN error and status register (ESR) bits */
#define FLEXCAN_ESR_TWRN_INT				BIT(17)
#define FLEXCAN_ESR_RWRN_INT				BIT(16)
#define FLEXCAN_ESR_BIT1_ERR				BIT(15)
#define FLEXCAN_ESR_BIT0_ERR				BIT(14)
#define FLEXCAN_ESR_ACK_ERR				BIT(13)
#define FLEXCAN_ESR_CRC_ERR				BIT(12)
#define FLEXCAN_ESR_FRM_ERR				BIT(11)
#define FLEXCAN_ESR_STF_ERR				BIT(10)
#define FLEXCAN_ESR_TX_WRN				BIT(9)
#define FLEXCAN_ESR_RX_WRN				BIT(8)
#define FLEXCAN_ESR_IDLE				BIT(7)
#define FLEXCAN_ESR_TXRX				BIT(6)
#define FLEXCAN_EST_FLT_CONF_SHIFT			(4)
#define FLEXCAN_ESR_FLT_CONF_MASK	(0x3 << FLEXCAN_EST_FLT_CONF_SHIFT)
#define FLEXCAN_ESR_FLT_CONF_ACTIVE	(0x0 << FLEXCAN_EST_FLT_CONF_SHIFT)
#define FLEXCAN_ESR_FLT_CONF_PASSIVE	(0x1 << FLEXCAN_EST_FLT_CONF_SHIFT)
#define FLEXCAN_ESR_BOFF_INT				BIT(2)
#define FLEXCAN_ESR_ERR_INT				BIT(1)
#define FLEXCAN_ESR_WAK_INT				BIT(0)
#define FLEXCAN_ESR_ERR_BUS \
	(FLEXCAN_ESR_BIT1_ERR | FLEXCAN_ESR_BIT0_ERR | \
	 FLEXCAN_ESR_ACK_ERR | FLEXCAN_ESR_CRC_ERR | \
	 FLEXCAN_ESR_FRM_ERR | FLEXCAN_ESR_STF_ERR)
#define FLEXCAN_ESR_ERR_STATE \
	(FLEXCAN_ESR_TWRN_INT | FLEXCAN_ESR_RWRN_INT | FLEXCAN_ESR_BOFF_INT)
#define FLEXCAN_ESR_ERR_ALL \
	(FLEXCAN_ESR_ERR_BUS | FLEXCAN_ESR_ERR_STATE)
#define FLEXCAN_ESR_ALL_INT \
	(FLEXCAN_ESR_TWRN_INT | FLEXCAN_ESR_RWRN_INT | \
	 FLEXCAN_ESR_BOFF_INT | FLEXCAN_ESR_ERR_INT)

/* FLEXCAN interrupt flag register (IFLAG) bits */
/* Errata ERR005829 step7: Reserve first valid MB */
#define FLEXCAN_TX_BUF_RESERVED				8
#define FLEXCAN_TX_BUF_ID				9
#define FLEXCAN_IFLAG_BUF(x)				BIT(x)
#define FLEXCAN_IFLAG_RX_FIFO_OVERFLOW			BIT(7)
#define FLEXCAN_IFLAG_RX_FIFO_WARN			BIT(6)
#define FLEXCAN_IFLAG_RX_FIFO_AVAILABLE			BIT(5)
#define FLEXCAN_IFLAG_DEFAULT \
	(FLEXCAN_IFLAG_RX_FIFO_OVERFLOW | FLEXCAN_IFLAG_RX_FIFO_AVAILABLE | \
	 FLEXCAN_IFLAG_BUF(FLEXCAN_TX_BUF_ID))

/* FLEXCAN message buffers */
#define FLEXCAN_MB_CODE_RX_INACTIVE			(0x0 << 24)
#define FLEXCAN_MB_CODE_RX_EMPTY			(0x4 << 24)
#define FLEXCAN_MB_CODE_RX_FULL				(0x2 << 24)
#define FLEXCAN_MB_CODE_RX_OVERRUN			(0x6 << 24)
#define FLEXCAN_MB_CODE_RX_RANSWER			(0xa << 24)

#define FLEXCAN_MB_CODE_TX_INACTIVE			(0x8 << 24)
#define FLEXCAN_MB_CODE_TX_ABORT			(0x9 << 24)
#define FLEXCAN_MB_CODE_TX_DATA				(0xc << 24)
#define FLEXCAN_MB_CODE_TX_TANSWER			(0xe << 24)

#define FLEXCAN_MB_CNT_SRR				BIT(22)
#define FLEXCAN_MB_CNT_IDE				BIT(21)
#define FLEXCAN_MB_CNT_RTR				BIT(20)
#define FLEXCAN_MB_CNT_LENGTH(x)			(((x) & 0xf) << 16)
#define FLEXCAN_MB_CNT_TIMESTAMP(x)			((x) & 0xffff)

#define FLEXCAN_MB_ID_PRIO(x)				(((x) & 0x7) << 29)
#define FLEXCAN_MB_ID_STANDARD(x)			(((x) & 0x7FF) << 18)
#define FLEXCAN_MB_DATA_BYTE0_4(x)		(((u32)(x) & 0xFF) << 24)
#define FLEXCAN_MB_DATA_BYTE1_5(x)		(((u32)(x) & 0xFF) << 16)
#define FLEXCAN_MB_DATA_BYTE2_6(x)		(((u32)(x) & 0xFF) << 8)
#define FLEXCAN_MB_DATA_BYTE3_7(x)			((u32)(x) & 0xFF)

#define FLEXCAN_TIMEOUT_US				(50)

/* Structure of the message buffer */
struct flexcan_mb {
	u32 can_ctrl;
	u32 can_id;
	u32 data[2];
};

/* Structure of the hardware registers */
struct can_module {
	u32 MCR;		/* 0x00 */
	u32 CTRL1;		/* 0x04 */
	u32 TIMER;		/* 0x08 */
	u32 _reserved1;		/* 0x0c */
	u32 RXGMASK;		/* 0x10 */
	u32 RX14MASK;		/* 0x14 */
	u32 RX15MASK;		/* 0x18 */
	u32 ecr;		/* 0x1c */
	u32 ESR;		/* 0x20 */
	u32 imask2;		/* 0x24 */
	u32 IMASK1;		/* 0x28 */
	u32 iflag2;		/* 0x2c */
	u32 IFLAG1;		/* 0x30 */
	u32 ctrl2;		/* 0x34 */
	u32 esr2;		/* 0x38 */
	u32 imeur;		/* 0x3c */
	u32 lrfr;		/* 0x40 */
	u32 crcr;		/* 0x44 */
	u32 RXFGMASK;		/* 0x48 */
	u32 rxfir;		/* 0x4c */
	u32 _reserved3[12];	/* 0x50 */
	struct flexcan_mb mb[64];	/* 0x80 */
	/* FIFO-mode:
	 *			MB
	 * 0x080...0x08f	0	RX message buffer
	 * 0x090...0x0df	1-5	reserverd
	 * 0x0e0...0x0ff	6-7	8 entry ID table
	 *				(mx25, mx28, mx35, mx53)
	 * 0x0e0...0x2df	6-7..37	8..128 entry ID table
	 *				size conf'ed via ctrl2::RFFN
	 *				(mx6, vf610)
	 */
	u32 _reserved4[408];	/* 0x480 */
	u32 mecr;		/* 0xae0 */
	u32 erriar;		/* 0xae4 */
	u32 erridpr;		/* 0xae8 */
	u32 errippr;		/* 0xaec */
	u32 rerrar;		/* 0xaf0 */
	u32 rerrdr;		/* 0xaf4 */
	u32 rerrsynr;		/* 0xaf8 */
	u32 errsr;		/* 0xafc */
};

/* Structure of the CAN bittiming */
struct can_bittiming_t {
	u8 presdiv;
	u8 rjw;
	u8 pseg1;
	u8 pseg2;
	u8 propseg;
};

struct can_ctrlmode_t {
	u8 loopmode:1;
	u8 listenonly:1;
	u8 samples:1;
	u8 err_report:1;
	u8 rx_irq:1;
	u8 res:3;
};

struct can_init_t {
	struct can_module *canx;
	struct can_bittiming_t *bt;
	struct can_ctrlmode_t *ctrlmode;
	u32 reg_ctrl_default;
	u32 reg_esr;
};

#define CAN1_BASE           ((u32)0x02A70000)
#define CAN2_BASE           ((u32)0x02A80000)
#define CAN3_BASE           ((u32)0x02A90000)
#define CAN4_BASE           ((u32)0x02AA0000)

#define CAN1                ((struct can_module *)CAN1_BASE)
#define CAN2                ((struct can_module *)CAN2_BASE)
#define CAN3                ((struct can_module *)CAN3_BASE)
#define CAN4                ((struct can_module *)CAN4_BASE)

enum can_clksrc_e {
	FLEXCAN_CLKSRC_OSC = 0,		/* the crystal oscillator clock */
	FLEXCAN_CLKSRC_PLL = 1		/* the peripheral clock */
};

#define BITRATE_500K		(500*1000)
#define BITRATE_250K		(250*1000)

/* special address description flags for the CAN_ID */
#define CAN_EFF_FLAG 0x80000000U /* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG 0x40000000U /* remote transmission request */
#define CAN_ERR_FLAG 0x20000000U /* error message frame */

/* valid bits in CAN ID for frame formats */
#define CAN_SFF_MASK 0x000007FFU /* standard frame format (SFF) */
#define CAN_EFF_MASK 0x1FFFFFFFU /* extended frame format (EFF) */
#define CAN_ERR_MASK 0x1FFFFFFFU /* omit EFF, RTR, ERR flags */

/* CAN payload length and DLC definitions according to ISO 11898-1 */
#define CAN_MAX_DLC 8
#define CAN_MAX_DLEN 8

#define get_can_dlc(i) ((i) < CAN_MAX_DLC ? (i) : CAN_MAX_DLC)

#define CAN_1000K 10
#define CAN_500K  20
#define CAN_250K  40
#define CAN_200K  50
#define CAN_125K  80
#define CAN_100K  100
#define CAN_50K   200
#define CAN_20K   500
#define CAN_10K   1000
#define CAN_5K    2000

#define CAN_BITTIM_INIT(bps) {\
	.presdiv = bps,\
	.rjw = 1,\
	.pseg1 = 6,\
	.pseg2 = 2,\
	.propseg = 6\
}

/*
 * Controller Area Network Identifier structure
 *
 * bit 0-28	: CAN identifier (11/29 bit)
 * bit 29	: error message frame flag (0 = data frame, 1 = error message)
 * bit 30	: remote transmission request flag (1 = rtr frame)
 * bit 31	: frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
 */
typedef __u32 canid_t;

/**
 * struct can_frame - basic CAN frame structure
 * @can_id:  CAN ID of the frame and CAN_*_FLAG flags, see canid_t definition
 * @can_dlc: frame payload length in byte (0 .. 8) aka data length code
 *           N.B. the DLC field from ISO 11898-1 Chapter 8.4.2.3 has a 1:1
 *           mapping of the 'data length code' to the real payload length
 * @__pad:   padding
 * @__res0:  reserved / padding
 * @__res1:  reserved / padding
 * @data:    CAN frame payload (up to 8 byte)
 */
struct can_frame {
	canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	__u8    can_dlc; /* frame payload length in byte (0 .. CAN_MAX_DLEN) */
	__u8    __pad;   /* padding */
	__u8    __res0;  /* reserved / padding */
	__u8    __res1;  /* reserved / padding */
	__u8    data[CAN_MAX_DLEN] __aligned(8);
};

typedef void (*irq_func)(struct can_module *canx);

extern irq_func flexcan_rx_handle;
extern struct can_bittiming_t flexcan3_bittiming;
extern struct can_ctrlmode_t flexcan3_ctrlmode;
extern struct can_init_t flexcan3;

int flexcan_softRest(struct can_module *canx);
int flexcan_enable(struct can_module *canx);/* enable module */
int flexcan_disable(struct can_module *canx);/* disable module */
void flexcan_stop(struct can_init_t *can_init);/* disable clock */
int flexcan_send(struct can_module *canx, struct can_frame *cf);
void flexcan_receive(struct can_module *canx, struct can_frame *cf);
int flexcan_init(void);
#endif
