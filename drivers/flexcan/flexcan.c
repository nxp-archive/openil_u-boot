/*
 * freescale flexcan Support
 *
 * Copyright 2018 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <linux/delay.h>
#include "flexcan.h"

irq_func flexcan_rx_handle = NULL;

static u32 readl(void *addr)
{
	return *(u32 *)addr;
}

static void writel(u32 val, void *addr)
{
	*(u32 *)addr = val;
}

static u32 flexcan_read(void *addr)
{
	u32 le = readl(addr);
	u8 *mid = (u8 *)&le;

	return ((u32)mid[0] << 24) + ((u32)mid[1] << 16) +
	       ((u32)mid[2] << 8) + (u32)mid[3];
}

static void flexcan_write(u32 val, void *addr)
{
	u8 *mid = (u8 *)&val;
	u32 be = ((u32)mid[0] << 24) + ((u32)mid[1] << 16) +
		 ((u32)mid[2] << 8) + (u32)mid[3];

	writel(be, addr);
}

static u32 canData_be32(u8 *data)
{
	return FLEXCAN_MB_DATA_BYTE0_4(data[0]) |
	       FLEXCAN_MB_DATA_BYTE1_5(data[1]) |
	       FLEXCAN_MB_DATA_BYTE2_6(data[2]) |
	       FLEXCAN_MB_DATA_BYTE3_7(data[3]);
}

/*****************************************************************
low-power mode: Disable mode , Stop mode
return 0:not LP; 1:LP
****************************************************************/
static u32 flexcan_isLPMode(struct can_module *canx)
{
	u32 reg = flexcan_read(&(canx->MCR));
	return (reg & FLEXCAN_MCR_LPM_ACK) /*>> 20*/;
}

/*****************************************************************
The following registers are reset: MCR (except the MDIS bit),
TIMER , ECR, ESR1, ESR2, IMASK1, IMASK2, IFLAG1, IFLAG2 and CRCR.
return 0:OK;  1:timeout;  2:in LP
*****************************************************************/
int flexcan_softRest(struct can_module *canx)
{
	u32 reg = flexcan_read(&(canx->MCR));
	unsigned int timeout = FLEXCAN_TIMEOUT_US / 10;

	reg |= FLEXCAN_MCR_SOFTRST;
	flexcan_write(reg, &(canx->MCR));
	while (timeout-- && (flexcan_read(&(canx->MCR)) & FLEXCAN_MCR_SOFTRST))
		udelay(10);

	if (flexcan_read(&(canx->MCR)) & FLEXCAN_MCR_SOFTRST)
		return FLEXCAN_ERRNUM_RESET;

	return 0;
}

int flexcan_enable(struct can_module *canx)
{
	unsigned int timeout = FLEXCAN_TIMEOUT_US / 10;
	u32 reg = flexcan_read(&(canx->MCR));

	reg &= ~FLEXCAN_MCR_MDIS;
	flexcan_write(reg, &(canx->MCR));
	while (timeout-- && flexcan_isLPMode(canx))
		udelay(10);

	if (flexcan_isLPMode(canx))
		return FLEXCAN_ERRNUM_ENABLE;

	return 0;
}

int flexcan_disable(struct can_module *canx)
{
	unsigned int timeout = FLEXCAN_TIMEOUT_US / 10;
	u32 reg = flexcan_read(&(canx->MCR));

	reg |= FLEXCAN_MCR_MDIS;
	flexcan_write(reg, &(canx->MCR));
	while (timeout-- && !flexcan_isLPMode(canx))
		udelay(10);

	if (!flexcan_isLPMode(canx))
		return 1;

	return 0;
}

static void flexcan_setClockSource(struct can_module *canx,
				enum can_clksrc_e clksrc, bool is_enable)
{
	u32 reg = flexcan_read(&(canx->CTRL1));

