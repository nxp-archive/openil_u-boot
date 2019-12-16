// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2021 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 */

#ifndef _SGTL5000_H_
#define _SGTL5000_H_

/* Registers addresses */

/*
 * BITS | FIELD  | RW | RESET | DEFINITION
 * -----|--------|----|-------|---------------------------------------
 * 15:8 | PARTID | RO | 0xA0  | SGTL5000 Part ID
 *                              0xA0 - 8 bit identifier for SGTL5000
 *  7:0 | REVID  | RO | 0x00  | SGTL5000 Revision ID
 *                              0xHH - revision number for SGTL5000.
 */
#define SGTL5000_CHIP_ID	0x0000
/*
 * BITS |       FIELD     | RW | RESET | DEFINITION
 * -----|-----------------|----|-------|----------------------------------
 * 15:7 |       RSVD      | RO |  0x0  | Reserved
 *   6  |   ADC_POWERUP   | RW |  0x0  | Enable/disable the ADC block,
 *                                       both digital and analog
 *                                       0x0 = Disable
 *                                       0x1 = Enable
 *   5  |   DAC_POWERUP   | RW |  0x0  | Enable/disable the DAC block,
 *                                       both analog and digital
 *                                       0x0 = Disable
 *                                       0x1 = Enable
 *   4  |   DAP_POWERUP   | RW |  0x0  | Enable/disable the DAP block
 *                                       0x0 = Disable
 *                                       0x1 = Enable
 *  3:2 |       RSVD      | RW |  0x0  | Reserved
 *   1  | I2S_OUT_POWERUP | RW |  0x0  | Enable/disable the I2S data output
 *                                       0x0 = Disable
 *                                       0x1 = Enable
 *   0  | I2S_IN_POWERUP  | RW |  0x0  | Enable/disable the I2S data input
 *                                       0x0 = Disable
 *                                       0x1 = Enable
 */
#define SGTL5000_CHIP_DIG_POWER	0x0002
/*
 * BITS |   FIELD   | RW | RESET | DEFINITION
 * -----|-----------|----|-------|---------------------------------------------
 * 15:6 |    RSVD   | RO |  0x0  | Reserved
 *  5:4 | RATE_MODE | RW |  0x0  | Sets the sample rate mode.MCLK_FREQ is still
 *                                 specified relative to the rate in SYS_FS
 *                                 0x0 = SYS_FS specifies the rate
 *                                 0x1 = Rate is 1/2 of the SYS_FS rate
 *                                 0x2 = Rate is 1/4 of the SYS_FS rate
 *                                 0x3 = Rate is 1/6 of the SYS_FS rate
 *  3:2 |   SYS_FS  | RW |  0x2  | Sets the internal system sample rate
 *                                 0x0 = 32 kHz
 *                                 0x1 = 44.1 kHz
 *                                 0x2 = 48 kHz
 *                                 0x3 = 96 kHz
 *  1:0 | MCLK_FREQ | RW |  0x0  | Identifies incoming SYS_MCLK frequency and
 *                                 if the PLL should be used
 *                                 0x0 = 256*Fs
 *                                 0x1 = 384*Fs
 *                                 0x2 = 512*Fs
 *                                 0x3 = Use PLL
 *                               The 0x3 (Use PLL) setting must be used if the
 *                               SYS_MCLK is not a standard multiple of Fs
 *                               (256, 384, or 512). This setting can also be
 *                               used if SYS_MCLK is a standard multiple of Fs.
 *                               Before this field is set to 0x3 (Use PLL),
 *                               the PLL must be powered up by setting
 *                               CHIP_ANA_POWER->PLL_POWERUP and CHIP_ANA_POWER-
 *                               >VCOAMP_POWERUP. Also, the PLL dividers must be
 *                               calculated based on the external MCLK rate and
 *                               CHIP_PLL_CTRL register must be set (see
 *                               CHIP_PLL_CTRL register description details on
 *                               how to calculate the divisors).
 */
