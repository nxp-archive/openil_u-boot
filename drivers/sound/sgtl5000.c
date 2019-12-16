// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2021 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 */

#define LOG_CATEGORY UCLASS_SOUND

#include <common.h>
#include <audio_codec.h>
#include <dm.h>
#include <i2c.h>
#include "sgtl5000.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
/*
 * struct reg_default - Default value for a register.
 *
 * @reg: Register address.
 * @def: Register default value.
 *
 * We use an array of structs rather than a simple array as many modern devices
 * have very sparse register maps.
 */
struct reg_default {
	unsigned int reg;
	unsigned int def;
};

/* default value of sgtl5000 registers */
static const struct reg_default sgtl5000_reg_defaults[] = {
	{ SGTL5000_CHIP_DIG_POWER,		0x0000 },
	{ SGTL5000_CHIP_I2S_CTRL,		0x0010 },
	{ SGTL5000_CHIP_SSS_CTRL,		0x0010 },
	{ SGTL5000_CHIP_ADCDAC_CTRL,		0x020c },
	{ SGTL5000_CHIP_DAC_VOL,		0x3c3c },
	{ SGTL5000_CHIP_PAD_STRENGTH,		0x015f },
	{ SGTL5000_CHIP_ANA_ADC_CTRL,		0x0000 },
	{ SGTL5000_CHIP_ANA_HP_CTRL,		0x1818 },
	{ SGTL5000_CHIP_ANA_CTRL,		0x0111 },
	{ SGTL5000_CHIP_REF_CTRL,		0x0000 },
	{ SGTL5000_CHIP_MIC_CTRL,		0x0000 },
	{ SGTL5000_CHIP_LINE_OUT_CTRL,		0x0000 },
	{ SGTL5000_CHIP_LINE_OUT_VOL,		0x0404 },
	{ SGTL5000_CHIP_PLL_CTRL,		0x5000 },
	{ SGTL5000_CHIP_CLK_TOP_CTRL,		0x0000 },
	{ SGTL5000_CHIP_ANA_STATUS,		0x0000 },
	{ SGTL5000_CHIP_SHORT_CTRL,		0x0000 },
	{ SGTL5000_CHIP_ANA_TEST2,		0x0000 },
	{ SGTL5000_DAP_CTRL,			0x0000 },
	{ SGTL5000_DAP_PEQ,			0x0000 },
	{ SGTL5000_DAP_BASS_ENHANCE,		0x0040 },
	{ SGTL5000_DAP_BASS_ENHANCE_CTRL,	0x051f },
	{ SGTL5000_DAP_AUDIO_EQ,		0x0000 },
	{ SGTL5000_DAP_SURROUND,		0x0040 },
	{ SGTL5000_DAP_EQ_BASS_BAND0,		0x002f },
	{ SGTL5000_DAP_EQ_BASS_BAND1,		0x002f },
	{ SGTL5000_DAP_EQ_BASS_BAND2,		0x002f },
	{ SGTL5000_DAP_EQ_BASS_BAND3,		0x002f },
	{ SGTL5000_DAP_EQ_BASS_BAND4,		0x002f },
	{ SGTL5000_DAP_MAIN_CHAN,		0x8000 },
	{ SGTL5000_DAP_MIX_CHAN,		0x0000 },
	{ SGTL5000_DAP_AVC_CTRL,		0x0510 },
	{ SGTL5000_DAP_AVC_THRESHOLD,		0x1473 },
	{ SGTL5000_DAP_AVC_ATTACK,		0x0028 },
	{ SGTL5000_DAP_AVC_DECAY,		0x0050 }
};

#define LDO_VOLTAGE	1200000
#define LINREG_VDDD	((1600 - LDO_VOLTAGE / 1000) / 50)

enum sgtl5000_micbias_resistor {
	SGTL5000_MICBIAS_OFF = 0,
	SGTL5000_MICBIAS_2K = 2,
	SGTL5000_MICBIAS_4K = 4,
	SGTL5000_MICBIAS_8K = 8,
};

