/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <netdev.h>
#include <fm_eth.h>
#include <fsl_mdio.h>
#include <malloc.h>
#include <fsl_dtsec.h>
#include <asm/arch/soc.h>
#include <asm/arch-fsl-layerscape/config.h>
#include <asm/arch/fsl_serdes.h>

#include "../common/qixis.h"
#include "../../../drivers/net/pfe_eth/pfe_eth.h"
#include "ls1012aqds_qixis.h"
#include <asm/arch-fsl-layerscape/immap_lsch2.h>

#define EMI_NONE	0xFF
#define EMI1_RGMII	1
#define EMI1_SLOT1	2
#define EMI1_SLOT2	3

#define DEFAULT_PFE_MDIO_NAME "PFE_MDIO"

static const char * const mdio_names[] = {
	"NULL",
	"LS1012AQDS_MDIO_RGMII",
	"LS1012AQDS_MDIO_SLOT1",
	"LS1012AQDS_MDIO_SLOT2",
	"NULL",
};

static const char *ls1012aqds_mdio_name_for_muxval(u8 muxval)
{
	return mdio_names[muxval];
}

struct ls1012aqds_mdio {
	u8 muxval;
	struct mii_dev *realbus;
};

static void ls1012aqds_mux_mdio(u8 muxval)
{
	u8 brdcfg4;

	if (muxval < 7) {
		brdcfg4 = QIXIS_READ(brdcfg[4]);
		brdcfg4 &= ~BRDCFG4_EMISEL_MASK;
		brdcfg4 |= (muxval << BRDCFG4_EMISEL_SHIFT);
		QIXIS_WRITE(brdcfg[4], brdcfg4);
	}
}

static int ls1012aqds_mdio_read(struct mii_dev *bus, int addr, int devad,
			      int regnum)
{
	struct ls1012aqds_mdio *priv = bus->priv;

	ls1012aqds_mux_mdio(priv->muxval);

	return priv->realbus->read(priv->realbus, addr, devad, regnum);
}

static int ls1012aqds_mdio_write(struct mii_dev *bus, int addr, int devad,
			       int regnum, u16 value)
{
	struct ls1012aqds_mdio *priv = bus->priv;

	ls1012aqds_mux_mdio(priv->muxval);

	return priv->realbus->write(priv->realbus, addr, devad, regnum, value);
}

static int ls1012aqds_mdio_reset(struct mii_dev *bus)
{
	struct ls1012aqds_mdio *priv = bus->priv;

	if(priv->realbus->reset)
		return priv->realbus->reset(priv->realbus);
	else
		return -1;
}

static int ls1012aqds_mdio_init(char *realbusname, u8 muxval)
{
	struct ls1012aqds_mdio *pmdio;
	struct mii_dev *bus = mdio_alloc();

	if (!bus) {
		printf("Failed to allocate ls1012aqds MDIO bus\n");
		return -1;
	}

	pmdio = malloc(sizeof(*pmdio));
	if (!pmdio) {
		printf("Failed to allocate ls1012aqds private data\n");
		free(bus);
		return -1;
	}

	bus->read = ls1012aqds_mdio_read;
	bus->write = ls1012aqds_mdio_write;
	bus->reset = ls1012aqds_mdio_reset;
	sprintf(bus->name, ls1012aqds_mdio_name_for_muxval(muxval));

	pmdio->realbus = miiphy_get_dev_by_name(realbusname);

	if (!pmdio->realbus) {
		printf("No bus with name %s\n", realbusname);
		free(bus);
		free(pmdio);
		return -1;
	}

	pmdio->muxval = muxval;
	bus->priv = pmdio;
	return mdio_register(bus);
}

int board_eth_init(bd_t *bis)
{
#ifdef CONFIG_FSL_PPFE
        struct mii_dev *bus;
	struct mdio_info mac1_mdio_info;
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;
	u8 data8;


	/*TODO Following config should be done for all boards, where is the right place to put this */
	out_be32(&scfg->pfeasbcr, in_be32(&scfg->pfeasbcr) | SCFG_PPFEASBCR_AWCACHE0);
	out_be32(&scfg->pfebsbcr, in_be32(&scfg->pfebsbcr) | SCFG_PPFEASBCR_AWCACHE0);

	/*CCI-400 QoS settings for PFE */
	out_be32(&scfg->wr_qos1, 0x0ff00000);
	out_be32(&scfg->rd_qos1, 0x0ff00000);

	/* Set RGMII into 1G + Full duplex mode */
	out_be32(&scfg->rgmiipcr, in_be32(&scfg->rgmiipcr) | (SCFG_RGMIIPCR_SETSP_1000M | SCFG_RGMIIPCR_SETFD));

	out_be32((CONFIG_SYS_DCSR_DCFG_ADDR + 0x520), 0xFFFFFFFF);
	out_be32((CONFIG_SYS_DCSR_DCFG_ADDR + 0x524), 0xFFFFFFFF);

	ls1012aqds_mux_mdio(2);

#ifdef RGMII_RESET_WA
	/* Work around for FPGA registers initialization
	 * This is needed for RGMII to work */
	printf("Reset RGMII WA....\n");
	data8 = QIXIS_READ(rst_frc[0]);
	data8 |= 0x2;
	QIXIS_WRITE(rst_frc[0], data8);
	data8 = QIXIS_READ(rst_frc[0]);
#endif

	mac1_mdio_info.reg_base = (void *)0x04200000; /*EMAC1_BASE_ADDR*/
	mac1_mdio_info.name = DEFAULT_PFE_MDIO_NAME;

	bus = ls1012a_mdio_init(&mac1_mdio_info);
	if(!bus)
	{
		printf("Failed to register mdio \n");
		return -1;
	}

	/*Based on RCW config initialize correctly */
	/*MAC2 */
	if(ls1012aqds_mdio_init(DEFAULT_PFE_MDIO_NAME, EMI1_RGMII) < 0)
	{
		printf("Failed to register mdio for %s\n", ls1012aqds_mdio_name_for_muxval(EMI1_RGMII));
		return -1;
	}
	ls1012a_set_mdio(1, miiphy_get_dev_by_name(ls1012aqds_mdio_name_for_muxval(EMI1_RGMII)));
	ls1012a_set_phy_address_mode(1,  EMAC2_PHY_ADDR, PHY_INTERFACE_MODE_RGMII);

	/*MAC1 */
	if(ls1012aqds_mdio_init(DEFAULT_PFE_MDIO_NAME, EMI1_SLOT1) < 0)
	{
		printf("Failed to register mdio for %s\n", ls1012aqds_mdio_name_for_muxval(EMI1_SLOT1));
		return -1;
	}
	ls1012a_set_mdio(0, miiphy_get_dev_by_name(ls1012aqds_mdio_name_for_muxval(EMI1_SLOT1)));
	ls1012a_set_phy_address_mode(0,  EMAC1_PHY_ADDR, PHY_INTERFACE_MODE_SGMII);

	cpu_eth_init(bis);
#endif
	return pci_eth_init(bis);
}