#define SGTL5000_CHIP_CLK_CTRL	0x0004
/*
 * BITS |   FIELD  | RW | RESET | DEFINITION
 * -----|----------|----|-------|-----------------------------------------------
 * 15:9 |   RSVD   | RO |  0x0  | Reserved
 *   8  | SCLKFREQ | RW |  0x0  | Sets frequency of I2S_SCLK when in master mode
 *                                (MS=1). When in slave mode (MS=0), this field
 *                                must be set appropriately to match SCLK input
 *                                rate.
 *                                0x0 = 64Fs
 *                                0x1 = 32Fs - Not supported for RJ mode
 *                                      (I2S_MODE = 1)
 *   7  |    MS    | RW |  0x0  | Configures master or slave of I2S_LRCLK and
 *                                I2S_SCLK.
 *                              0x0 = Slave: I2S_LRCLK and I2S_SCLK are inputs
 *                              0x1 = Master: I2S_LRCLK and I2S_SCLK are outputs
 *                     NOTE: If the PLL is used (CHIP_CLK_CTRL->MCLK_FREQ==0x3),
 *                     the SGTL5000 must be a master of the I2S port (MS==1)
 *   6  | SCLK_INV | RW |  0x0  | Sets the edge that data (input and output) is
 *                                clocked in on for I2S_SCLK
 *                               0x0 = data is valid on rising edge of I2S_SCLK
 *                               0x1 = data is valid on falling edge of I2S_SCLK
 *  5:4 |   DLEN   | RW |  0x1  | I2S data length
 *                                0x0 = 32 bits (only valid when SCLKFREQ=0),
 *                                      not valid for Right Justified Mode
 *                                0x1 = 24 bits (only valid when SCLKFREQ=0)
 *                                0x2 = 20 bits
 *                                0x3 = 16 bits
 *  3:2 | I2S_MODE | RW |  0x0  | Sets the mode for the I2S port
 *                                0x0 = I2S mode or Left Justified
 *                                      (Use LRALIGN to select)
 *                                0x1 = Right Justified Mode
 *                                0x2 = PCM Format A/B
 *                                0x3 = RESERVED
 *   1  | LRALIGN  | RW |  0x0  | I2S_LRCLK Alignment to data word.
 *                                Not used for Right Justified mode
 *                                0x0 = Data word starts 1 I2S_SCLK delay after
 *                                      I2S_LRCLK transition (I2S format, PCM
 *                                      format A)
 *                                0x1 = Data word starts after I2S_LRCLK
 *                                      transition (left justified format, PCM
 *                                      format B)
 *   0  |   LRPOL  | RW |  0x0  | I2S_LRCLK Polarity when data is presented.
 *                                0x0 = I2S_LRCLK = 0 - Left, 1 - Right
 *                                1x0 = I2S_LRCLK = 0 - Right, 1 - Left
 *                                The left subframe should be presented first
 *                                regardless of the setting of LRPOL.
 */
#define SGTL5000_CHIP_I2S_CTRL	0x0006
/*
 * BITS |      FIELD     | RW | RESET | DEFINITION
 * -----|----------------|----|-------|--------------------------------------
 *  15  |      RSVD      | RW |  0x0  |  Reserved
 *  14  | DAP_MIX_LRSWAP | RW |  0x0  | DAP Mixer Input Swap
 *                                      0x0 = Normal Operation
 *                                      0x1 = Left and Right channels for the
 *                                            DAP MIXER Input are swapped.
 *  13  |   DAP_LRSWAP   | RW |  0x0  | DAP Input Swap
 *                                      0x0 = Normal Operation
 *                                      0x1 = Left and Right channels for the
 *                                            DAP Input are swapped
 *  12  |   DAC_LRSWAP   | RW |  0x0  | DAC Input Swap
 *                                      0x0 = Normal Operation
 *                                      0x1 = Left and Right channels for the
 *                                            DAC are swapped
 *  11  |      RSVD      | RW |  0x0  | Reserved
 *  10  |   I2S_LRSWAP   | RW |  0x0  | I2S_DOUT Swap
 *                                      0x0 = Normal Operation
 *                                      0x1 = Left and Right channels for the
 *                                            I2S_DOUT are swapped
 *  9:8 | DAP_MIX_SELECT | RW |  0x0  | Select data source for DAP mixer
 *                                      0x0 = ADC
 *                                      0x1 = I2S_IN
 *                                      0x2 = Reserved
 *                                      0x3 = Reserved
 *  7:6 |   DAP_SELECT   | RW |  0x0  | Select data source for DAP
 *                                      0x0 = ADC
 *                                      0x1 = I2S_IN
 *                                      0x2 = Reserved
 *                                      0x3 = Reserved
 *  5:4 |   DAC_SELECT   | RW |  0x1  | Select data source for DAC
 *                                      0x0 = ADC
 *                                      0x1 = I2S_IN
 *                                      0x2 = Reserved
 *                                      0x3 = DAP
 *  3:2 |      RSVD      | RW |  0x0  | Reserved
 *  1:0 |    I2S_SELECT  | WO |  0x0  | Select data source for I2S_DOUT
 *                                      0x0 = ADC
 *                                      0x1 = I2S_IN
 *                                      0x2 = Reserved
 *                                      0x3 = DAP
 */