enum {
	I2S_LRCLK_STRENGTH_DISABLE,
	I2S_LRCLK_STRENGTH_LOW,
	I2S_LRCLK_STRENGTH_MEDIUM,
	I2S_LRCLK_STRENGTH_HIGH,
};

enum {
	I2S_SCLK_STRENGTH_DISABLE,
	I2S_SCLK_STRENGTH_LOW,
	I2S_SCLK_STRENGTH_MEDIUM,
	I2S_SCLK_STRENGTH_HIGH,
};

enum {
	I2S_FORMAT_I2S = 0,
	I2S_FORMAT_LEFT,
	I2S_FORMAT_RIGHT,
	I2S_FORMAT_PCMA,
	I2S_FORMAT_PCMB
};

/* sgtl5000 private structure in codec */
struct sgtl5000_priv {
	struct udevice *dev;
	int sysclk;	/* sysclk rate */
	int master;	/* i2s master or not */
	int fmt;	/* i2s data format */
	int num_supplies;
	int revision;
	u8 micbias_resistor;
	u8 micbias_voltage;
	u8 lrclk_strength;
	u8 sclk_strength;
};

/**
 * sgtl5000_i2c_read() - Read a 16-bit register
 *
 * @priv: Private driver data
 * @reg: Register number to read
 * @returns data read or -ve on error
 */
static int sgtl5000_i2c_read(struct sgtl5000_priv *priv, uint reg)
{
	u8 buf[2];
	int ret;

	ret = dm_i2c_read(priv->dev, reg, buf, sizeof(u16));
	if (ret < 0)
		return ret;
	return buf[0] << 8 | buf[1];
}

/**
 * sgtl5000_i2c_write() - Write a 16-bit register
 *
 * @priv: Private driver data
 * @reg: Register number to read
 * @data: Data to write
 * @returns 0 if OK, -ve on error
 */
static int sgtl5000_i2c_write(struct sgtl5000_priv *priv, uint reg, uint data)
{
	u8 buf[2];

	buf[0] = (data >> 8) & 0xff;
	buf[1] = data & 0xff;

	return dm_i2c_write(priv->dev, reg, buf, sizeof(u16));
}

static int sgtl5000_i2c_modify(struct sgtl5000_priv *priv, uint reg,
			       uint mask, uint data)
{
	int tmp;

	tmp = sgtl5000_i2c_read(priv, reg);
	tmp &= ~mask;
	tmp |= data;

	if (tmp < 0)
		return tmp;
	return sgtl5000_i2c_write(priv, reg, tmp);
}

/*
 * Write all the default values from sgtl5000_reg_defaults[] array into the
 * sgtl5000 registers, to make sure we always start with the sane registers
 * values as stated in the datasheet.
 *
 * Since sgtl5000 does not have a reset line, nor a reset command in software,
 * we follow this approach to guarantee we always start from the default values
 * and avoid problems like, not being able to probe after an audio playback
 * followed by a system reset or a 'reboot' command in Linux
 */
static void sgtl5000_fill_defaults(struct sgtl5000_priv *priv)
{
	int i, ret, val, index;

	for (i = 0; i < ARRAY_SIZE(sgtl5000_reg_defaults); i++) {
		val = sgtl5000_reg_defaults[i].def;
		index = sgtl5000_reg_defaults[i].reg;
		ret = sgtl5000_i2c_write(priv, index, val);
		if (ret)
			printf("%s: error %d setting reg 0x%04X to 0x%04X\n",
			       __func__, ret, index, val);
	}
}