	flexcan_disable(canx);
	switch (clksrc) {
	case FLEXCAN_CLKSRC_OSC:    /* the crystal oscillator clock */
		reg &= ~FLEXCAN_CTRL_CLK_SRC;
		break;

	case FLEXCAN_CLKSRC_PLL:
		reg |= FLEXCAN_CTRL_CLK_SRC;
		break;
	}
	flexcan_write(reg, &(canx->CTRL1));
	if (is_enable)
		flexcan_enable(canx);
}

static int flexcan_isFreezeMode(struct can_module *canx)
{
	u32 reg = flexcan_read(&(canx->MCR));
	return reg & FLEXCAN_MCR_FRZ_ACK;
}

/*****************************************************************
return 0:OK;  1:timeout;
*****************************************************************/
static int flexcan_setFreezeMode(struct can_module *canx)
{
	unsigned int timeout = 1000 * 1000 * 10 / BITRATE_500K;
	u32 reg = flexcan_read(&(canx->MCR));

	/*
	 * bit.FRZ:  Enabled to enter Freeze mode.
	 * bit.HALT: Enters Freeze mode.
	 */
	reg |= (FLEXCAN_MCR_FRZ | FLEXCAN_MCR_HALT);
	flexcan_write(reg, &(canx->MCR));

	while (timeout-- && !flexcan_isFreezeMode(canx))
		udelay(100);

	if (!flexcan_isFreezeMode(canx))
		return 1;

	return 0;
}

static int flexcan_unFreezeMode(struct can_module *canx)
{
	unsigned int timeout = FLEXCAN_TIMEOUT_US / 10;
	u32 reg = flexcan_read(&(canx->MCR));

	reg |= FLEXCAN_MCR_FRZ;
	reg &= ~FLEXCAN_MCR_HALT;
	flexcan_write(reg, &(canx->MCR));

	while (timeout-- && flexcan_isFreezeMode(canx))
		udelay(10);

	if (flexcan_isFreezeMode(canx))
		return 1;

	return 0;
}

/*
 * fCANCLK: Peripheral Clock（150MHz） or Oscillator Clock
 * fTq = fCANCLK / (PRESDIV + 1)
 * Bit Rate = fTq / (1 + PROPSEG + PSEG1 + 2 + PSEG2 + 1)
 */
static void flexcan_set_bittiming(struct can_module *canx,
				struct can_bittiming_t *bt)
{
	u32 reg = flexcan_read(&(canx->CTRL1));

	reg &= ~(FLEXCAN_CTRL_PRESDIV(0xff) |
		 FLEXCAN_CTRL_RJW(0x3) |
		 FLEXCAN_CTRL_PSEG1(0x7) |
		 FLEXCAN_CTRL_PSEG2(0x7) |
		 FLEXCAN_CTRL_PROPSEG(0x7));

	reg |= FLEXCAN_CTRL_PRESDIV(bt->presdiv - 1) |
		FLEXCAN_CTRL_PSEG1(bt->pseg1 - 1) |
		FLEXCAN_CTRL_PSEG2(bt->pseg2 - 1) |
		FLEXCAN_CTRL_RJW(bt->rjw - 1) |
		FLEXCAN_CTRL_PROPSEG(bt->propseg - 1);

	flexcan_write(reg, &(canx->CTRL1));
}

static void flexcan_set_ctrlmode(struct can_module *canx,
		struct can_ctrlmode_t *ctrlmode)
{
	u32 reg = flexcan_read(&(canx->CTRL1));

	reg &= ~(FLEXCAN_CTRL_LPB |		/* Loop Back Mode disable */
			 FLEXCAN_CTRL_SMP |	/* one CAN bit sample */
			 FLEXCAN_CTRL_LOM);	/* Listen-Only Mode disable */

	if (ctrlmode->loopmode)
		reg |= FLEXCAN_CTRL_LPB;
	if (ctrlmode->listenonly)
		reg |= FLEXCAN_CTRL_LOM;
	if (ctrlmode->samples)
		reg |= FLEXCAN_CTRL_SMP;