#define SGTL5000_CHIP_SSS_CTRL		0x000A
#define SGTL5000_CHIP_ADCDAC_CTRL	0x000E
/*
 * BITS |     FIELD     | RW | RESET | DEFINITION
 * 15:8 | DAC_VOL_RIGHT | RW | 0x3C  | DAC Right Channel Volume
 *                                     Set the Right channel DAC volume with
 *                                     0.5017 dB steps from 0 to -90 dB
 *                                     0x3B and less = Reserved
 *                                     0x3C = 0 dB
 *                                     0x3D = -0.5 dB
 *                                     0xF0 = -90 dB
 *                                     0xFC and greater = Muted
 *                                     If VOL_RAMP_EN = 1, there is an automatic
 *                                     ramp to the new volume setting.
 * 7:0  | DAC_VOL_LEFT  | RW | 0x3C  | DAC Left Channel Volume
 *                                     Set the Left channel DAC volume with
 *                                     0.5017 dB steps from 0 to -90 dB
 *                                     0x3B and less = Reserved
 *                                     0x3C = 0 dB
 *                                     0x3D = -0.5 dB
 *                                     0xF0 = -90 dB
 *                                     0xFC and greater = Muted
 *                                     If VOL_RAMP_EN = 1, there is an automatic
 *                                     ramp to the new volume setting.
 */
#define SGTL5000_CHIP_DAC_VOL		0x0010
#define SGTL5000_CHIP_PAD_STRENGTH	0x0014
/*
 * BITS |      FIELD    | RW | RESET | DEFINITION
 * 15:9 |      RSVD     | RO |  0x0  | Reserved
 *   8  | ADC_VOL_M6DB  | RW |  0x0  | ADC Volume Range Reduction
 *                                     This bit shifts both right and left
 *                                     analog ADC volume range down by 6.0 dB.
 *                                     0x0 = No change in ADC range
 *                                     0x1 = ADC range reduced by 6.0 dB
 *  7:4 | ADC_VOL_RIGHT | RW |  0x0  | ADC Right Channel Volume
 *                                     Right channel analog ADC volume control
 *                                     in 1.5.0 dB steps.
 *                                     0x0 = 0 dB
 *                                     0x1 = +1.5 dB
 *                                     ...
 *                                     0xF = +22.5 dB
 *                                     This range is -6.0 dB to +16.5 dB if
 *                                     ADC_VOL_M6DB is set to 1.
 *  3:0 | ADC_VOL_LEFT  | RW |  0x0  | ADC Left Channel Volume
 *                                     Left channel analog ADC volume control
 *                                     in 1.5 dB steps.
 *                                     0x0 = 0 dB
 *                                     0x1 = +1.5 dB
 *                                     ...
 *                                     0xF = +22.5 dB
 *                                     This range is -6.0 dB to +16.5 dB if
 *                                     ADC_VOL_M6DB is set to 1.
 */
#define SGTL5000_CHIP_ANA_ADC_CTRL	0x0020
/*
 * BITS |     FIELD    | RW | RESET | DEFINITION
 *  15  |     RSVD     | RO |  0x0  | Reserved
 * 14:8 | HP_VOL_RIGHT | RW |  0x18 | Headphone Right Channel Volume
 *                                    Right channel headphone volume control
 *                                    with 0.5 dB steps.
 *                                    0x00 = +12 dB
 *                                    0x01 = +11.5 dB
 *                                    0x18 = 0 dB
 *                                    ...
 *                                    0x7F = -51.5 dB
 *   7  |      RSVD    | RO |  0x0  | Reserved
 *  6:0 |  HP_VOL_LEFT | RW |  0x18 | Headphone Left Channel Volume
 *                                    Left channel headphone volume control
 *                                    with 0.5 dB steps.
 *                                    0x00 = +12 dB
 *                                    0x01 = +11.5 dB
 *                                    0x18 = 0 dB
 *                                    ...
 *                                    0x7F = -51.5 dB
 */