static void sgtl5000_set_sample(struct sgtl5000_priv *priv, u32 sample_rate)
{
	u16 value = SGTL5000_MCLK_FREQ_PLL;

	sgtl5000_i2c_modify(priv, SGTL5000_CHIP_ANA_POWER,
			    SGTL5000_PLL_POWERUP | SGTL5000_VCOAMP_POWERUP,
			    SGTL5000_PLL_POWERUP | SGTL5000_VCOAMP_POWERUP);

	switch (sample_rate) {
	case 8000:
		value |= (SGTL5000_SYS_FS_32k << SGTL5000_SYS_FS_SHIFT) |
			 (SGTL5000_RATE_MODE_DIV_4 << SGTL5000_RATE_MODE_SHIFT);
		break;

	case 11025:
		value |= (SGTL5000_SYS_FS_44_1k << SGTL5000_SYS_FS_SHIFT) |
			 (SGTL5000_RATE_MODE_DIV_4 << SGTL5000_RATE_MODE_SHIFT);
		break;

	case 12000:
		value |= (SGTL5000_SYS_FS_48k << SGTL5000_SYS_FS_SHIFT) |
			 (SGTL5000_RATE_MODE_DIV_4 << SGTL5000_RATE_MODE_SHIFT);
		break;

	case 16000:
		value |= (SGTL5000_SYS_FS_32k << SGTL5000_SYS_FS_SHIFT) |
			 (SGTL5000_RATE_MODE_DIV_2 << SGTL5000_RATE_MODE_SHIFT);
		break;

	case 22050:
		value |= (SGTL5000_SYS_FS_44_1k << SGTL5000_SYS_FS_SHIFT) |
			 (SGTL5000_RATE_MODE_DIV_2 << SGTL5000_RATE_MODE_SHIFT);
		break;

	case 24000:
		value |= (SGTL5000_SYS_FS_48k << SGTL5000_SYS_FS_SHIFT) |
			 (SGTL5000_RATE_MODE_DIV_2 << SGTL5000_RATE_MODE_SHIFT);
		break;

	case 32000:
		value |= (SGTL5000_SYS_FS_32k << SGTL5000_SYS_FS_SHIFT) |
			 (SGTL5000_RATE_MODE_DIV_1 << SGTL5000_RATE_MODE_SHIFT);
		break;

	case 44100:
		value |= (SGTL5000_SYS_FS_44_1k << SGTL5000_SYS_FS_SHIFT) |
			 (SGTL5000_RATE_MODE_DIV_1 << SGTL5000_RATE_MODE_SHIFT);
		break;

	case 48000:
		value |= (SGTL5000_SYS_FS_48k << SGTL5000_SYS_FS_SHIFT) |
			 (SGTL5000_RATE_MODE_DIV_1 << SGTL5000_RATE_MODE_SHIFT);
		break;

	case 96000:
		value |= (SGTL5000_SYS_FS_96k << SGTL5000_SYS_FS_SHIFT) |
			 (SGTL5000_RATE_MODE_DIV_1 << SGTL5000_RATE_MODE_SHIFT);
		break;

	default:
		break;
	}
	sgtl5000_i2c_write(priv, SGTL5000_CHIP_CLK_CTRL, value);
}

static void sgtl5000_set_clock(struct sgtl5000_priv *priv, u32 sample_rate)
{
	ofnode node = priv->dev->node;
	u32 sys_mclk;
	float PLL_OUTPUT_FREQ, PLL_INPUT_FREQ;
	u8 INT_DIVISOR, FRAC_DIVISOR;
	u16 value;

	sgtl5000_set_sample(priv, sample_rate);

	if (ofnode_read_u32(node, "sys_mclk", &sys_mclk))
		sys_mclk = 25000000;

	if (sys_mclk > 17000000) {
		sgtl5000_i2c_modify(priv, SGTL5000_CHIP_CLK_TOP_CTRL,
				    SGTL5000_INPUT_FREQ_DIV2,
				    SGTL5000_INPUT_FREQ_DIV2);
		PLL_INPUT_FREQ = sys_mclk / 2000000.0;
	} else {
		sgtl5000_i2c_modify(priv, SGTL5000_CHIP_CLK_TOP_CTRL,
				    SGTL5000_INPUT_FREQ_DIV2, 0);
		PLL_INPUT_FREQ = sys_mclk / 1000000.0;
	}

	if (sample_rate == 44100)
		PLL_OUTPUT_FREQ = 180.6336;
	else
		PLL_OUTPUT_FREQ = 196.608;

	INT_DIVISOR = (u8)(PLL_OUTPUT_FREQ / PLL_INPUT_FREQ);
	FRAC_DIVISOR = (u8)(((PLL_OUTPUT_FREQ / PLL_INPUT_FREQ) - INT_DIVISOR)
				* 2048);

	value = (INT_DIVISOR << SGTL5000_PLL_INT_DIV_SHIFT) | FRAC_DIVISOR;
	sgtl5000_i2c_write(priv, SGTL5000_CHIP_PLL_CTRL, value);
}

