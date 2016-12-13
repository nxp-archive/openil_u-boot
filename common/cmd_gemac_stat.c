/*
* Copyright (C) 2016 Freescale Semiconductor Inc.
*
* SPDX-License-Identifier:GPL-2.0+
*/

/**
 * @file
 * @brief Retrieve GEMAC Statistics
 */

#include <common.h>
#include <command.h>
#include "../drivers/net/pfe_eth/pfe_eth.h"
#include "../drivers/net/pfe_eth/pfe/pfe.h"
#include "../drivers/net/pfe_eth/pfe_firmware.h"
#include "../drivers/net/pfe_eth/pfe/cbus.h"
#include "../drivers/net/pfe_eth/pfe/cbus/class_csr.h"
#include "../drivers/net/pfe_eth/pfe/cbus/emac.h"

#define ETH_GSTRING_LEN         32 /* from linux/include/ethtool.h */

static const struct fec_stat {
	char name[ETH_GSTRING_LEN];
	u16 offset;
} fec_stats[] = {
	/* RMON TX */
	{ "tx_dropped", RMON_T_DROP },
	{ "tx_packets", RMON_T_PACKETS },
	{ "tx_broadcast", RMON_T_BC_PKT },
	{ "tx_multicast", RMON_T_MC_PKT },
	{ "tx_crc_errors", RMON_T_CRC_ALIGN },
	{ "tx_undersize", RMON_T_UNDERSIZE },
	{ "tx_oversize", RMON_T_OVERSIZE },
	{ "tx_fragment", RMON_T_FRAG },
	{ "tx_jabber", RMON_T_JAB },
	{ "tx_collision", RMON_T_COL },
	{ "tx_64byte", RMON_T_P64 },
	{ "tx_65to127byte", RMON_T_P65TO127 },
	{ "tx_128to255byte", RMON_T_P128TO255 },
	{ "tx_256to511byte", RMON_T_P256TO511 },
	{ "tx_512to1023byte", RMON_T_P512TO1023 },
	{ "tx_1024to2047byte", RMON_T_P1024TO2047 },
	{ "tx_GTE2048byte", RMON_T_P_GTE2048 },
	{ "tx_octets", RMON_T_OCTETS },

	/* IEEE TX */
	{ "IEEE_tx_drop", IEEE_T_DROP },
	{ "IEEE_tx_frame_ok", IEEE_T_FRAME_OK },
	{ "IEEE_tx_1col", IEEE_T_1COL },
	{ "IEEE_tx_mcol", IEEE_T_MCOL },
	{ "IEEE_tx_def", IEEE_T_DEF },
	{ "IEEE_tx_lcol", IEEE_T_LCOL },
	{ "IEEE_tx_excol", IEEE_T_EXCOL },
	{ "IEEE_tx_macerr", IEEE_T_MACERR },
	{ "IEEE_tx_cserr", IEEE_T_CSERR },
	{ "IEEE_tx_sqe", IEEE_T_SQE },
	{ "IEEE_tx_fdxfc", IEEE_T_FDXFC },
	{ "IEEE_tx_octets_ok", IEEE_T_OCTETS_OK },

	/* RMON RX */
	{ "rx_packets", RMON_R_PACKETS },
	{ "rx_broadcast", RMON_R_BC_PKT },
	{ "rx_multicast", RMON_R_MC_PKT },
	{ "rx_crc_errors", RMON_R_CRC_ALIGN },
	{ "rx_undersize", RMON_R_UNDERSIZE },
	{ "rx_oversize", RMON_R_OVERSIZE },
	{ "rx_fragment", RMON_R_FRAG },
	{ "rx_jabber", RMON_R_JAB },
	{ "rx_64byte", RMON_R_P64 },
	{ "rx_65to127byte", RMON_R_P65TO127 },
	{ "rx_128to255byte", RMON_R_P128TO255 },
	{ "rx_256to511byte", RMON_R_P256TO511 },
	{ "rx_512to1023byte", RMON_R_P512TO1023 },
	{ "rx_1024to2047byte", RMON_R_P1024TO2047 },
	{ "rx_GTE2048byte", RMON_R_P_GTE2048 },
	{ "rx_octets", RMON_R_OCTETS },

	/* IEEE RX */
	{ "IEEE_rx_drop", IEEE_R_DROP },
	{ "IEEE_rx_frame_ok", IEEE_R_FRAME_OK },
	{ "IEEE_rx_crc", IEEE_R_CRC },
	{ "IEEE_rx_align", IEEE_R_ALIGN },
	{ "IEEE_rx_macerr", IEEE_R_MACERR },
	{ "IEEE_rx_fdxfc", IEEE_R_FDXFC },
	{ "IEEE_rx_octets_ok", IEEE_R_OCTETS_OK },
};

static void ls1012a_emac_print_stats(void *base)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fec_stats); i++)
		printf("%s: %d\n", fec_stats[i].name, readl(base + fec_stats[i].offset));
}

static int gemac_stats(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	void *gemac_base = NULL;

	if (argc != 2) {
		printf("Usage: \n" "gemac_stat [ethx]\n");
		return CMD_RET_SUCCESS;
	}

	if ( strcmp(argv[1], "eth0") == 0)
		gemac_base = (void *)EMAC1_BASE_ADDR;
	else if ( strcmp(argv[1], "eth1") == 0)
		gemac_base = (void *)EMAC2_BASE_ADDR;

	if (gemac_base)
	{
		ls1012a_emac_print_stats(gemac_base);
	}
	else
	{
		printf("no such net device: %s\n", argv[1]);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	gemac_stat,	2,	1,	gemac_stats,
	"retrieve GEMAC statistics",
	"Usage: \n"
	"gemac_stat [ethx]\n"
);