	flexcan_write(reg, &(canx->CTRL1));
}

/* flexcan_start
 *
 * this functions is entered with clocks enabled
 *
 */
static int flexcan_start(struct can_init_t *can_init)
{
	struct can_module *canx = can_init->canx;
	int err, i;
	u32 reg;

	/* enable module */
	err = flexcan_enable(canx);
	if (err)
		return err;

	/* soft reset */
	err = flexcan_softRest(canx);
	if (err)
		return err;

	flexcan_set_bittiming(canx, can_init->bt);
	flexcan_set_ctrlmode(canx, can_init->ctrlmode);

	if (!flexcan_setFreezeMode(canx)) {	/* enable freeze,halt now */
		/* MCR */
		reg = flexcan_read(&(canx->MCR));
		reg &= ~(FLEXCAN_MCR_MAXMB(0xff) | FLEXCAN_MCR_IDAM_D);
		reg |= FLEXCAN_MCR_FEN |	/* Rx FIFO enabled. */
		       FLEXCAN_MCR_SUPV |	/* only supervisor access */
		       FLEXCAN_MCR_WRN_EN |	/* Warning Interrupt Enable */
		       FLEXCAN_MCR_SRX_DIS |	/* Self Reception Disable */
		       FLEXCAN_MCR_IDAM_C |	/* choose format C */
		       /* set max mailbox number */
		       FLEXCAN_MCR_MAXMB(FLEXCAN_TX_BUF_ID);
		flexcan_write(reg, &(canx->MCR));

		/* CTRL */
		reg = flexcan_read(&(canx->CTRL1));
		reg &= ~FLEXCAN_CTRL_TSYN;/* disable timer sync feature */
		/* disable auto busoff recovery
		 * transmit lowest buffer first
		 * enable tx/rx warning and bus off interrupt
		 */
		reg |= FLEXCAN_CTRL_BOFF_REC |
		       FLEXCAN_CTRL_LBUF |
		       FLEXCAN_CTRL_ERR_STATE;

		/* enable the "error interrupt" (FLEXCAN_CTRL_ERR_MSK),
		 * on most Flexcan cores, too. Otherwise we don't get
		 * any error warning or passive interrupts.
		 */
		if (can_init->ctrlmode->err_report)
			reg |= FLEXCAN_CTRL_ERR_MSK;
		else
			reg &= ~FLEXCAN_CTRL_ERR_MSK;

		/* save for later use */
		can_init->reg_ctrl_default = reg;
		/* leave interrupts disabled for now */
		reg &= ~FLEXCAN_CTRL_ERR_ALL;
		flexcan_write(reg, &(canx->CTRL1));

		for (i = 0 ; i < 6; i++) {
			flexcan_write(FLEXCAN_MB_CODE_RX_EMPTY,
				      &(canx->mb[i].can_ctrl));
		}
		/* clear and invalidate all mailboxes first */
		for (i = FLEXCAN_TX_BUF_ID; i < ARRAY_SIZE(canx->mb); i++) {
			flexcan_write(FLEXCAN_MB_CODE_RX_INACTIVE,
				      &(canx->mb[i].can_ctrl));
		}

		/* Errata ERR005829: mark first TX mailbox as INACTIVE */
		flexcan_write(FLEXCAN_MB_CODE_TX_INACTIVE,
			      &(canx->mb[FLEXCAN_TX_BUF_RESERVED].can_ctrl));

		/* mark TX mailbox as INACTIVE */
		flexcan_write(FLEXCAN_MB_CODE_TX_INACTIVE,
			      &(canx->mb[FLEXCAN_TX_BUF_ID].can_ctrl));

		/* acceptance mask/acceptance code (accept everything) */
		flexcan_write(0x0, &(canx->RXGMASK));
		flexcan_write(0x0, &(canx->RX14MASK));
		flexcan_write(0x0, &(canx->RX15MASK));
		flexcan_write(0x0, &(canx->RXFGMASK));

		/* synchronize with the can bus */
		if (flexcan_unFreezeMode(canx))
			return FLEXCAN_ERRNUM_UNFREEZE;

		/* enable interrupts atomically */
		flexcan_write(can_init->reg_ctrl_default, &(canx->CTRL1));
		flexcan_write(FLEXCAN_IFLAG_DEFAULT, &(canx->IMASK1));
	} else {
		return FLEXCAN_ERRNUM_FREEZE;
	}
	return 0;
}