static u16 regs_table[] = {
	SGTL5000_CHIP_ID,
	SGTL5000_CHIP_DIG_POWER,
	SGTL5000_CHIP_CLK_CTRL,
	SGTL5000_CHIP_I2S_CTRL,
	SGTL5000_CHIP_SSS_CTRL,
	SGTL5000_CHIP_ADCDAC_CTRL,
	SGTL5000_CHIP_DAC_VOL,
	SGTL5000_CHIP_PAD_STRENGTH,
	SGTL5000_CHIP_ANA_ADC_CTRL,
	SGTL5000_CHIP_ANA_HP_CTRL,
	SGTL5000_CHIP_ANA_CTRL,
	SGTL5000_CHIP_LINREG_CTRL,
	SGTL5000_CHIP_REF_CTRL,
	SGTL5000_CHIP_MIC_CTRL,
	SGTL5000_CHIP_LINE_OUT_CTRL,
	SGTL5000_CHIP_LINE_OUT_VOL,
	SGTL5000_CHIP_ANA_POWER,
	SGTL5000_CHIP_PLL_CTRL,
	SGTL5000_CHIP_CLK_TOP_CTRL,
	SGTL5000_CHIP_ANA_STATUS,
	SGTL5000_CHIP_ANA_TEST1,
	SGTL5000_CHIP_ANA_TEST2,
	SGTL5000_CHIP_SHORT_CTRL,
	SGTL5000_DAP_CTRL,
	SGTL5000_DAP_PEQ,
	SGTL5000_DAP_BASS_ENHANCE,
	SGTL5000_DAP_BASS_ENHANCE_CTRL,
	SGTL5000_DAP_AUDIO_EQ,
	SGTL5000_DAP_SURROUND,
	SGTL5000_DAP_FILTER_COEF_ACCESS,
	SGTL5000_DAP_COEF_WR_B0_MSB,
	SGTL5000_DAP_COEF_WR_B0_LSB,
	SGTL5000_DAP_EQ_BASS_BAND0, /* 115 Hz  */
	SGTL5000_DAP_EQ_BASS_BAND1, /* 330 Hz  */
	SGTL5000_DAP_EQ_BASS_BAND2, /* 990 Hz  */
	SGTL5000_DAP_EQ_BASS_BAND3, /* 3000 Hz */
	SGTL5000_DAP_EQ_BASS_BAND4, /* 9900 Hz */
	SGTL5000_DAP_MAIN_CHAN,
	SGTL5000_DAP_MIX_CHAN,
	SGTL5000_DAP_AVC_CTRL,
	SGTL5000_DAP_AVC_THRESHOLD,
	SGTL5000_DAP_AVC_ATTACK,
	SGTL5000_DAP_AVC_DECAY,
	SGTL5000_DAP_COEF_WR_B1_MSB,
	SGTL5000_DAP_COEF_WR_B1_LSB,
	SGTL5000_DAP_COEF_WR_B2_MSB,
	SGTL5000_DAP_COEF_WR_B2_LSB,
	SGTL5000_DAP_COEF_WR_A1_MSB,
	SGTL5000_DAP_COEF_WR_A1_LSB,
	SGTL5000_DAP_COEF_WR_A2_MSB,
	SGTL5000_DAP_COEF_WR_A2_LSB
};

