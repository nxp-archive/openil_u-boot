/*
 * Copyright 2015,2018-2019 NXP 
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <netdev.h>
#include <fdt_support.h>
#include <fm_eth.h>
#include <fsl_mdio.h>
#include <fsl_dtsec.h>
#include <linux/libfdt.h>
#include <malloc.h>
#include <asm/arch/fsl_serdes.h>

#include "../common/qixis.h"
#include "../common/fman.h"
#include "ls1043aqds_qixis.h"

#define EMI_NONE	0xFF
#define EMI1_RGMII1	0
#define EMI1_RGMII2	1
#define EMI1_SLOT1	4
#define EMI1_SLOT2	5
#define EMI1_SLOT3	6
#define EMI1_SLOT4	7
#define EMI2		8

static int mdio_mux[NUM_FM_PORTS];

static const char * const mdio_names[] = {
	"LS1028AQDS_MDIO_RGMII1",
	"LS1028AQDS_MDIO_RGMII2",
	"NULL",
	"NULL",
	"LS1028AQDS_MDIO_SLOT1",
	"LS1028AQDS_MDIO_SLOT2",
	"LS1028AQDS_MDIO_SLOT3",
	"LS1028AQDS_MDIO_SLOT4",
	"NULL",
};

/* Map SerDes1 4 lanes to default slot, will be initialized dynamically */
static u8 lane_to_slot[] = {0, 0, 0, 0};

static const char *LS1028AQDS_mdio_name_for_muxval(u8 muxval)
{
	return mdio_names[muxval];
}

struct mii_dev *mii_dev_for_muxval(u8 muxval)
{
	struct mii_dev *bus;
	const char *name;

	if (muxval > EMI2)
		return NULL;

	name = LS1028AQDS_mdio_name_for_muxval(muxval);

	if (!name) {
		printf("No bus for muxval %x\n", muxval);
		return NULL;
	}

	bus = miiphy_get_dev_by_name(name);

	if (!bus) {
		printf("No bus by name %s\n", name);
		return NULL;
	}

	return bus;
}

struct LS1028AQDS_mdio {
	u8 muxval;
	struct mii_dev *realbus;
};

static void LS1028AQDS_mux_mdio(u8 muxval)
{
	u8 brdcfg4;

	if (muxval < 7) {
		brdcfg4 = QIXIS_READ(brdcfg[4]);
		brdcfg4 &= ~BRDCFG4_EMISEL_MASK;
		brdcfg4 |= (muxval << BRDCFG4_EMISEL_SHIFT);
		QIXIS_WRITE(brdcfg[4], brdcfg4);
	}
}

static int LS1028AQDS_mdio_read(struct mii_dev *bus, int addr, int devad,
			      int regnum)
{
	struct LS1028AQDS_mdio *priv = bus->priv;

	LS1028AQDS_mux_mdio(priv->muxval);

	return priv->realbus->read(priv->realbus, addr, devad, regnum);
}

static int LS1028AQDS_mdio_write(struct mii_dev *bus, int addr, int devad,
			       int regnum, u16 value)
{
	struct LS1028AQDS_mdio *priv = bus->priv;

	LS1028AQDS_mux_mdio(priv->muxval);

	return priv->realbus->write(priv->realbus, addr, devad,
				    regnum, value);
}

static int LS1028AQDS_mdio_reset(struct mii_dev *bus)
{
	struct LS1028AQDS_mdio *priv = bus->priv;

	return priv->realbus->reset(priv->realbus);
}