#define SGTL5000_CHIP_ANA_HP_CTRL		0x0022
#define SGTL5000_CHIP_ANA_CTRL			0x0024
#define SGTL5000_CHIP_LINREG_CTRL		0x0026
#define SGTL5000_CHIP_REF_CTRL			0x0028
#define SGTL5000_CHIP_MIC_CTRL			0x002A
#define SGTL5000_CHIP_LINE_OUT_CTRL		0x002C
#define SGTL5000_CHIP_LINE_OUT_VOL		0x002E
#define SGTL5000_CHIP_ANA_POWER			0x0030
#define SGTL5000_CHIP_PLL_CTRL			0x0032
#define SGTL5000_CHIP_CLK_TOP_CTRL		0x0034
#define SGTL5000_CHIP_ANA_STATUS		0x0036
#define SGTL5000_CHIP_ANA_TEST1			0x0038
#define SGTL5000_CHIP_ANA_TEST2			0x003A
#define SGTL5000_CHIP_SHORT_CTRL		0x003C
#define SGTL5000_DAP_CTRL			0x0100
#define SGTL5000_DAP_PEQ			0x0102
#define SGTL5000_DAP_BASS_ENHANCE		0x0104
#define SGTL5000_DAP_BASS_ENHANCE_CTRL		0x0106
#define SGTL5000_DAP_AUDIO_EQ			0x0108
#define SGTL5000_DAP_SURROUND			0x010A
#define SGTL5000_DAP_FILTER_COEF_ACCESS		0x010C
#define SGTL5000_DAP_COEF_WR_B0_MSB		0x010E
#define SGTL5000_DAP_COEF_WR_B0_LSB		0x0110
#define SGTL5000_DAP_EQ_BASS_BAND0		0x0116 /* 115 Hz  */
#define SGTL5000_DAP_EQ_BASS_BAND1		0x0118 /* 330 Hz  */
#define SGTL5000_DAP_EQ_BASS_BAND2		0x011A /* 990 Hz  */
#define SGTL5000_DAP_EQ_BASS_BAND3		0x011C /* 3000 Hz */
#define SGTL5000_DAP_EQ_BASS_BAND4		0x011E /* 9900 Hz */
#define SGTL5000_DAP_MAIN_CHAN			0x0120
#define SGTL5000_DAP_MIX_CHAN			0x0122
#define SGTL5000_DAP_AVC_CTRL			0x0124
#define SGTL5000_DAP_AVC_THRESHOLD		0x0126
#define SGTL5000_DAP_AVC_ATTACK			0x0128
#define SGTL5000_DAP_AVC_DECAY			0x012A
#define SGTL5000_DAP_COEF_WR_B1_MSB		0x012C
#define SGTL5000_DAP_COEF_WR_B1_LSB		0x012E
#define SGTL5000_DAP_COEF_WR_B2_MSB		0x0130
#define SGTL5000_DAP_COEF_WR_B2_LSB		0x0132
#define SGTL5000_DAP_COEF_WR_A1_MSB		0x0134
#define SGTL5000_DAP_COEF_WR_A1_LSB		0x0136
#define SGTL5000_DAP_COEF_WR_A2_MSB		0x0138
#define SGTL5000_DAP_COEF_WR_A2_LSB		0x013A

/*
 * Field Definitions.
 */

/*
 * SGTL5000_CHIP_ID
 */
#define SGTL5000_PARTID_MASK			0xff00
#define SGTL5000_PARTID_SHIFT			8
#define SGTL5000_PARTID_WIDTH			8
#define SGTL5000_PARTID_PART_ID			0xa0
#define SGTL5000_REVID_MASK			0x00ff
#define SGTL5000_REVID_SHIFT			0
#define SGTL5000_REVID_WIDTH			8

/*
 * SGTL5000_CHIP_DIG_POWER
 */
#define SGTL5000_ADC_EN				0x0040
#define SGTL5000_DAC_EN				0x0020
#define SGTL5000_DAP_POWERUP			0x0010
#define SGTL5000_I2S_OUT_POWERUP		0x0002
#define SGTL5000_I2S_IN_POWERUP			0x0001

/*
 * SGTL5000_CHIP_CLK_CTRL
 */