/* flexcan_stop
 *
 * this functions is entered with clocks disabled
 */
void flexcan_stop(struct can_init_t *can_init)
{
	struct can_module *canx = can_init->canx;

	/* freeze + disable module */
	flexcan_setFreezeMode(canx);
	flexcan_disable(canx);

	/* Disable all interrupts */
	flexcan_write(0, &(canx->IMASK1));
	flexcan_write(can_init->reg_ctrl_default & ~FLEXCAN_CTRL_ERR_ALL,
		      &(canx->CTRL1));
}

static inline int flexcan_has_and_handle_berr(struct can_ctrlmode_t ctrlmode,
						u32 reg_esr)
{
	return (ctrlmode.err_report) && (reg_esr & FLEXCAN_ESR_ERR_BUS);
}

struct can_bittiming_t flexcan3_bittiming = CAN_BITTIM_INIT(CAN_500K);

struct can_ctrlmode_t flexcan3_ctrlmode = {
	.loopmode = 0,
	.listenonly = 0,
	.samples = 0,
	.err_report = 1,
	.rx_irq = 0
};

struct can_init_t flexcan3 = {
	.canx = CAN3,
	.bt = &flexcan3_bittiming,
	.ctrlmode = &flexcan3_ctrlmode,
	.reg_ctrl_default = 0,
	.reg_esr = 0
};

static void flexcan_irq(int hw_irq, int src_coreid)
{
	u32 reg_iflag1, reg_esr;

	reg_iflag1 = flexcan_read(&(CAN3->IFLAG1));
	reg_esr = flexcan_read(&(CAN3->ESR));

	/* ACK all bus error and state change IRQ sources */
	if (reg_esr & FLEXCAN_ESR_ALL_INT) {
		flexcan_write(reg_esr & FLEXCAN_ESR_ALL_INT, &(CAN3->ESR));
		printf("flexcan error: 0x%x!\n", reg_esr);
	}

	/* schedule NAPI in case of:
	 * - rx IRQ
	 * - state change IRQ
	 * - bus error IRQ and bus error reporting is activated
	 */
	if ((reg_iflag1 & FLEXCAN_IFLAG_RX_FIFO_AVAILABLE) ||
	    (reg_esr & FLEXCAN_ESR_ERR_STATE) ||
	    flexcan_has_and_handle_berr(flexcan3_ctrlmode, reg_esr)) {
		/* The error bits are cleared on read,
		 * save them for later use.
		 */
		flexcan3.reg_esr = reg_esr & FLEXCAN_ESR_ERR_BUS;
		flexcan_write(flexcan3.reg_ctrl_default & ~FLEXCAN_CTRL_ERR_ALL,
			      &(CAN3->CTRL1));
		if (flexcan_rx_handle != NULL)
			flexcan_rx_handle(CAN3);
	}

	/* FIFO overflow */
	if (reg_iflag1 & FLEXCAN_IFLAG_RX_FIFO_OVERFLOW) {
		flexcan_write(FLEXCAN_IFLAG_RX_FIFO_OVERFLOW, &(CAN3->IFLAG1));
		printf("flexcan fifo overflow!\n");
	}

	/* transmission complete interrupt */
	if (reg_iflag1 & (1 << FLEXCAN_TX_BUF_ID))
		flexcan_write((1 << FLEXCAN_TX_BUF_ID), &(CAN3->IFLAG1));
}

