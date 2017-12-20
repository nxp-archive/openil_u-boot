/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/fsl_serdes.h>
#include <asm/arch/soc.h>
#include <fsl-mc/ldpaa_wriop.h>

#ifdef CONFIG_SYS_FSL_SRDS_1
static u8 serdes1_prtcl_map[SERDES_PRCTL_COUNT];
#endif
#ifdef CONFIG_SYS_FSL_SRDS_2
static u8 serdes2_prtcl_map[SERDES_PRCTL_COUNT];
#endif

#if defined(CONFIG_FSL_MC_ENET) && !defined(CONFIG_SPL_BUILD)
int xfi_dpmac[XFI8 + 1];
int sgmii_dpmac[SGMII16 + 1];
#endif

__weak void wriop_init_dpmac_qsgmii(int sd, int lane_prtcl)
{
	return;
}

/*
 *The return value of this func is the serdes protocol used.
 *Typically this function is called number of times depending
 *upon the number of serdes blocks in the Silicon.
 *Zero is used to denote that no serdes was enabled,
 *this is the case when golden RCW was used where DPAA2 bring was
 *intentionally removed to achieve boot to prompt
*/

__weak int serdes_get_number(int serdes, int cfg)
{
	return cfg;
}

int is_serdes_configured(enum srds_prtcl device)
{
	int ret = 0;

#ifdef CONFIG_SYS_FSL_SRDS_1
	if (!serdes1_prtcl_map[NONE])
		fsl_serdes_init();

	ret |= serdes1_prtcl_map[device];
#endif
#ifdef CONFIG_SYS_FSL_SRDS_2
	if (!serdes2_prtcl_map[NONE])
		fsl_serdes_init();

	ret |= serdes2_prtcl_map[device];
#endif

	return !!ret;
}

int serdes_get_first_lane(u32 sd, enum srds_prtcl device)
{
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	u32 cfg = 0;
	int i;

	switch (sd) {
#ifdef CONFIG_SYS_FSL_SRDS_1
	case FSL_SRDS_1:
		cfg = gur_in32(&gur->rcwsr[FSL_CHASSIS3_SRDS1_REGSR - 1]);
		cfg &= FSL_CHASSIS3_SRDS1_PRTCL_MASK;
		cfg >>= FSL_CHASSIS3_SRDS1_PRTCL_SHIFT;
		break;
#endif
#ifdef CONFIG_SYS_FSL_SRDS_2
	case FSL_SRDS_2:
		cfg = gur_in32(&gur->rcwsr[FSL_CHASSIS3_SRDS2_REGSR - 1]);
		cfg &= FSL_CHASSIS3_SRDS2_PRTCL_MASK;
		cfg >>= FSL_CHASSIS3_SRDS2_PRTCL_SHIFT;
		break;
#endif
	default:
		printf("invalid SerDes%d\n", sd);
		break;
	}

	cfg = serdes_get_number(sd, cfg);

	/* Is serdes enabled at all? */
	if (cfg == 0)
		return -ENODEV;

	for (i = 0; i < SRDS_MAX_LANES; i++) {
		if (serdes_get_prtcl(sd, cfg, i) == device)
			return i;
	}

	return -ENODEV;
}

void serdes_init(u32 sd, u32 sd_addr, u32 rcwsr, u32 sd_prctl_mask,
		 u32 sd_prctl_shift, u8 serdes_prtcl_map[SERDES_PRCTL_COUNT])
{
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	u32 cfg;
	int lane;

	if (serdes_prtcl_map[NONE])
		return;

	memset(serdes_prtcl_map, 0, sizeof(u8) * SERDES_PRCTL_COUNT);

	cfg = gur_in32(&gur->rcwsr[rcwsr - 1]) & sd_prctl_mask;
	cfg >>= sd_prctl_shift;

	cfg = serdes_get_number(sd, cfg);
	printf("Using SERDES%d Protocol: %d (0x%x)\n", sd + 1, cfg, cfg);

	if (!is_serdes_prtcl_valid(sd, cfg))
		printf("SERDES%d[PRTCL] = 0x%x is not valid\n", sd + 1, cfg);

	for (lane = 0; lane < SRDS_MAX_LANES; lane++) {
		enum srds_prtcl lane_prtcl = serdes_get_prtcl(sd, cfg, lane);
		if (unlikely(lane_prtcl >= SERDES_PRCTL_COUNT))
			debug("Unknown SerDes lane protocol %d\n", lane_prtcl);
		else {
			serdes_prtcl_map[lane_prtcl] = 1;
#if defined(CONFIG_FSL_MC_ENET) && !defined(CONFIG_SPL_BUILD)
			switch (lane_prtcl) {
			case QSGMII_A:
			case QSGMII_B:
			case QSGMII_C:
			case QSGMII_D:
				wriop_init_dpmac_qsgmii(sd, (int)lane_prtcl);
				break;
			default:
				if (lane_prtcl >= XFI1 && lane_prtcl <= XFI8)
					wriop_init_dpmac(sd,
							 xfi_dpmac[lane_prtcl],
							 (int)lane_prtcl);

				 if (lane_prtcl >= SGMII1 &&
				     lane_prtcl <= SGMII16)
					wriop_init_dpmac(sd, sgmii_dpmac[
							 lane_prtcl],
							 (int)lane_prtcl);
				break;
			}
#endif
		}
	}

	/* Set the first element to indicate serdes has been initialized */
	serdes_prtcl_map[NONE] = 1;
}