#define SGTL5000_CHIP_CLK_CTRL_DEFAULT		0x0008
#define SGTL5000_RATE_MODE_MASK			0x0030
#define SGTL5000_RATE_MODE_SHIFT		4
#define SGTL5000_RATE_MODE_WIDTH		2
#define SGTL5000_RATE_MODE_DIV_1		0
#define SGTL5000_RATE_MODE_DIV_2		1
#define SGTL5000_RATE_MODE_DIV_4		2
#define SGTL5000_RATE_MODE_DIV_6		3
#define SGTL5000_SYS_FS_MASK			0x000c
#define SGTL5000_SYS_FS_SHIFT			2
#define SGTL5000_SYS_FS_WIDTH			2
#define SGTL5000_SYS_FS_32k			0x0
#define SGTL5000_SYS_FS_44_1k			0x1
#define SGTL5000_SYS_FS_48k			0x2
#define SGTL5000_SYS_FS_96k			0x3
#define SGTL5000_MCLK_FREQ_MASK			0x0003
#define SGTL5000_MCLK_FREQ_SHIFT		0
#define SGTL5000_MCLK_FREQ_WIDTH		2
#define SGTL5000_MCLK_FREQ_256FS		0x0
#define SGTL5000_MCLK_FREQ_384FS		0x1
#define SGTL5000_MCLK_FREQ_512FS		0x2
#define SGTL5000_MCLK_FREQ_PLL			0x3

/*
 * SGTL5000_CHIP_I2S_CTRL
 */
#define SGTL5000_I2S_SCLKFREQ_MASK		0x0100
#define SGTL5000_I2S_SCLKFREQ_SHIFT		8
#define SGTL5000_I2S_SCLKFREQ_WIDTH		1
#define SGTL5000_I2S_SCLKFREQ_64FS		0x0
#define SGTL5000_I2S_SCLKFREQ_32FS		0x1     /* Not for RJ mode */
#define SGTL5000_I2S_MASTER			0x0080
#define SGTL5000_I2S_SCLK_INV			0x0040
#define SGTL5000_I2S_DLEN_MASK			0x0030
#define SGTL5000_I2S_DLEN_SHIFT			4
#define SGTL5000_I2S_DLEN_WIDTH			2
#define SGTL5000_I2S_DLEN_32			0x0
#define SGTL5000_I2S_DLEN_24			0x1
#define SGTL5000_I2S_DLEN_20			0x2
#define SGTL5000_I2S_DLEN_16			0x3
#define SGTL5000_I2S_MODE_MASK			0x000c
#define SGTL5000_I2S_MODE_SHIFT			2
#define SGTL5000_I2S_MODE_WIDTH			2
#define SGTL5000_I2S_MODE_I2S_LJ		0x0
#define SGTL5000_I2S_MODE_RJ			0x1
#define SGTL5000_I2S_MODE_PCM			0x2
#define SGTL5000_I2S_LRALIGN			0x0002
#define SGTL5000_I2S_LRPOL			0x0001  /* set for which mode */

/*
 * SGTL5000_CHIP_SSS_CTRL
 */
#define SGTL5000_DAP_MIX_LRSWAP			0x4000
#define SGTL5000_DAP_LRSWAP			0x2000
#define SGTL5000_DAC_LRSWAP			0x1000
#define SGTL5000_I2S_OUT_LRSWAP			0x0400
#define SGTL5000_DAP_MIX_SEL_MASK		0x0300
#define SGTL5000_DAP_MIX_SEL_SHIFT		8
#define SGTL5000_DAP_MIX_SEL_WIDTH		2
#define SGTL5000_DAP_MIX_SEL_ADC		0x0
#define SGTL5000_DAP_MIX_SEL_I2S_IN		0x1
#define SGTL5000_DAP_SEL_MASK			0x00c0
#define SGTL5000_DAP_SEL_SHIFT			6
#define SGTL5000_DAP_SEL_WIDTH			2
#define SGTL5000_DAP_SEL_ADC			0x0
#define SGTL5000_DAP_SEL_I2S_IN			0x1
#define SGTL5000_DAC_SEL_MASK			0x0030
#define SGTL5000_DAC_SEL_SHIFT			4
#define SGTL5000_DAC_SEL_WIDTH			2
#define SGTL5000_DAC_SEL_ADC			0x0
#define SGTL5000_DAC_SEL_I2S_IN			0x1
#define SGTL5000_DAC_SEL_DAP			0x3
#define SGTL5000_I2S_OUT_SEL_MASK		0x0003
#define SGTL5000_I2S_OUT_SEL_SHIFT		0
#define SGTL5000_I2S_OUT_SEL_WIDTH		2
#define SGTL5000_I2S_OUT_SEL_ADC		0x0
#define SGTL5000_I2S_OUT_SEL_I2S_IN		0x1
#define SGTL5000_I2S_OUT_SEL_DAP		0x3

