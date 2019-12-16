// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2021 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 */

#include <common.h>
#include <dm.h>
#include <i2s.h>
#include <sound.h>
#include "fsl-sai.h"

#define TIMEOUT_I2S_TX		100	/* i2s transfer timeout */

/*
 * Writes the register for I2S
 *
 * @param i2s_reg	i2s register address
 * @param mask		mask value of the corresponding flag bit
 * @param value         the value written to register
 */
static void i2s_write_reg(u32 *i2s_reg, u32 mask, u32 value)
{
	u32 tmp;

	tmp = *i2s_reg;
	tmp &= ~mask;
	tmp |= value;
	*i2s_reg = tmp;
}

static void i2s_get_all_regs(struct i2s_module *i2s_reg)
{
	u32 *data = (u32 *)i2s_reg;

	for (; data <= &i2s_reg->I2S_TCR5; data++)
		printf("[SAI-0x%08X] = 0x%08X\n", (u32)data, *data);
	printf("[SAI-0x%08X] = 0x%08X\n", (u32)i2s_reg->I2S_TFR,
	       i2s_reg->I2S_TFR[0]);
	printf("[SAI-0x%08X] = 0x%08X\n", (u32)(&i2s_reg->I2S_TMR),
	       i2s_reg->I2S_TMR);
}

/*
 * Gets the status flag for I2S
 *
 * @param i2s_reg	i2s register address
 * @param mask		mask value of the corresponding flag bit
 */
static u8 i2s_tx_get_status(struct i2s_module *i2s_reg, u32 mask)
{
	return (i2s_reg->I2S_TCSR & mask) ? 1 : 0;
}

/*
 * Sets the i2s transfer control
 *
 * @param i2s_reg	i2s register address
 * @param on		1 enable tx , 0 disable tx transfer
 */
static void i2s_tx_ctrl(struct i2s_module *i2s_reg, u8 on)
{
	u32 tmp1 = 0, tmp2 = 0;

	if (on) {
		tmp1 = TCSR_TE_MASK | TCSR_BCE_MASK;
		tmp2 = TCR3_TCE_MASK;
	}

	i2s_write_reg(&i2s_reg->I2S_TCR3, TCR3_TCE_MASK, tmp2);
	i2s_write_reg(&i2s_reg->I2S_TCSR, TCSR_TE_MASK | TCSR_BCE_MASK, tmp1);
}

/*
 * resets the i2s tx fifo
 *
 * @param i2s_reg	i2s register address
 */
static void i2s_tx_fifo_reset(struct i2s_module *i2s_reg)
{
	if (i2s_tx_get_status(i2s_reg, TCSR_TE_MASK) ||
	    i2s_tx_get_status(i2s_reg, TCSR_FEF_MASK))
		i2s_write_reg(&i2s_reg->I2S_TCSR, TCSR_FR_MASK, TCSR_FR_MASK);
}

/*
 * resets the i2s transmitter
 *
 * @param i2s_reg	i2s register address
 */
static void i2s_tx_reset(struct i2s_module *i2s_reg)
{
	/* Set the software reset and FIFO reset to clear internal state */
	i2s_reg->I2S_TCSR = TCSR_FR_MASK | TCSR_SR_MASK;
	/* Clear software reset bit, this should be done by software */
	i2s_reg->I2S_TCSR &= ~TCSR_SR_MASK;
	/* Reset all Tx register values */
	i2s_reg->I2S_TCR2 = 0;
	i2s_reg->I2S_TCR3 = 0;
	i2s_reg->I2S_TCR4 = 0;
	i2s_reg->I2S_TCR5 = 0;
	i2s_reg->I2S_TMR = 0;
}

/*
 * Configures between asynchronous and synchronous modes of operation
 *
 * @param i2s_reg       i2s register address
 * @param mode          asynchronous or synchronous
 */
static void i2s_tx_set_mode(struct i2s_module *i2s_reg, u32 mode)
{
	i2s_write_reg(&i2s_reg->I2S_TCR2, TCR2_SYNC_MASK, mode);
}

/*
 * swaps the bit clock used by the transmitter
 *
 * @param i2s_reg       i2s register address
 * @param swap          set bit clock source
 */
static void i2s_tx_bitclock_swap(struct i2s_module *i2s_reg, u32 swap)
{
	i2s_write_reg(&i2s_reg->I2S_TCR2, TCR2_BCS_MASK, swap);
}

/*
 * Configures the polarity of the bit clock
 *
 * @param i2s_reg       i2s register address
 * @param polarity      bit clock is active high or low
 */