static void flexcan_register_irq(u32 coreid, u32 hw_irq)
{
	gic_irq_register(hw_irq, flexcan_irq);
	gic_set_type(hw_irq);
	gic_set_target(1 << coreid, hw_irq);
	enable_interrupts();
}

int flexcan_send(struct can_module *canx, struct can_frame *cf)
{
	u32 can_id, data;
	u32 ctrl = FLEXCAN_MB_CODE_TX_DATA | (cf->can_dlc << 16);

	if (cf->can_id & CAN_EFF_FLAG) {
		can_id = cf->can_id & CAN_EFF_MASK;
		ctrl |= FLEXCAN_MB_CNT_IDE | FLEXCAN_MB_CNT_SRR;
	} else {
		can_id = (cf->can_id & CAN_SFF_MASK) << 18;
	}

	if (cf->can_id & CAN_RTR_FLAG)
		ctrl |= FLEXCAN_MB_CNT_RTR;

	if (cf->can_dlc > 0) {
		data = canData_be32((u8 *)&cf->data[0]);
		flexcan_write(data, &canx->mb[FLEXCAN_TX_BUF_ID].data[0]);
	}
	if (cf->can_dlc > 3) {
		data = canData_be32((u8 *)&cf->data[4]);
		flexcan_write(data, &canx->mb[FLEXCAN_TX_BUF_ID].data[1]);
	}

	flexcan_write(can_id, &canx->mb[FLEXCAN_TX_BUF_ID].can_id);
	flexcan_write(ctrl, &canx->mb[FLEXCAN_TX_BUF_ID].can_ctrl);

	/* Errata ERR005829 step8:
	 * Write twice INACTIVE(0x8) code to first MB.
	 */
	flexcan_write(FLEXCAN_MB_CODE_TX_INACTIVE,
		      &canx->mb[FLEXCAN_TX_BUF_RESERVED].can_ctrl);
	flexcan_write(FLEXCAN_MB_CODE_TX_INACTIVE,
		      &canx->mb[FLEXCAN_TX_BUF_RESERVED].can_ctrl);

	return 0;
}

void flexcan_receive(struct can_module *canx, struct can_frame *cf)
{
	struct flexcan_mb *mb = &canx->mb[0];
	u32 reg_ctrl, reg_id;

	reg_ctrl = flexcan_read(&mb->can_ctrl);
	reg_id = flexcan_read(&mb->can_id);
	if (reg_ctrl & FLEXCAN_MB_CNT_IDE)
		cf->can_id = ((reg_id >> 0) & CAN_EFF_MASK) | CAN_EFF_FLAG;
	else
		cf->can_id = (reg_id >> 18) & CAN_SFF_MASK;

	if (reg_ctrl & FLEXCAN_MB_CNT_RTR)
		cf->can_id |= CAN_RTR_FLAG;
	cf->can_dlc = get_can_dlc((reg_ctrl >> 16) & 0xf);

	*((u32 *)(cf->data + 0)) = readl(&mb->data[0]);
	*((u32 *)(cf->data + 4)) = readl(&mb->data[1]);

	/* mark as read */
	flexcan_write(FLEXCAN_IFLAG_RX_FIFO_AVAILABLE, &canx->IFLAG1);
	flexcan_write(FLEXCAN_MB_CODE_RX_EMPTY, &(canx->mb[0].can_ctrl));
	flexcan_read(&canx->TIMER);
}

int flexcan_init(void)
{
	int coreid = 1;
	int err;
	struct can_frame canfram_tx = {
		.can_id = 0x00000002,
		.can_dlc = 3,
		.data = "OK!"
	};

	mdelay(35000);
	/* the module is enabled */
	flexcan_setClockSource(CAN3, FLEXCAN_CLKSRC_PLL, true);
	flexcan_register_irq(coreid, 160);
	err = flexcan_start(&flexcan3);
	if (err)
		printf("flexcan init err: %d\n", err);
	else
		printf("flexcan init OK!\n");
		flexcan_send(CAN3, &canfram_tx);

	return 0;
}