/*
 * SGTL5000_CHIP_ADCDAC_CTRL
 */
#define SGTL5000_VOL_BUSY_DAC_RIGHT		0x2000
#define SGTL5000_VOL_BUSY_DAC_LEFT		0x1000
#define SGTL5000_DAC_VOL_RAMP_EN		0x0200
#define SGTL5000_DAC_VOL_RAMP_EXPO		0x0100
#define SGTL5000_DAC_MUTE_RIGHT			0x0008
#define SGTL5000_DAC_MUTE_LEFT			0x0004
#define SGTL5000_ADC_HPF_FREEZE			0x0002
#define SGTL5000_ADC_HPF_BYPASS			0x0001

/*
 * SGTL5000_CHIP_DAC_VOL
 */
#define SGTL5000_DAC_VOL_RIGHT_MASK		0xff00
#define SGTL5000_DAC_VOL_RIGHT_SHIFT		8
#define SGTL5000_DAC_VOL_RIGHT_WIDTH		8
#define SGTL5000_DAC_VOL_LEFT_MASK		0x00ff
#define SGTL5000_DAC_VOL_LEFT_SHIFT		0
#define SGTL5000_DAC_VOL_LEFT_WIDTH		8

/*
 * SGTL5000_CHIP_PAD_STRENGTH
 */
#define SGTL5000_PAD_I2S_LRCLK_MASK		0x0300
#define SGTL5000_PAD_I2S_LRCLK_SHIFT		8
#define SGTL5000_PAD_I2S_LRCLK_WIDTH		2
#define SGTL5000_PAD_I2S_SCLK_MASK		0x00c0
#define SGTL5000_PAD_I2S_SCLK_SHIFT		6
#define SGTL5000_PAD_I2S_SCLK_WIDTH		2
#define SGTL5000_PAD_I2S_DOUT_MASK		0x0030
#define SGTL5000_PAD_I2S_DOUT_SHIFT		4
#define SGTL5000_PAD_I2S_DOUT_WIDTH		2
#define SGTL5000_PAD_I2C_SDA_MASK		0x000c
#define SGTL5000_PAD_I2C_SDA_SHIFT		2
#define SGTL5000_PAD_I2C_SDA_WIDTH		2
#define SGTL5000_PAD_I2C_SCL_MASK		0x0003
#define SGTL5000_PAD_I2C_SCL_SHIFT		0
#define SGTL5000_PAD_I2C_SCL_WIDTH		2

/*
 * SGTL5000_CHIP_ANA_ADC_CTRL
 */
#define SGTL5000_ADC_VOL_M6DB			0x0100
#define SGTL5000_ADC_VOL_RIGHT_MASK		0x00f0
#define SGTL5000_ADC_VOL_RIGHT_SHIFT		4
#define SGTL5000_ADC_VOL_RIGHT_WIDTH		4
#define SGTL5000_ADC_VOL_LEFT_MASK		0x000f
#define SGTL5000_ADC_VOL_LEFT_SHIFT		0
#define SGTL5000_ADC_VOL_LEFT_WIDTH		4

/*
 * SGTL5000_CHIP_ANA_HP_CTRL
 */
#define SGTL5000_HP_VOL_RIGHT_MASK		0x7f00
#define SGTL5000_HP_VOL_RIGHT_SHIFT		8
#define SGTL5000_HP_VOL_RIGHT_WIDTH		7
#define SGTL5000_HP_VOL_LEFT_MASK		0x007f
#define SGTL5000_HP_VOL_LEFT_SHIFT		0
#define SGTL5000_HP_VOL_LEFT_WIDTH		7

/*
 * SGTL5000_CHIP_ANA_CTRL
 */
