// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2021 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 */

#ifndef _FSL_SAI_H_
#define _FSL_SAI_H_

struct i2s_module {
	u32 I2S_TCSR;
	u32 I2S_TCR1;
	u32 I2S_TCR2;
	u32 I2S_TCR3;
	u32 I2S_TCR4;
	u32 I2S_TCR5;
	u32 rev0[2];
	u32 I2S_TDR[8];
	u32 I2S_TFR[8];
	u32 I2S_TMR;
	u32 rev1[7];
	u32 I2S_RCSR;
	u32 I2S_RCR1;
	u32 I2S_RCR2;
	u32 I2S_RCR3;
	u32 I2S_RCR4;
	u32 I2S_RCR5;
	u32 rev2[2];
	u32 I2S_RDR[8];
	u32 I2S_RFR[8];
	u32 I2S_RMR;
};

/* SAI Transmit Control Register ( I2Sx_TCSR ) */
#define TCSR_TE_MASK            0x80000000 /* Transmitter Enable */
#define TCSR_BCE_MASK           0x10000000 /* Bit Clock Enable */
#define TCSR_FR_MASK            0x02000000 /* FIFO Reset */
#define TCSR_SR_MASK            0x01000000 /* Software Reset */
#define TCSR_WSF_MASK           0x00100000 /* Word Start Flag */
#define TCSR_SEF_MASK           0x00080000 /* Sync Error Flag */
#define TCSR_FEF_MASK           0x00040000 /* FIFO Error Flag */
#define TCSR_FWF_MASK           0x00020000 /* FIFO Warning Flag */
#define TCSR_FRF_MASK           0x00010000 /* FIFO Request Flag */
#define TCSR_WSIE_MASK          0x00001000 /* Word Start Interrupt Enable */
#define TCSR_SEIE_MASK          0x00000800 /* Sync Error Interrupt Enable */
#define TCSR_FEIE_MASK          0x00000400 /* FIFO Error Interrupt Enable */
#define TCSR_FWIE_MASK          0x00000200 /* FIFO Warning Interrupt Enable */
#define TCSR_FRIE_MASK          0x00000100 /* FIFO Request Interrupt Enable */
#define TCSR_FWDE_MASK          0x00000002 /* FIFO Warning DMA Enable */
#define TCSR_FRDE_MASK          0x00000001 /* FIFO Request DMA Enable */

/* SAI Transmit Configuration 1 Register ( I2Sx_TCR1 ) */
#define TCR1_TFW_MASK           0x0000001F /* Transmit FIFO Watermark */
#define TCR1_TFW(X)             (X)

/* SAI Transmit Configuration 2 Register ( I2Sx_TCR2 ) */
#define TCR2_SYNC_MASK          0xC0000000 /* Synchronous Mode */
#define TCR2_SYNC_ASYNC         0x00000000
#define TCR2_SYNC_SYNC          0x40000000
#define TCR2_BCS_MASK           0x20000000 /* Bit Clock Swap */
#define TCR2_BCS_NORMAL         0x00000000
#define TCR2_BCS_SWAP           0x20000000
#define TCR2_BCI_MASK           0x10000000 /* Bit Clock Input */
#define TCR2_MSEL_MASK          0x0C000000 /* MCLK Select */
#define TCR2_MSEL(X)            ((X) << 26)
#define TCR2_BCP_MASK           0x02000000 /* Bit Clock Polarity */
#define TCR2_BCP_HIGH           0x00000000
#define TCR2_BCP_LOW            0x02000000
#define TCR2_BCD_MASK           0x01000000 /* Bit Clock Direction */
#define TCR2_BCD_SLAVE          0x00000000
#define TCR2_BCD_MASTER         0x01000000
#define TCR2_DIV_MASK           0x000000FF /* Bit Clock Divide */
#define TCR2DIV(X)              (X)

/*SAI Transmit Configuration 3 Register ( I2Sx_TCR3 )*/
#define TCR3_TCE_MASK           0x00010000 /* Transmit Channel Enable */
#define TCR3_TCE(X)             ((X) << 16)
#define TCR3_WDFL_MASK          0x0000001F /* Word Flag Configuration */
#define TCR3_WDFL(X)            ((X) - 1)

/* I2Sx_TCR4 */
#define TCR4_FRSZ_MASK          0x001F0000 /* Frame size */
#define TCR4_FRSZ(X)            ((((X) - 1) << 16) & TCR4_FRSZ_MASK)
#define TCR4_SYWD_MASK          0x00001F00 /* Sync Width */
#define TCR4_SYWD(X)            ((((X) - 1) << 8) & TCR4_SYWD_MASK)
#define TCR4_MF_MASK            0x00000010 /* MSB First */
#define TCR4_MF_LSB             0x00000000
#define TCR4_MF_MSB             0x00000010
#define TCR4_FSE_MASK           0x00000008 /* Frame Sync Early */
#define TCR4_FSE_FIRST          0x00000000
#define TCR4_FSE_EARLY          0x00000008
#define TCR4_FSP_MASK           0x00000002 /* Frame Sync Polarity */
#define TCR4_FSP_HIGH           0x00000000
#define TCR4_FSP_LOW            0x00000002
#define TCR4_FSD_MASK           0x00000001 /* Frame Sync Direction */
#define TCR4_FSD_SLAVE          0x00000000
#define TCR4_FSD_MASTER         0x00000001

/* SAI Transmit Configuration 5 Register ( I2Sx_TCR5 ) */
#define TCR5_WNW_MASK           0x1F000000 /* Word N Width */
#define TCR5_WNW(X)             ((((X) - 1) << 24) & TCR5_WNW_MASK)
#define TCR5_W0W_MASK           0x001F0000 /* Word 0 Width */
#define TCR5_W0W(X)             ((((X) - 1) << 16) & TCR5_W0W_MASK)
#define TCR5_FBT_MASK           0x00001F00 /* First Bit Shifted */
#define TCR5_FBT(X)             ((((X) - 1) << 8) & TCR5_FBT_MASK)

#define FIFO_SIZE               32
#define FIFO_WATERMARK          16
#define I2S_TX_ON               1
#define I2S_TX_OFF              0
#define CHANNEL1                0x01
#define CHANNEL2                0x02

#define I2S_BUSLEFTJUSTIFIED    0x01
#define I2S_BUSRIGHTJUSTIFIED   0x02
#define I2S_BUSI2S              0x03
#define I2S_BUSPCMA             0x04
#define I2S_BUSPCMB             0x05

#define I2S_SLAVE               0x00
#define I2S_MASTER              0x01

#endif
