/*
 * Copyright 2017,2018-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/fsl_serdes.h>

struct serdes_config {
	u32 protocol;
	u8 lanes[SRDS_MAX_LANES];
};


static struct serdes_config serdes1_cfg_tbl[] = {
	/* SerDes 1 */
	{0x130E, {SXGMII1, QXGMII2, NONE, SATA1} },
	{0x199B, {SXGMII1, SGMII1, SGMII2, PCIE1} },
	{0x13BB, {SXGMII1, QXGMII2, PCIE1, PCIE1} },
	{0x13CC, {SXGMII1, QXGMII2, PCIE2, PCIE2} },
	{0x15BB, {SXGMII1, QSGMII_B, PCIE2, PCIE1} },
	{0x15CC, {SXGMII1, QSGMII_B, PCIE2, PCIE2} },
	{0xB3CC, {PCIE1, QXGMII2, PCIE2, PCIE2} },
	{0xB5CC, {PCIE1, QSGMII_B, PCIE2, PCIE2} },
	{0x83BB, {SGMII_T1, QXGMII2, PCIE2, PCIE1} },
	{0x83CC, {SGMII_T1, QXGMII2, PCIE2, PCIE2} },
	{0x85BB, {SGMII_T1, QSGMII_B, PCIE2, PCIE1} },
	{0x85CC, {SGMII_T1, QSGMII_B, PCIE2, PCIE2} },
	{0xB8CC, {PCIE1, SGMII_T1, PCIE2, PCIE2} },
	{0x9999, {SGMII1, SGMII2, SGMII3, SGMII4} },
	{0x85BE, {SGMII_T1, QSGMII_B, PCIE2, SATA1} },
	{0xB8BE, {PCIE1, SGMII_T1, PCIE2, SATA1} },
	{0xCC8E, {PCIE1, PCIE1, SGMII_T1, SATA1} },
	{0xCCBE, {PCIE1, PCIE1, PCIE2, SATA1} },
	{0xCCCC, {PCIE1, PCIE1, PCIE2, PCIE2} },
	{0xDDDD, {PCIE1, PCIE1, PCIE1, PCIE1} },
	{}
};

static struct serdes_config *serdes_cfg_tbl[] = {
	serdes1_cfg_tbl,
};

enum srds_prtcl serdes_get_prtcl(int serdes, int cfg, int lane)
{
	struct serdes_config *ptr;

	if (serdes >= ARRAY_SIZE(serdes_cfg_tbl))
		return 0;

	ptr = serdes_cfg_tbl[serdes];
	while (ptr->protocol) {
		if (ptr->protocol == cfg)
			return ptr->lanes[lane];
		ptr++;
	}

	return 0;
}

int is_serdes_prtcl_valid(int serdes, u32 prtcl)
{
	int i;
	struct serdes_config *ptr;

	if (serdes >= ARRAY_SIZE(serdes_cfg_tbl))
		return 0;

	ptr = serdes_cfg_tbl[serdes];
	while (ptr->protocol) {
		if (ptr->protocol == prtcl)
			break;
		ptr++;
	}

	if (!ptr->protocol)
		return 0;

	for (i = 0; i < SRDS_MAX_LANES; i++) {
		if (ptr->lanes[i] != NONE)
			return 1;
	}

	return 0;
}