#define SGTL5000_LINE_OUT_MUTE			0x0100
#define SGTL5000_HP_SEL_MASK			0x0040
#define SGTL5000_HP_SEL_SHIFT			6
#define SGTL5000_HP_SEL_WIDTH			1
#define SGTL5000_HP_SEL_DAC			0x0
#define SGTL5000_HP_SEL_LINE_IN			0x1
#define SGTL5000_HP_ZCD_EN			0x0020
#define SGTL5000_HP_MUTE			0x0010
#define SGTL5000_ADC_SEL_MASK			0x0004
#define SGTL5000_ADC_SEL_SHIFT			2
#define SGTL5000_ADC_SEL_WIDTH			1
#define SGTL5000_ADC_SEL_MIC			0x0
#define SGTL5000_ADC_SEL_LINE_IN		0x1
#define SGTL5000_ADC_ZCD_EN			0x0002
#define SGTL5000_ADC_MUTE			0x0001

/*
 * SGTL5000_CHIP_LINREG_CTRL
 */
#define SGTL5000_VDDC_MAN_ASSN_MASK		0x0040
#define SGTL5000_VDDC_MAN_ASSN_SHIFT		6
#define SGTL5000_VDDC_MAN_ASSN_WIDTH		1
#define SGTL5000_VDDC_MAN_ASSN_VDDA		0x0
#define SGTL5000_VDDC_MAN_ASSN_VDDIO		0x1
#define SGTL5000_VDDC_ASSN_OVRD			0x0020
#define SGTL5000_LINREG_VDDD_MASK		0x000f
#define SGTL5000_LINREG_VDDD_SHIFT		0
#define SGTL5000_LINREG_VDDD_WIDTH		4

/*
 * SGTL5000_CHIP_REF_CTRL
 */
#define SGTL5000_ANA_GND_MASK			0x01f0
#define SGTL5000_ANA_GND_SHIFT			4
#define SGTL5000_ANA_GND_WIDTH			5
#define SGTL5000_ANA_GND_BASE			800     /* mv */
#define SGTL5000_ANA_GND_STP			25      /* mv */
#define SGTL5000_BIAS_CTRL_MASK			0x000e
#define SGTL5000_BIAS_CTRL_SHIFT		1
#define SGTL5000_BIAS_CTRL_WIDTH		3
#define SGTL5000_SMALL_POP			1

/*
 * SGTL5000_CHIP_MIC_CTRL
 */
#define SGTL5000_BIAS_R_MASK			0x0300
#define SGTL5000_BIAS_R_SHIFT			8
#define SGTL5000_BIAS_R_WIDTH			2
#define SGTL5000_BIAS_R_off			0x0
#define SGTL5000_BIAS_R_2K			0x1
#define SGTL5000_BIAS_R_4k			0x2
#define SGTL5000_BIAS_R_8k			0x3
#define SGTL5000_BIAS_VOLT_MASK			0x0070
#define SGTL5000_BIAS_VOLT_SHIFT		4
#define SGTL5000_BIAS_VOLT_WIDTH		3
#define SGTL5000_MIC_GAIN_MASK			0x0003
#define SGTL5000_MIC_GAIN_SHIFT			0
#define SGTL5000_MIC_GAIN_WIDTH			2

/*
 * SGTL5000_CHIP_LINE_OUT_CTRL
 */
#define SGTL5000_LINE_OUT_CURRENT_MASK		0x0f00
#define SGTL5000_LINE_OUT_CURRENT_SHIFT		8
#define SGTL5000_LINE_OUT_CURRENT_WIDTH		4
#define SGTL5000_LINE_OUT_CURRENT_180u		0x0
#define SGTL5000_LINE_OUT_CURRENT_270u		0x1
#define SGTL5000_LINE_OUT_CURRENT_360u		0x3
#define SGTL5000_LINE_OUT_CURRENT_450u		0x7
#define SGTL5000_LINE_OUT_CURRENT_540u		0xf
#define SGTL5000_LINE_OUT_GND_MASK		0x003f
#define SGTL5000_LINE_OUT_GND_SHIFT		0
#define SGTL5000_LINE_OUT_GND_WIDTH		6
#define SGTL5000_LINE_OUT_GND_BASE		800     /* mv */
#define SGTL5000_LINE_OUT_GND_STP		25
#define SGTL5000_LINE_OUT_GND_MAX		0x23

/*
 * SGTL5000_CHIP_LINE_OUT_VOL
 */