static void i2s_tx_bitclock_polarity(struct i2s_module *i2s_reg, u32 polarity)
{
	i2s_write_reg(&i2s_reg->I2S_TCR2, TCR2_BCP_MASK, polarity);
}

/*
 * Configures the direction of the bit clock
 *
 * @param i2s_reg       i2s register address
 * @param dir           slave(externally) or master(internally) mode
 */
static void i2s_tx_bitclock_direction(struct i2s_module *i2s_reg, u32 dir)
{
	i2s_write_reg(&i2s_reg->I2S_TCR2, TCR2_BCD_MASK, dir);
}

/*
 * Configures which word sets the start of word flag
 *
 * @param i2s_reg       i2s register address
 * @param num           the value written must be one less than the word number
 */
static u8 i2s_tx_set_start_word(struct i2s_module *i2s_reg, u8 num)
{
	if (num <= 32) {
		i2s_write_reg(&i2s_reg->I2S_TCR3, TCR3_WDFL_MASK,
			      TCR3_WDFL(num));
		return 0;
	}
	return 1;
}

/*
 * Configures the number of words in each frame
 *
 * @param i2s_reg	i2s register address
 * @param words		the number of words
 */
static u8 i2s_tx_set_framesize(struct i2s_module *i2s_reg, u8 words)
{
	if (words <= 32) {
		i2s_write_reg(&i2s_reg->I2S_TCR4, TCR4_FRSZ_MASK,
			      TCR4_FRSZ(words));
		return 0;
	}
	return 1;
}

/*
 * Configures the length of the frame sync in number of bit clocks
 *
 * @param i2s_reg	i2s register address
 * @param width		the length of the frame sync in number of bit clocks
 */
static u8 i2s_tx_set_framewidth(struct i2s_module *i2s_reg, u8 width)
{
	if (width <= 32) {
		i2s_write_reg(&i2s_reg->I2S_TCR4, TCR4_SYWD_MASK,
			      TCR4_SYWD(width));
		return 0;
	}
	return 1;
}

/*
 * Configures whether the LSB or the MSB is transmitted first
 *
 * @param i2s_reg	i2s register address
 * @param first		LSB or MSB
 *
 */
static void i2s_tx_set_firstbit(struct i2s_module *i2s_reg, u32 first)
{
	i2s_write_reg(&i2s_reg->I2S_TCR4, TCR4_MF_MASK, first);
}

/*
 * @param i2s_reg       i2s register address
 * @param early         asserts frame sync
 */
static void i2s_tx_set_early(struct i2s_module *i2s_reg, u32 early)
{
	i2s_write_reg(&i2s_reg->I2S_TCR4, TCR4_FSE_MASK, early);
}

/*
 * Configures the polarity of the frame sync
 *
 * @param i2s_reg       i2s register address
 * @param polarity      Frame sync is active high or low
 */
static void i2s_tx_frame_polarity(struct i2s_module *i2s_reg, u32 polarity)
{
	i2s_write_reg(&i2s_reg->I2S_TCR4, TCR4_FSP_MASK, polarity);
}

/*
 * Configures the direction of the frame sync
 *
 * @param i2s_reg	i2s register address
 * @param dir		Clock direction
 */
static void i2s_tx_set_frame_dir(struct i2s_module *i2s_reg, u32 dir)
{
	i2s_write_reg(&i2s_reg->I2S_TCR4, TCR4_FSD_MASK, dir);
}

/*
 * Configures the number of bits in each word
 *
 * @param i2s_reg       i2s register address
 * @param width         the number of bits
 */
static u8 i2s_tx_set_word_width(struct i2s_module *i2s_reg, u8 width)
{
	if (width <= 32) {
		i2s_write_reg(&i2s_reg->I2S_TCR5, TCR5_WNW_MASK | TCR5_W0W_MASK,
			      TCR5_WNW(width) | TCR5_W0W(width));
		return 0;
	}
	return 1;
}

/*
 * Configures the bit index for the first bit transmitted
 * for each word in the frame
 *
 * @param i2s_reg       i2s register address
 * @param shift         the index of the first bit
 */
static u8 i2s_tx_set_firstbit_shifted(struct i2s_module *i2s_reg, u8 shift)
{
	if (shift <= 31) {
		i2s_write_reg(&i2s_reg->I2S_TCR5, TCR5_FBT_MASK,
			      TCR5_FBT(shift));
		return 0;
	}
	return 1;
}