static void sgtl5000_get_regs(struct sgtl5000_priv *priv)
{
	u8 reg_num = ARRAY_SIZE(regs_table);
	int value;

	for (u8 i = 0; i < reg_num; i++) {
		value = sgtl5000_i2c_read(priv, regs_table[i]);
		if (value >= 0)
			printf("[codec-0x%04X] = 0x%04X\n", regs_table[i],
			       value);
	}
}

/**
 * Initialise sgtl5000 codec device
 *
 * @priv: Private driver data
 * @returns 0 if OK, -ve on error
 */
int sgtl5000_device_init(struct sgtl5000_priv *priv)
{
	ofnode node = priv->dev->node;
	int ret;
	u32 vdda, vddio, sclk_strength;
	u16 value;

	/* read chip information */
	ret = sgtl5000_i2c_read(priv, SGTL5000_CHIP_ID);
	if (ret < 0) {
		printf("Error reading chip id %d\n", ret);
		return ret;
	}

	if (((ret & SGTL5000_PARTID_MASK) >> SGTL5000_PARTID_SHIFT) !=
	    SGTL5000_PARTID_PART_ID) {
		printf("Device with ID register is not a sgtl5000\n");
	}

	ret = (ret & SGTL5000_REVID_MASK) >> SGTL5000_REVID_SHIFT;
	printf("sgtl5000 revision 0x%x\n", ret);
	priv->revision = ret;

	/* Ensure sgtl5000 will start with sane register values */
	sgtl5000_fill_defaults(priv);

	ret = sgtl5000_i2c_write(priv, SGTL5000_CHIP_ANA_POWER, 0x42E0);
	if (ret) {
		printf("Error %d setting CHIP_ANA_POWER to 0x4260\n", ret);
		return 1;
	}

	if (ofnode_read_u32(node, "VDDA-supply", &vdda)) {
		printf("Error getting VDDA-supply\n");
		return 1;
	}

	if (ofnode_read_u32(node, "VDDIO-supply", &vddio)) {
		printf("Error getting VDDIO-supply\n");
		return 1;
	}

	if (vdda < 3100 && vddio < 3100) {
		sgtl5000_i2c_write(priv, SGTL5000_CHIP_CLK_TOP_CTRL,
				   SGTL5000_INT_OSC_EN);

		value = ((vdda / 2 - SGTL5000_ANA_GND_BASE) /
			 SGTL5000_ANA_GND_STP) << SGTL5000_ANA_GND_SHIFT;
		value |= SGTL5000_BIAS_CTRL_MASK;
		sgtl5000_i2c_write(priv, SGTL5000_CHIP_REF_CTRL, value);

		value = (vddio / 2 - SGTL5000_LINE_OUT_GND_BASE) /
			 SGTL5000_LINE_OUT_GND_STP;
		value |= SGTL5000_LINE_OUT_CURRENT_360u <<
			 SGTL5000_LINE_OUT_CURRENT_SHIFT;
		sgtl5000_i2c_write(priv, SGTL5000_CHIP_LINE_OUT_CTRL, value);

		sgtl5000_i2c_modify(priv, SGTL5000_CHIP_REF_CTRL,
				    SGTL5000_SMALL_POP, SGTL5000_SMALL_POP);
	}

	if (!ofnode_read_u32(node, "sclk-strength", &sclk_strength))
		sgtl5000_i2c_modify(priv, SGTL5000_CHIP_PAD_STRENGTH,
				    SGTL5000_PAD_I2S_SCLK_MASK,
				    sclk_strength <<
				    SGTL5000_PAD_I2S_SCLK_SHIFT);
	sgtl5000_i2c_write(priv, SGTL5000_CHIP_DIG_POWER, SGTL5000_DAC_EN |
			   SGTL5000_DAP_POWERUP | SGTL5000_I2S_IN_POWERUP);
	sgtl5000_i2c_write(priv, SGTL5000_CHIP_LINE_OUT_VOL, 0x0F0F);

	sgtl5000_i2c_modify(priv, SGTL5000_CHIP_SSS_CTRL, SGTL5000_DAP_SEL_MASK
			    | SGTL5000_DAC_SEL_MASK, (SGTL5000_DAP_SEL_I2S_IN <<
			    SGTL5000_DAP_SEL_SHIFT) | (SGTL5000_DAC_SEL_DAP <<
			    SGTL5000_DAC_SEL_SHIFT));
	sgtl5000_i2c_modify(priv, SGTL5000_CHIP_ANA_CTRL, SGTL5000_LINE_OUT_MUTE
			    | SGTL5000_HP_MUTE, 0);

	sgtl5000_i2c_modify(priv, SGTL5000_DAP_CTRL, SGTL5000_DAP_EN,
			    SGTL5000_DAP_EN);
	sgtl5000_i2c_modify(priv, SGTL5000_CHIP_ANA_POWER, SGTL5000_DAC_POWERUP
			    | SGTL5000_LINE_OUT_POWERUP, SGTL5000_DAC_POWERUP |
			    SGTL5000_LINE_OUT_POWERUP);
	sgtl5000_i2c_modify(priv, SGTL5000_CHIP_ADCDAC_CTRL,
			    SGTL5000_DAC_VOL_RAMP_EN | SGTL5000_DAC_MUTE_RIGHT |
			    SGTL5000_DAC_MUTE_LEFT, 0);
	return 0;
}