#define SGTL5000_LINE_OUT_VOL_RIGHT_MASK	0x1f00
#define SGTL5000_LINE_OUT_VOL_RIGHT_SHIFT	8
#define SGTL5000_LINE_OUT_VOL_RIGHT_WIDTH	5
#define SGTL5000_LINE_OUT_VOL_LEFT_MASK		0x001f
#define SGTL5000_LINE_OUT_VOL_LEFT_SHIFT	0
#define SGTL5000_LINE_OUT_VOL_LEFT_WIDTH	5

/*
 * SGTL5000_CHIP_ANA_POWER
 */
#define SGTL5000_ANA_POWER_DEFAULT		0x7060
#define SGTL5000_DAC_STEREO			0x4000
#define SGTL5000_LINREG_SIMPLE_POWERUP		0x2000
#define SGTL5000_STARTUP_POWERUP		0x1000
#define SGTL5000_VDDC_CHRGPMP_POWERUP		0x0800
#define SGTL5000_PLL_POWERUP			0x0400
#define SGTL5000_LINEREG_D_POWERUP		0x0200
#define SGTL5000_VCOAMP_POWERUP			0x0100
#define SGTL5000_VAG_POWERUP			0x0080
#define SGTL5000_ADC_STEREO			0x0040
#define SGTL5000_REFTOP_POWERUP			0x0020
#define SGTL5000_HP_POWERUP			0x0010
#define SGTL5000_DAC_POWERUP			0x0008
#define SGTL5000_CAPLESS_HP_POWERUP		0x0004
#define SGTL5000_ADC_POWERUP			0x0002
#define SGTL5000_LINE_OUT_POWERUP		0x0001

/*
 * SGTL5000_CHIP_PLL_CTRL
 */
#define SGTL5000_PLL_INT_DIV_MASK		0xf800
#define SGTL5000_PLL_INT_DIV_SHIFT		11
#define SGTL5000_PLL_INT_DIV_WIDTH		5
#define SGTL5000_PLL_FRAC_DIV_MASK		0x07ff
#define SGTL5000_PLL_FRAC_DIV_SHIFT		0
#define SGTL5000_PLL_FRAC_DIV_WIDTH		11

/*
 * SGTL5000_CHIP_CLK_TOP_CTRL
 */
#define SGTL5000_INT_OSC_EN			0x0800
#define SGTL5000_INPUT_FREQ_DIV2		0x0008

/*
 * SGTL5000_CHIP_ANA_STATUS
 */
#define SGTL5000_HP_LRSHORT			0x0200
#define SGTL5000_CAPLESS_SHORT			0x0100
#define SGTL5000_PLL_LOCKED			0x0010

/*
 * SGTL5000_CHIP_SHORT_CTRL
 */
#define SGTL5000_LVLADJR_MASK			0x7000
#define SGTL5000_LVLADJR_SHIFT			12
#define SGTL5000_LVLADJR_WIDTH			3
#define SGTL5000_LVLADJL_MASK			0x0700
#define SGTL5000_LVLADJL_SHIFT			8
#define SGTL5000_LVLADJL_WIDTH			3
#define SGTL5000_LVLADJC_MASK			0x0070
#define SGTL5000_LVLADJC_SHIFT			4
#define SGTL5000_LVLADJC_WIDTH			3
#define SGTL5000_LR_SHORT_MOD_MASK		0x000c
#define SGTL5000_LR_SHORT_MOD_SHIFT		2
#define SGTL5000_LR_SHORT_MOD_WIDTH		2
#define SGTL5000_CM_SHORT_MOD_MASK		0x0003
#define SGTL5000_CM_SHORT_MOD_SHIFT		0
#define SGTL5000_CM_SHORT_MOD_WIDTH		2

/*
 *SGTL5000_CHIP_ANA_TEST2
 */
#define SGTL5000_MONO_DAC			0x1000

/*
 * SGTL5000_DAP_CTRL
 */
#define SGTL5000_DAP_MIX_EN			0x0010
#define SGTL5000_DAP_EN				0x0001

#define SGTL5000_SYSCLK				0x00
#define SGTL5000_LRCLK				0x01

/*
 * SGTL5000_DAP_AUDIO_EQ
 */
#define SGTL5000_DAP_SEL_PEQ			1
#define SGTL5000_DAP_SEL_TONE_CTRL		2
#define SGTL5000_DAP_SEL_GEQ			3

#endif