static u8 *i2s_tx_write_fifo(struct i2s_uc_priv *pi2s_tx, u8 *data, u8 word_num)
{
	struct i2s_module *i2s_reg = (struct i2s_module *)pi2s_tx->base_address;
	u8 bytespersample = pi2s_tx->bitspersample / 8;
	u8 *ptr = data;
	u32 word = 0;
	u8 i;

	for (i = 0; i < word_num; i++) {
		word = 0;
		switch (bytespersample) {
		case 3:
			word |= (*(ptr + 2)) << 16;
		case 2:
			word |= (*(ptr + 1)) << 8;
		case 1:
			word |= *ptr;
			break;
		default:
			return NULL;
		}
		i2s_reg->I2S_TDR[0] = word;
		ptr += bytespersample;
	}

	return ptr;
}

int i2s_transfer_tx_data(struct i2s_uc_priv *pi2s_tx, void *data,
			 uint data_size)
{
	struct i2s_module *i2s_reg = (struct i2s_module *)pi2s_tx->base_address;
	u8 *ptr = data;
	u8 bitspersample = pi2s_tx->bitspersample;
	u8 bytespersample = bitspersample / 8;
	u8 channels = pi2s_tx->channels;
	u8 word_num;

	printf("%s\n", __func__);

	i2s_tx_set_framewidth(i2s_reg, bitspersample);
	i2s_tx_set_word_width(i2s_reg, bitspersample);
	i2s_tx_set_firstbit_shifted(i2s_reg, bitspersample);

	i2s_reg->I2S_TMR = ~0UL - ((1 << channels) - 1);

	i2s_tx_ctrl(i2s_reg, I2S_TX_ON);
	if (data_size >= (FIFO_SIZE * bytespersample))
		word_num = FIFO_SIZE;
	else
		word_num = data_size / bytespersample;
	/* Fill the tx buffer before starting the tx transmit */
	ptr = i2s_tx_write_fifo(pi2s_tx, data, word_num);
	if (!ptr) {
		printf("%d-bit sampling is not supported\n",
		       bitspersample);
		return 1;
	}
	data_size -= bytespersample * word_num;

	while (data_size > 0) {
		/* The transmit FIFO request flag is set  */
		if (i2s_tx_get_status(i2s_reg, TCSR_FRF_MASK)) {
			if (data_size >= FIFO_WATERMARK * bytespersample)
				word_num = FIFO_WATERMARK;
			else
				word_num = data_size / bytespersample;

			ptr = i2s_tx_write_fifo(pi2s_tx, ptr, word_num);
			data_size -= bytespersample * word_num;
		} else if (i2s_tx_get_status(i2s_reg, TCSR_FWF_MASK)) {
			if (data_size >= (FIFO_SIZE * bytespersample))
				word_num = FIFO_SIZE;
			else
				word_num = data_size / bytespersample;
			/* Fill the tx buffer before starting the tx transmit */
			ptr = i2s_tx_write_fifo(pi2s_tx, data, word_num);
			data_size -= bytespersample * word_num;
		}
	}

	printf("The music waits for the end!\n");
	/* Wait for FIFO to be empty */
	while (!i2s_tx_get_status(i2s_reg, TCSR_FWF_MASK))
		;

	i2s_tx_ctrl(i2s_reg, I2S_TX_OFF);
	printf("The music is finished!\n");

	return 0;
}