static int sgtl5000_set_params(struct udevice *dev, int interface, int rate,
			       int mclk_freq, int bits_per_sample,
			       uint channels)
{
	struct sgtl5000_priv *priv = dev_get_priv(dev);
	u16 value = 0;

	if (channels == 2)
		value = SGTL5000_DAC_STEREO;
	sgtl5000_i2c_modify(priv, SGTL5000_CHIP_ANA_POWER, SGTL5000_DAC_STEREO,
			    value);

	value = SGTL5000_I2S_MASTER;
	switch (bits_per_sample) {
	case 32:
		value |= SGTL5000_I2S_DLEN_32 << SGTL5000_I2S_DLEN_SHIFT;
		break;

	case 24:
		value |= SGTL5000_I2S_DLEN_24 << SGTL5000_I2S_DLEN_SHIFT;
		break;

	case 20:
		value |= SGTL5000_I2S_DLEN_20 << SGTL5000_I2S_DLEN_SHIFT;
		break;

	case 16:
		value |= SGTL5000_I2S_SCLKFREQ_32FS <<
			 SGTL5000_I2S_SCLKFREQ_SHIFT;
		value |= SGTL5000_I2S_DLEN_16 << SGTL5000_I2S_DLEN_SHIFT;
		break;

	default:
		value |= SGTL5000_I2S_DLEN_24 << SGTL5000_I2S_DLEN_SHIFT;
	}
	sgtl5000_i2c_write(priv, SGTL5000_CHIP_I2S_CTRL, value);
	sgtl5000_set_clock(priv, rate);

	return 0;
}

static int sgtl5000_probe(struct udevice *dev)
{
	struct sgtl5000_priv *priv = dev_get_priv(dev);

	priv->dev = dev;
	i2c_set_chip_offset_len(dev, 2);

	return sgtl5000_device_init(priv);
}

static const struct audio_codec_ops sgtl5000_ops = {
	.set_params     = sgtl5000_set_params,
};

static const struct udevice_id sgtl5000_ids[] = {
	{ .compatible = "fsl,sgtl5000" },
	{ }
};

U_BOOT_DRIVER(sgtl5000) = {
	.name           = "sgtl5000",
	.id             = UCLASS_AUDIO_CODEC,
	.of_match       = sgtl5000_ids,
	.ops            = &sgtl5000_ops,
	.probe          = sgtl5000_probe,
	.priv_auto_alloc_size   = sizeof(struct sgtl5000_priv),
};