int LS1028AQDS_mdio_init(char *realbusname, u8 muxval)
{
	struct LS1028AQDS_mdio *pmdio;
	struct mii_dev *bus = mdio_alloc();

	if (!bus) {
		printf("Failed to allocate LS1028AQDS MDIO bus\n");
		return -1;
	}

	pmdio = malloc(sizeof(*pmdio));
	if (!pmdio) {
		printf("Failed to allocate LS1028AQDS private data\n");
		free(bus);
		return -1;
	}

	bus->read = LS1028AQDS_mdio_read;
	bus->write = LS1028AQDS_mdio_write;
	bus->reset = LS1028AQDS_mdio_reset;
	strcpy(bus->name, LS1028AQDS_mdio_name_for_muxval(muxval));

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

void board_ft_fman_fixup_port(void *fdt, char *compat, phys_addr_t addr,
			      enum fm_port port, int offset)
{
	struct fixed_link f_link;

	printf("fixup for port: %d\n", fm_info_get_enet_if(port));

	if (fm_info_get_enet_if(port) == PHY_INTERFACE_MODE_SGMII) {
		if (port == FM1_DTSEC9) {
			fdt_set_phy_handle(fdt, compat, addr,
					   "sgmii_riser_s1_p1");
		} else if (port == FM1_DTSEC2) {
			fdt_set_phy_handle(fdt, compat, addr,
					   "sgmii_riser_s2_p1");
		} else if (port == FM1_DTSEC5) {
			fdt_set_phy_handle(fdt, compat, addr,
					   "sgmii_riser_s3_p1");
		} else if (port == FM1_DTSEC6) {
			fdt_set_phy_handle(fdt, compat, addr,
					   "sgmii_riser_s4_p1");
		}
	} else if (fm_info_get_enet_if(port) ==
		   PHY_INTERFACE_MODE_SGMII_2500) {
		/* 2.5G SGMII interface */
		f_link.phy_id = cpu_to_fdt32(port);
		f_link.duplex = cpu_to_fdt32(1);
		f_link.link_speed = cpu_to_fdt32(1000);
		f_link.pause = 0;
		f_link.asym_pause = 0;
		/* no PHY for 2.5G SGMII */
		fdt_delprop(fdt, offset, "phy-handle");
		fdt_setprop(fdt, offset, "fixed-link", &f_link, sizeof(f_link));
		fdt_setprop_string(fdt, offset, "phy-connection-type",
				   "sgmii-2500");
	} else if (fm_info_get_enet_if(port) ==
		   PHY_INTERFACE_MODE_RGMII || (fm_info_get_enet_if(port) ==
						                   PHY_INTERFACE_MODE_RGMII_TXID)) {
		/* 2.5G SGMII interface */
		f_link.phy_id = cpu_to_fdt32(port);
		f_link.duplex = cpu_to_fdt32(1);
		f_link.link_speed = cpu_to_fdt32(1000);
		f_link.pause = 0;
		f_link.asym_pause = 0;
		/* no PHY for 2.5G SGMII */
		fdt_delprop(fdt, offset, "phy-handle");
		fdt_setprop(fdt, offset, "fixed-link", &f_link, sizeof(f_link));
		fdt_setprop_string(fdt, offset, "phy-connection-type",
				   "rgmii");
	} else if (fm_info_get_enet_if(port) == PHY_INTERFACE_MODE_QSGMII) {
		switch (mdio_mux[port]) {
		case EMI1_SLOT1:
			switch (port) {
			case FM1_DTSEC1:
				fdt_set_phy_handle(fdt, compat, addr,
						   "qsgmii_s1_p1");
				break;
			case FM1_DTSEC2:
				fdt_set_phy_handle(fdt, compat, addr,
						   "qsgmii_s1_p2");
				break;
			case FM1_DTSEC5:
				fdt_set_phy_handle(fdt, compat, addr,
						   "qsgmii_s1_p3");
				break;
			case FM1_DTSEC6:
				fdt_set_phy_handle(fdt, compat, addr,
						   "qsgmii_s1_p4");
				break;
			default:
				break;
			}
			break;
		case EMI1_SLOT2:
			switch (port) {
			case FM1_DTSEC1:
				fdt_set_phy_handle(fdt, compat, addr,
						   "qsgmii_s2_p1");
				break;
			case FM1_DTSEC2:
				fdt_set_phy_handle(fdt, compat, addr,
						   "qsgmii_s2_p2");
				break;
			case FM1_DTSEC5:
				fdt_set_phy_handle(fdt, compat, addr,
						   "qsgmii_s2_p3");
				break;
			case FM1_DTSEC6:
				fdt_set_phy_handle(fdt, compat, addr,
						   "qsgmii_s2_p4");
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		fdt_delprop(fdt, offset, "phy-connection-type");
		fdt_setprop_string(fdt, offset, "phy-connection-type",
				   "qsgmii");
	} else if (fm_info_get_enet_if(port) == PHY_INTERFACE_MODE_XGMII &&
		   port == FM1_10GEC1) {
		/* XFI interface */
		f_link.phy_id = cpu_to_fdt32(port);
		f_link.duplex = cpu_to_fdt32(1);
		f_link.link_speed = cpu_to_fdt32(10000);
		f_link.pause = 0;
		f_link.asym_pause = 0;
		/* no PHY for XFI */
		fdt_delprop(fdt, offset, "phy-handle");
		fdt_setprop(fdt, offset, "fixed-link", &f_link, sizeof(f_link));
		fdt_setprop_string(fdt, offset, "phy-connection-type", "xgmii");
	}
}

void initialize_mac_to_slot(void)
{
	struct ccsr_gur *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	u32 serdes1_prtcl;

	serdes1_prtcl = in_be32(&gur->rcwsr[4]) &
			FSL_CHASSIS2_RCWSR4_SRDS1_PRTCL_MASK;
	serdes1_prtcl >>= FSL_CHASSIS2_RCWSR4_SRDS1_PRTCL_SHIFT;

	switch (serdes1_prtcl) {
		case 0x2555:
		break;
	case 0x4555:
	case 0x4558:
		break;
	case 0x1355:
		break;
	case 0x2460:
			lane_to_slot[0] = 2;
			lane_to_slot[1] = 2;
			lane_to_slot[2] = 2;
			lane_to_slot[3] = 2;
		break;
	case 0x3335:
#if 0
			lane_to_slot[0] = EMI1_SLOT1 - 3;
			lane_to_slot[1] = EMI1_SLOT1 - 3;
			lane_to_slot[2] = EMI1_SLOT1 - 3;
			lane_to_slot[3] = EMI1_SLOT1 - 3;
#else
			lane_to_slot[0] = EMI1_SLOT1 - 2;
			lane_to_slot[1] = EMI1_SLOT1 - 2;
			lane_to_slot[2] = EMI1_SLOT1 - 2;
			lane_to_slot[3] = EMI1_SLOT1 - 2;
#endif
		break;
	case 0x3355:
	case 0x3358:
	case 0x3555:
	case 0x3558:
			lane_to_slot[0] = EMI1_SLOT2 - 3;
			lane_to_slot[1] = EMI1_SLOT2 - 3;
			lane_to_slot[2] = EMI1_SLOT2 - 3;
			lane_to_slot[3] = EMI1_SLOT2 - 3;
		break;
	case 0x3455:
			lane_to_slot[0] = EMI1_SLOT2 - 3;
			lane_to_slot[1] = EMI1_SLOT2 - 3;
			lane_to_slot[2] = EMI1_SLOT2 - 3;
			lane_to_slot[3] = EMI1_SLOT2 - 3;
	break;
	case 0x1455:
		break;
	case 0x2455:
		break;
	case 0x2255:
			lane_to_slot[0] = EMI1_SLOT2 - 3;
			lane_to_slot[1] = EMI1_SLOT2 - 3;
			lane_to_slot[2] = EMI1_SLOT2 - 3;
			lane_to_slot[3] = EMI1_SLOT2 - 3;
		break;
	case 0x3333:
#if 1
	/* SLOT 2*/
	/* lance A and B only */
	/*Lane A is mapped to Slot2-lane2 and Lane B is mapped to Slot2-lane0 */
			lane_to_slot[0] = EMI1_SLOT2 - 3;
			lane_to_slot[1] = EMI1_SLOT2 - 3;
			lane_to_slot[2] = EMI1_SLOT2 - 3;
			lane_to_slot[3] = EMI1_SLOT2 - 3;
#elif 0 
	/* SLOT 3*/
	/* lane C and D only */
	/* lane C and D mux to slot3-lan0 and slot3-lan1 only */
			lane_to_slot[0] = 3;
			lane_to_slot[1] = 3;
			lane_to_slot[2] = 3;
			lane_to_slot[3] = 3;
#else
	/* SLOT 4*/
	/* lane D only */
			lane_to_slot[0] = 4;
			lane_to_slot[1] = 4;
			lane_to_slot[2] = 4;
			lane_to_slot[3] = 4;

#endif
		break;
	default:
		printf("Invalid SerDes protocol 0x%x for LS1028AQDS\n",
		       serdes1_prtcl);
		break;
	}
}

void fdt_fixup_board_enet(void *fdt)
{
	int i;
	struct ccsr_gur *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	u32 srds_s1;

	srds_s1 = in_be32(&gur->rcwsr[4]) &
			FSL_CHASSIS2_RCWSR4_SRDS1_PRTCL_MASK;
	srds_s1 >>= FSL_CHASSIS2_RCWSR4_SRDS1_PRTCL_SHIFT;

	for (i = FM1_DTSEC1; i < NUM_FM_PORTS; i++) {
		switch (fm_info_get_enet_if(i)) {
		case PHY_INTERFACE_MODE_SGMII:
		case PHY_INTERFACE_MODE_QSGMII:
			switch (mdio_mux[i]) {
			case EMI1_SLOT1:
				fdt_status_okay_by_alias(fdt, "emi1_slot1");
				break;
			case EMI1_SLOT2:
				fdt_status_okay_by_alias(fdt, "emi1_slot2");
				break;
			case EMI1_SLOT3:
				fdt_status_okay_by_alias(fdt, "emi1_slot3");
				break;
			case EMI1_SLOT4:
				fdt_status_okay_by_alias(fdt, "emi1_slot4");
				break;
			default:
				break;
			}
			break;
		case PHY_INTERFACE_MODE_XGMII:
			break;
		default:
			break;
		}
	}
}

int board_eth_init(bd_t *bis)
{
#ifdef CONFIG_FMAN_ENET
	int i, idx, lane, slot, interface;
	struct memac_mdio_info dtsec_mdio_info;
	struct memac_mdio_info tgec_mdio_info;
	struct ccsr_gur *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	u32 srds_s1;

	srds_s1 = in_be32(&gur->rcwsr[4]) &
			FSL_CHASSIS2_RCWSR4_SRDS1_PRTCL_MASK;
	srds_s1 >>= FSL_CHASSIS2_RCWSR4_SRDS1_PRTCL_SHIFT;

	/* Initialize the mdio_mux array so we can recognize empty elements */
	for (i = 0; i < NUM_FM_PORTS; i++)
		mdio_mux[i] = EMI_NONE;

	dtsec_mdio_info.regs =
		(struct memac_mdio_controller *)CONFIG_SYS_FM1_DTSEC_MDIO_ADDR;

	dtsec_mdio_info.name = DEFAULT_FM_MDIO_NAME;

	/* Register the 1G MDIO bus */
	fm_memac_mdio_init(bis, &dtsec_mdio_info);

	tgec_mdio_info.regs =
		(struct memac_mdio_controller *)CONFIG_SYS_FM1_TGEC_MDIO_ADDR;
	tgec_mdio_info.name = DEFAULT_FM_TGEC_MDIO_NAME;

	/* Register the 10G MDIO bus */
	fm_memac_mdio_init(bis, &tgec_mdio_info);

	/* Register the muxing front-ends to the MDIO buses */
	LS1028AQDS_mdio_init(DEFAULT_FM_MDIO_NAME, EMI1_RGMII1);
	LS1028AQDS_mdio_init(DEFAULT_FM_MDIO_NAME, EMI1_RGMII2);
	LS1028AQDS_mdio_init(DEFAULT_FM_MDIO_NAME, EMI1_SLOT1);
	LS1028AQDS_mdio_init(DEFAULT_FM_MDIO_NAME, EMI1_SLOT2);
	LS1028AQDS_mdio_init(DEFAULT_FM_MDIO_NAME, EMI1_SLOT3);
	LS1028AQDS_mdio_init(DEFAULT_FM_MDIO_NAME, EMI1_SLOT4);
	LS1028AQDS_mdio_init(DEFAULT_FM_TGEC_MDIO_NAME, EMI2);

	/* Set the two on-board RGMII PHY address */
	fm_info_set_phy_address(FM1_DTSEC3, RGMII_PHY1_ADDR);
	fm_info_set_phy_address(FM1_DTSEC4, RGMII_PHY2_ADDR);

	initialize_mac_to_slot();

	switch (srds_s1) {
	case 0x2555:
		break;
	case 0x4555:
	case 0x4558:
		break;
	case 0x1355:
		break;
	case 0x2355:
		break;
	case 0x2460:
		fm_info_set_phy_address(FM1_DTSEC9, QSGMII_CARD_PORT1_PHY_ADDR_S2);
		fm_info_set_phy_address(FM1_DTSEC2, QSGMII_CARD_PORT2_PHY_ADDR_S2);
		fm_info_set_phy_address(FM1_DTSEC5, QSGMII_CARD_PORT3_PHY_ADDR_S2);
		fm_info_set_phy_address(FM1_DTSEC6, QSGMII_CARD_PORT4_PHY_ADDR_S2);
		break;
	case 0x3335:
#if 0
		fm_info_set_phy_address(FM1_DTSEC9, SGMII_CARD_PORT1_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC2, SGMII_CARD_PORT2_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC5, SGMII_CARD_PORT3_PHY_ADDR);
#else
		fm_info_set_phy_address(FM1_DTSEC9, SGMII_CARD_PORT3_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC2, SGMII_CARD_PORT1_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC5, SGMII_CARD_PORT4_PHY_ADDR);
#endif
		break;
	case 0x3355:
	case 0x3358:
	case 0x3555:
	case 0x3558:
		fm_info_set_phy_address(FM1_DTSEC9, SGMII_CARD_PORT3_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC2, SGMII_CARD_PORT1_PHY_ADDR);
		break;
	case 0x1455:
		break;
	case 0x2455:
		break;
	case 0x2255:
		/* 2.5G SGMII on lane A, MAC 9 */
		fm_info_set_phy_address(FM1_DTSEC9, 0x1);
		/* 2.5G SGMII on lane B, MAC 2 */
		fm_info_set_phy_address(FM1_DTSEC2, 0x10);
		break;
	case 0x3333:
#if 1
	/* SLOT 2*/
	/* lance A and B only */
	/*Lane A is mapped to Slot2-lane2 and Lane B is mapped to Slot2-lane0 */
		fm_info_set_phy_address(FM1_DTSEC9, SGMII_CARD_PORT3_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC2, SGMII_CARD_PORT1_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC5, SGMII_CARD_PORT2_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC6, SGMII_CARD_PORT4_PHY_ADDR);
#elif 0
	/* SLOT 3*/
	/* lane C and D mux to slot3-lan0 and slot3-lan1 only */
		fm_info_set_phy_address(FM1_DTSEC5, SGMII_CARD_PORT1_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC6, SGMII_CARD_PORT2_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC9, SGMII_CARD_PORT3_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC2, SGMII_CARD_PORT4_PHY_ADDR);
#else

	/* SLOT 4*/
	/* lane D */
		fm_info_set_phy_address(FM1_DTSEC6, SGMII_CARD_PORT1_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC5, SGMII_CARD_PORT2_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC9, SGMII_CARD_PORT3_PHY_ADDR);
		fm_info_set_phy_address(FM1_DTSEC2, SGMII_CARD_PORT4_PHY_ADDR);

#endif
		break;
	case 0x3455:
		fm_info_set_phy_address(FM1_DTSEC6, QSGMII_CARD_PORT1_PHY_ADDR_S1);
		fm_info_set_phy_address(FM1_DTSEC5, QSGMII_CARD_PORT2_PHY_ADDR_S1);
		fm_info_set_phy_address(FM1_DTSEC9, QSGMII_CARD_PORT3_PHY_ADDR_S1);
		fm_info_set_phy_address(FM1_DTSEC2, QSGMII_CARD_PORT4_PHY_ADDR_S1);
		break;
	default:
		printf("Invalid SerDes protocol 0x%x for LS1028AQDS\n",
		       srds_s1);
		break;
	}

	for (i = FM1_DTSEC1; i < FM1_DTSEC1 + CONFIG_SYS_NUM_FM1_DTSEC; i++) {
		idx = i - FM1_DTSEC1;
		interface = fm_info_get_enet_if(i);
		switch (interface) {
		case PHY_INTERFACE_MODE_SGMII:
		case PHY_INTERFACE_MODE_SGMII_2500:
		case PHY_INTERFACE_MODE_QSGMII:
			if (interface == PHY_INTERFACE_MODE_SGMII) {
				lane = serdes_get_first_lane(FSL_SRDS_1,
						SGMII_FM1_DTSEC1 + idx);
			} else if (interface == PHY_INTERFACE_MODE_SGMII_2500) {
				lane = serdes_get_first_lane(FSL_SRDS_1,
						SGMII_2500_FM1_DTSEC1 + idx);
			} else {
				lane = serdes_get_first_lane(FSL_SRDS_1,
						QSGMII_FM1_A);
			}

			if (lane < 0)
				break;

			slot = lane_to_slot[lane];
			printf("FM1@DTSEC%u expects SGMII in slot %u\n",
			      idx + 1, slot);
			if (QIXIS_READ(present2) & (1 << (slot - 1)))
				fm_disable_port(i);

			switch (slot) {
			case 1:
				printf("case 1\n");
				mdio_mux[i] = EMI1_SLOT1;
				fm_info_set_mdio(i, mii_dev_for_muxval(
						 mdio_mux[i]));
				break;
			case 2:
				printf("case 2\n");
				mdio_mux[i] = EMI1_SLOT2;
				fm_info_set_mdio(i, mii_dev_for_muxval(
						 mdio_mux[i]));
				break;
			case 3:
				printf("case 3\n");
				mdio_mux[i] = EMI1_SLOT3;
				fm_info_set_mdio(i, mii_dev_for_muxval(
						 mdio_mux[i]));
				break;
			case 4:
				printf("case 4\n");
				mdio_mux[i] = EMI1_SLOT4;
				fm_info_set_mdio(i, mii_dev_for_muxval(
						 mdio_mux[i]));
				break;
			default:
				break;
			}
			break;
		case PHY_INTERFACE_MODE_RGMII:
		case PHY_INTERFACE_MODE_RGMII_TXID:
			if (i == FM1_DTSEC3)
				mdio_mux[i] = EMI1_RGMII1;
			else if (i == FM1_DTSEC4)
				mdio_mux[i] = EMI1_RGMII2;

			fm_info_set_mdio(i, mii_dev_for_muxval(mdio_mux[i]));
			break;
		default:
			break;
		}
	}

	cpu_eth_init(bis);
#endif /* CONFIG_FMAN_ENET */

	return pci_eth_init(bis);
}