static void i2s_set_format(struct i2s_module *i2s_reg, u8 format)
{
	switch (format) {
	case I2S_BUSLEFTJUSTIFIED:
		/* Bit clock is active low */
		i2s_reg->I2S_TCR2 |= TCR2_BCP_LOW;
		/* Configures one word sets the start of word flag */
		i2s_reg->I2S_TCR3 &= ~TCR3_WDFL_MASK;
		/*
		 * MSB; Sync Width: 32 bit clocks; first bit of the frame
		 * Frame sync is active low; Frame size: 2 words
		 */
		i2s_reg->I2S_TCR4 = TCR4_MF_MSB | TCR4_SYWD(32) | TCR4_FSE_FIRST
				    | TCR4_FSP_LOW | TCR4_FRSZ(2);
		break;

	case I2S_BUSRIGHTJUSTIFIED:
		i2s_reg->I2S_TCR2 |= TCR2_BCP_LOW;
		i2s_reg->I2S_TCR3 &= ~TCR3_WDFL_MASK;
		i2s_reg->I2S_TCR4 = TCR4_MF_MSB | TCR4_SYWD(32) | TCR4_FSE_FIRST
				    | TCR4_FSP_HIGH | TCR4_FRSZ(2);
		break;

	case I2S_BUSI2S:
		i2s_reg->I2S_TCR2 |= TCR2_BCP_LOW;
		i2s_reg->I2S_TCR3 &= ~TCR3_WDFL_MASK;
		i2s_reg->I2S_TCR4 = TCR4_MF_MSB | TCR4_SYWD(32) | TCR4_FSE_EARLY
				    | TCR4_FSP_LOW | TCR4_FRSZ(2);
		break;

	case I2S_BUSPCMA:
		i2s_reg->I2S_TCR2 &= ~TCR2_BCP_LOW;
		i2s_reg->I2S_TCR3 &= ~TCR3_WDFL_MASK;
		i2s_reg->I2S_TCR4 = TCR4_MF_MSB | TCR4_SYWD(1) | TCR4_FSE_EARLY
				    | TCR4_FSP_HIGH | TCR4_FRSZ(2);
		break;

	case I2S_BUSPCMB:
		i2s_reg->I2S_TCR2 &= ~TCR2_BCP_LOW;
		i2s_reg->I2S_TCR3 &= ~TCR3_WDFL_MASK;
		i2s_reg->I2S_TCR4 = TCR4_MF_MSB | TCR4_SYWD(1) | TCR4_FSE_FIRST
				    | TCR4_FSP_HIGH | TCR4_FRSZ(2);
		break;
	}
}

static void i2s_set_clock_mode(struct i2s_module *i2s_reg, u8 mode)
{
	if (mode) { /* master mode */
		i2s_tx_bitclock_direction(i2s_reg, TCR2_BCD_MASTER);
		i2s_tx_set_frame_dir(i2s_reg, TCR4_FSD_MASTER);
	} else {
		i2s_tx_bitclock_direction(i2s_reg, TCR2_BCD_SLAVE);
		i2s_tx_set_frame_dir(i2s_reg, TCR4_FSD_SLAVE);
	}
}

static int sai_tx_init(struct i2s_uc_priv *pi2s_tx)
{
	int ret = 0;
	struct i2s_module *i2s_reg = (struct i2s_module *)pi2s_tx->base_address;

	i2s_tx_reset(i2s_reg);
	i2s_set_format(i2s_reg, I2S_BUSI2S);           /* I2S Format */
	i2s_set_clock_mode(i2s_reg, I2S_SLAVE);        /* Slave mode */
	i2s_tx_set_mode(i2s_reg, TCR2_SYNC_ASYNC);     /* Async mode */
	i2s_reg->I2S_TCR1 = TCR1_TFW(FIFO_SIZE / 2);   /* Watermark */

	return ret;
}

static int fsl_sai_tx_data(struct udevice *dev, void *data, uint data_size)
{
	struct i2s_uc_priv *priv = dev_get_uclass_priv(dev);

	return i2s_transfer_tx_data(priv, data, data_size);
}

static int fsl_sai_probe(struct udevice *dev)
{
	struct i2s_uc_priv *priv = dev_get_uclass_priv(dev);

	return sai_tx_init(priv);
}

static int fsl_sai_ofdata_to_platdata(struct udevice *dev)
{
	struct i2s_uc_priv *priv = dev_get_uclass_priv(dev);
	ofnode node = dev->node;
	u32 reg[4];

	if (!ofnode_read_u32_array(node, "reg", reg, 4))
		priv->base_address = reg[1];
	else
		goto err;

	switch (priv->base_address) {
	case 0x0F100000: /* SAI1 */
	case 0x0F110000: /* SAI2 */
		priv->id = 114;
		break;

	case 0x0F120000: /* SAI3 */
	case 0x0F130000: /* SAI4 */
		priv->id = 115;
		break;

	case 0x0F140000: /* SAI5 */
	case 0x0F150000: /* SAI6 */
		priv->id = 116;
		break;
	}

	printf("%s\n", __func__);
	return 0;

err:
	debug("fail to get sound i2s node properties\n");

	return -EINVAL;
}

static const struct i2s_ops fsl_sai_ops = {
	.tx_data	= fsl_sai_tx_data,
};

static const struct udevice_id fsl_sai_ids[] = {
	{ .compatible = "fsl,ls1028a-sai" },
	{ }
};

U_BOOT_DRIVER(fsl_sai) = {
	.name		= "fsl_sai",
	.id		= UCLASS_I2S,
	.of_match	= fsl_sai_ids,
	.probe		= fsl_sai_probe,
	.ofdata_to_platdata	= fsl_sai_ofdata_to_platdata,
	.ops		= &fsl_sai_ops,
};