__weak int get_serdes_volt(void)
{
	return -1;
}

__weak int set_serdes_volt(int svdd)
{
	return -1;
}

int setup_serdes_volt(u32 svdd)
{
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	struct ccsr_serdes __iomem *serdes1_base;
	u32 cfg_rcwsrds1 = gur_in32(&gur->rcwsr[FSL_CHASSIS3_SRDS1_REGSR - 1]);
#ifdef CONFIG_SYS_FSL_SRDS_2
	struct ccsr_serdes __iomem *serdes2_base;
	u32 cfg_rcwsrds2 = gur_in32(&gur->rcwsr[FSL_CHASSIS3_SRDS2_REGSR - 1]);
#endif
	u32 cfg_tmp, reg = 0;
	int svdd_cur, svdd_tar;
	int ret = 1;
	int i;

	/* Only support switch SVDD to 900mV */
	if (svdd != 900)
		return -1;

	/* Scale up to the LTC resolution is 1/4096V */
	svdd = (svdd * 4096) / 1000;

	svdd_tar = svdd;
	svdd_cur = get_serdes_volt();
	if (svdd_cur < 0)
		return -EINVAL;

	debug("%s: current SVDD: %x; target SVDD: %x\n",
	      __func__, svdd_cur, svdd_tar);
	if (svdd_cur == svdd_tar)
		return 0;

	serdes1_base = (void *)CONFIG_SYS_FSL_LSCH3_SERDES_ADDR;
#ifdef CONFIG_SYS_FSL_SRDS_2
	serdes2_base =  (void *)(CONFIG_SYS_FSL_LSCH3_SERDES_ADDR + 0x10000);
#endif

	/* Put the all enabled lanes in reset */
#ifdef CONFIG_SYS_FSL_SRDS_1
	cfg_tmp = cfg_rcwsrds1 & FSL_CHASSIS3_SRDS1_PRTCL_MASK;
	cfg_tmp >>= FSL_CHASSIS3_SRDS1_PRTCL_SHIFT;

	for (i = 0; i < 4 && cfg_tmp & (0xf << (3 - i)); i++) {
		reg = in_le32(&serdes1_base->lane[i].gcr0);
		reg &= 0xFF9FFFFF;
		out_le32(&serdes1_base->lane[i].gcr0, reg);
	}
#endif

#ifdef CONFIG_SYS_FSL_SRDS_2
	cfg_tmp = cfg_rcwsrds2 & FSL_CHASSIS3_SRDS2_PRTCL_MASK;
	cfg_tmp >>= FSL_CHASSIS3_SRDS2_PRTCL_SHIFT;

	for (i = 0; i < 4 && cfg_tmp & (0xf << (3 - i)); i++) {
		reg = in_le32(&serdes2_base->lane[i].gcr0);
		reg &= 0xFF9FFFFF;
		out_le32(&serdes2_base->lane[i].gcr0, reg);
	}
#endif

	/* Put the all enabled PLL in reset */
#ifdef CONFIG_SYS_FSL_SRDS_1
	cfg_tmp = cfg_rcwsrds1 & 0x3;
	for (i = 0; i < 2 && !(cfg_tmp & (0x1 << (1 - i))); i++) {
		reg = in_le32(&serdes1_base->bank[i].rstctl);
		reg &= 0xFFFFFFBF;
		reg |= 0x10000000;
		out_le32(&serdes1_base->bank[i].rstctl, reg);
	}
	udelay(1);

	reg = in_le32(&serdes1_base->bank[i].rstctl);
	reg &= 0xFFFFFF1F;
	out_le32(&serdes1_base->bank[i].rstctl, reg);
#endif

#ifdef CONFIG_SYS_FSL_SRDS_2
	cfg_tmp = cfg_rcwsrds1 & 0xC;
	cfg_tmp >>= 2;
	for (i = 0; i < 2 && !(cfg_tmp & (0x1 << (1 - i))); i++) {
		reg = in_le32(&serdes2_base->bank[i].rstctl);
		reg &= 0xFFFFFFBF;
		reg |= 0x10000000;
		out_le32(&serdes2_base->bank[i].rstctl, reg);
	}
	udelay(1);

	reg = in_le32(&serdes2_base->bank[i].rstctl);
	reg &= 0xFFFFFF1F;
	out_le32(&serdes2_base->bank[i].rstctl, reg);
#endif

	/* Put the Rx/Tx calibration into reset */
#ifdef CONFIG_SYS_FSL_SRDS_1
	reg = in_le32(&serdes1_base->srdstcalcr);
	reg &= 0xF7FFFFFF;
	out_le32(&serdes1_base->srdstcalcr, reg);
	reg = in_le32(&serdes1_base->srdsrcalcr);
	reg &= 0xF7FFFFFF;
	out_le32(&serdes1_base->srdsrcalcr, reg);
#endif

#ifdef CONFIG_SYS_FSL_SRDS_2
	reg = in_le32(&serdes2_base->srdstcalcr);
	reg &= 0xF7FFFFFF;
	out_le32(&serdes2_base->srdstcalcr, reg);
	reg = in_le32(&serdes2_base->srdsrcalcr);
	reg &= 0xF7FFFFFF;
	out_le32(&serdes2_base->srdsrcalcr, reg);
#endif

	ret = set_serdes_volt(svdd);
	if (ret < 0) {
		printf("could not change SVDD\n");
		ret = -1;
	}

	/* For each PLL that’s not disabled via RCW enable the SERDES */
#ifdef CONFIG_SYS_FSL_SRDS_1
	cfg_tmp = cfg_rcwsrds1 & 0x3;
	for (i = 0; i < 2 && !(cfg_tmp & (0x1 << (1 - i))); i++) {
		reg = in_le32(&serdes1_base->bank[i].rstctl);
		reg |= 0x00000020;
		out_le32(&serdes1_base->bank[i].rstctl, reg);
		udelay(1);

		reg = in_le32(&serdes1_base->bank[i].rstctl);
		reg |= 0x00000080;
		out_le32(&serdes1_base->bank[i].rstctl, reg);
		udelay(1);
		/* Take the Rx/Tx calibration out of reset */
		if (!(cfg_tmp == 0x3 && i == 1)) {
			udelay(1);
			reg = in_le32(&serdes1_base->srdstcalcr);
			reg |= 0x08000000;
			out_le32(&serdes1_base->srdstcalcr, reg);
			reg = in_le32(&serdes1_base->srdsrcalcr);
			reg |= 0x08000000;
			out_le32(&serdes1_base->srdsrcalcr, reg);
		}
		udelay(1);
	}
#endif
#ifdef CONFIG_SYS_FSL_SRDS_2
	cfg_tmp = cfg_rcwsrds1 & 0xC;
	cfg_tmp >>= 2;
	for (i = 0; i < 2 && !(cfg_tmp & (0x1 << (1 - i))); i++) {
		reg = in_le32(&serdes2_base->bank[i].rstctl);
		reg |= 0x00000020;
		out_le32(&serdes2_base->bank[i].rstctl, reg);
		udelay(1);

		reg = in_le32(&serdes2_base->bank[i].rstctl);
		reg |= 0x00000080;
		out_le32(&serdes2_base->bank[i].rstctl, reg);
		udelay(1);

		/* Take the Rx/Tx calibration out of reset */
		if (!(cfg_tmp == 0x3 && i == 1)) {
			udelay(1);
			reg = in_le32(&serdes2_base->srdstcalcr);
			reg |= 0x08000000;
			out_le32(&serdes2_base->srdstcalcr, reg);
			reg = in_le32(&serdes2_base->srdsrcalcr);
			reg |= 0x08000000;
			out_le32(&serdes2_base->srdsrcalcr, reg);
		}
		udelay(1);
	}
#endif

	/* Wait for at atleast 625us, ensure the PLLs being reset are locked */
	udelay(800);

#ifdef CONFIG_SYS_FSL_SRDS_1
	cfg_tmp = cfg_rcwsrds1 & 0x3;
	for (i = 0; i < 2 && !(cfg_tmp & (0x1 << (1 - i))); i++) {
		/* if the PLL is not locked, set RST_ERR */
		reg = in_le32(&serdes1_base->bank[i].pllcr0);
		if (!((reg >> 23) & 0x1)) {
			reg = in_le32(&serdes1_base->bank[i].rstctl);
			reg |= 0x20000000;
			out_le32(&serdes1_base->bank[i].rstctl, reg);
		} else {
			udelay(1);
			reg = in_le32(&serdes1_base->bank[i].rstctl);
			reg &= 0xFFFFFFEF;
			reg |= 0x00000040;
			out_le32(&serdes1_base->bank[i].rstctl, reg);
			udelay(1);
		}
	}
#endif

#ifdef CONFIG_SYS_FSL_SRDS_2
	cfg_tmp = cfg_rcwsrds1 & 0xC;
	cfg_tmp >>= 2;

	for (i = 0; i < 2 && !(cfg_tmp & (0x1 << (1 - i))); i++) {
		reg = in_le32(&serdes2_base->bank[i].pllcr0);
		if (!((reg >> 23) & 0x1)) {
			reg = in_le32(&serdes2_base->bank[i].rstctl);
			reg |= 0x20000000;
			out_le32(&serdes2_base->bank[i].rstctl, reg);
		} else {
			udelay(1);
			reg = in_le32(&serdes2_base->bank[i].rstctl);
			reg &= 0xFFFFFFEF;
			reg |= 0x00000040;
			out_le32(&serdes2_base->bank[i].rstctl, reg);
			udelay(1);
		}
	}
#endif
	/* Take the all enabled lanes out of reset */
#ifdef CONFIG_SYS_FSL_SRDS_1
	cfg_tmp = cfg_rcwsrds1 & FSL_CHASSIS3_SRDS1_PRTCL_MASK;
	cfg_tmp >>= FSL_CHASSIS3_SRDS1_PRTCL_SHIFT;

	for (i = 0; i < 4 && cfg_tmp & (0xf << (3 - i)); i++) {
		reg = in_le32(&serdes1_base->lane[i].gcr0);
		reg |= 0x00600000;
		out_le32(&serdes1_base->lane[i].gcr0, reg);
	}
#endif
#ifdef CONFIG_SYS_FSL_SRDS_2
	cfg_tmp = cfg_rcwsrds2 & FSL_CHASSIS3_SRDS2_PRTCL_MASK;
	cfg_tmp >>= FSL_CHASSIS3_SRDS2_PRTCL_SHIFT;

	for (i = 0; i < 4 && cfg_tmp & (0xf << (3 - i)); i++) {
		reg = in_le32(&serdes2_base->lane[i].gcr0);
		reg |= 0x00600000;
		out_le32(&serdes2_base->lane[i].gcr0, reg);
	}
#endif

	/* For each PLL being reset, and achieved PLL lock set RST_DONE */
#ifdef CONFIG_SYS_FSL_SRDS_1
	cfg_tmp = cfg_rcwsrds1 & 0x3;
	for (i = 0; i < 2; i++) {
		reg = in_le32(&serdes1_base->bank[i].pllcr0);
		if (!(cfg_tmp & (0x1 << (1 - i))) && ((reg >> 23) & 0x1)) {
			reg = in_le32(&serdes1_base->bank[i].rstctl);
			reg |= 0x40000000;
			out_le32(&serdes1_base->bank[i].rstctl, reg);
		}
	}
#endif
#ifdef CONFIG_SYS_FSL_SRDS_2
	cfg_tmp = cfg_rcwsrds1 & 0xC;
	cfg_tmp >>= 2;

	for (i = 0; i < 2; i++) {
		reg = in_le32(&serdes2_base->bank[i].pllcr0);
		if (!(cfg_tmp & (0x1 << (1 - i))) && ((reg >> 23) & 0x1)) {
			reg = in_le32(&serdes2_base->bank[i].rstctl);
			reg |= 0x40000000;
			out_le32(&serdes2_base->bank[i].rstctl, reg);
		}
	}
#endif

	return ret;
}

void fsl_serdes_init(void)
{
#if defined(CONFIG_FSL_MC_ENET) && !defined(CONFIG_SPL_BUILD)
	int i , j;

	for (i = XFI1, j = 1; i <= XFI8; i++, j++)
		xfi_dpmac[i] = j;

	for (i = SGMII1, j = 1; i <= SGMII16; i++, j++)
		sgmii_dpmac[i] = j;
#endif

#ifdef CONFIG_SYS_FSL_SRDS_1
	serdes_init(FSL_SRDS_1,
		    CONFIG_SYS_FSL_LSCH3_SERDES_ADDR,
		    FSL_CHASSIS3_SRDS1_REGSR,
		    FSL_CHASSIS3_SRDS1_PRTCL_MASK,
		    FSL_CHASSIS3_SRDS1_PRTCL_SHIFT,
		    serdes1_prtcl_map);
#endif
#ifdef CONFIG_SYS_FSL_SRDS_2
	serdes_init(FSL_SRDS_2,
		    CONFIG_SYS_FSL_LSCH3_SERDES_ADDR + FSL_SRDS_2 * 0x10000,
		    FSL_CHASSIS3_SRDS2_REGSR,
		    FSL_CHASSIS3_SRDS2_PRTCL_MASK,
		    FSL_CHASSIS3_SRDS2_PRTCL_SHIFT,
		    serdes2_prtcl_map);
#endif
}
