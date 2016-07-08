/*
* Copyright (C) 2016 Freescale Semiconductor Inc.
*
* SPDX-License-Identifier:GPL-2.0+
*/
#include <common.h>
#include <config.h>
//#include <asm/arch/hardware.h>
#include <asm/byteorder.h>
#include <net.h>
#include <command.h>
#include <miiphy.h>
#include "pfe_eth.h"

struct gemac_s gem_info[] = {
        /* PORT_0 configuration */
        {
                /* GEMAC config */
                .gemac_mode = GMII,
                .gemac_speed = SPEED_1000M,
                .gemac_duplex = DUPLEX_FULL,

                /* phy iface */
                .phy_address = EMAC1_PHY_ADDR,
		.phy_mode = PHY_INTERFACE_MODE_SGMII,
        },
        /* PORT_1 configuration */
        {
                /* GEMAC config */
                .gemac_mode = GMII,
                .gemac_speed = SPEED_1000M,
                .gemac_duplex = DUPLEX_FULL,

                /* phy iface */
                .phy_address = EMAC2_PHY_ADDR,
		.phy_mode = PHY_INTERFACE_MODE_RGMII,
        },
};

#define MAX_GEMACS      2

static struct ls1012a_eth_dev *gemac_list[MAX_GEMACS];

/* Max MII register/address (we support) */
#define MII_REGISTER_MAX  31
#define MII_ADDRESS_MAX   31

#define MDIO_TIMEOUT    5000


static void ls1012a_gemac_enable(void *gemac_base)
{
        writel(readl(gemac_base + EMAC_ECNTRL_REG) | EMAC_ECNTRL_ETHER_EN, gemac_base + EMAC_ECNTRL_REG);	
}

static void ls1012a_gemac_disable(void *gemac_base)
{
        writel(readl(gemac_base + EMAC_ECNTRL_REG) & ~EMAC_ECNTRL_ETHER_EN, gemac_base + EMAC_ECNTRL_REG);	
}

static void ls1012a_gemac_set_speed(void *gemac_base, u32 speed)
{
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;
	u32 ecr = readl(gemac_base + EMAC_ECNTRL_REG) & ~EMAC_ECNTRL_SPEED;
	u32 rcr = readl(gemac_base + EMAC_RCNTRL_REG) & ~EMAC_RCNTRL_RMII_10T;
	u32 rgmii_pcr = in_be32(&scfg->rgmiipcr) & ~(SCFG_RGMIIPCR_SETSP_1000M|SCFG_RGMIIPCR_SETSP_10M);

	if (speed == _1000BASET) {
		ecr |= EMAC_ECNTRL_SPEED;
		rgmii_pcr |= SCFG_RGMIIPCR_SETSP_1000M;
	}
	else if (speed != _100BASET){
		rcr |= EMAC_RCNTRL_RMII_10T;
		rgmii_pcr |= SCFG_RGMIIPCR_SETSP_10M;
	}

	writel(ecr, gemac_base + EMAC_ECNTRL_REG);
	out_be32(&scfg->rgmiipcr, rgmii_pcr | SCFG_RGMIIPCR_SETFD);

	/* remove loop back */
	rcr &= ~EMAC_RCNTRL_LOOP;
	/* enable flow control */
	rcr |= EMAC_RCNTRL_FCE;

	/* Enable MII mode */
	rcr |= EMAC_RCNTRL_MII_MODE;

	/* CRC field is stripped from the frame */
	//rcr |= EMAC_RCNTRL_CRC_FWD;

	/* Enable promiscuous mode
	   FIXME should be removed later*/
	//rcr |= EMAC_RCNTRL_PROM;
	writel(rcr, gemac_base + EMAC_RCNTRL_REG);

	/*Enable Tx full duplex */
	writel(readl(gemac_base + EMAC_TCNTRL_REG ) | EMAC_TCNTRL_FDEN, gemac_base + EMAC_TCNTRL_REG);

}

static void ls1012a_gemac_set_ethaddr(void *gemac_base, uchar *mac)
{
        writel((mac[0] << 24) + (mac[1] << 16) + (mac[2] << 8) + mac[3], gemac_base + EMAC_PHY_ADDR_LOW);
        writel((mac[4] << 24) + (mac[5] << 16) + 0x8808, gemac_base + EMAC_PHY_ADDR_HIGH);
}

/** Stops or Disables GEMAC pointing to this eth iface.
 *
 * @param[in]   edev    Pointer to eth device structure.
 *
 * @return      none
 */
static void ls1012a_eth_halt(struct eth_device *edev)
{
        struct ls1012a_eth_dev *priv = (struct ls1012a_eth_dev *)edev->priv;

        ls1012a_gemac_disable(priv->gem->gemac_base);

        gpi_disable(priv->gem->egpi_base);

        return;
}

static int ls1012a_eth_init(struct eth_device *dev, bd_t * bd)
{
        struct ls1012a_eth_dev *priv = (struct ls1012a_eth_dev *)dev->priv;
        struct gemac_s *gem = priv->gem;
	int speed;

        /* set ethernet mac address */
        ls1012a_gemac_set_ethaddr(gem->gemac_base, dev->enetaddr);

	//MAC will be always in GMII mode, it doesn't change with the link speed.
	//ls1012a_gemac_set_mode(gem->gemac_base, gem->gemac_mode);

	writel(0x00000004, gem->gemac_base + EMAC_TFWR_STR_FWD);
	writel(0x00000005, gem->gemac_base + EMAC_RX_SECTIOM_FULL);
	writel(0x00003fff, gem->gemac_base + EMAC_TRUNC_FL);
	writel(0x00000030, gem->gemac_base + EMAC_TX_SECTION_EMPTY);
	writel(0x00000000, gem->gemac_base + EMAC_MIB_CTRL_STS_REG);

#ifndef CONFIG_EMU
#ifdef CONFIG_PHYLIB
	/* Start up the PHY */
	//if(gem->phy_mode != PHY_INTERFACE_MODE_SGMII) {
	if (phy_startup(priv->phydev)) {
		printf("Could not initialize PHY %s\n",
				priv->phydev->dev->name);
		return -1;
	}
	speed = priv->phydev->speed;
	printf("Speed detected %x\n", speed);
	if(priv->phydev->duplex == DUPLEX_HALF) {
		printf("Half duplex not supported \n");
		return -1;
	}
#endif
#else
	/*in emulator it is always 1000Mbps */
	speed = _1000BASET;
#endif
	ls1012a_gemac_set_speed(gem->gemac_base, speed);

        /* Enable GPI */
        gpi_enable(gem->egpi_base);

        /* Enable GEMAC */
        ls1012a_gemac_enable(gem->gemac_base);

	return 0;

}

static int ls1012a_eth_send(struct eth_device *dev, void *data, int length)
{
        struct ls1012a_eth_dev *priv = (struct ls1012a_eth_dev *)dev->priv;

        int rc;
	int i=0;

        rc = pfe_send(priv->gemac_port, data, length);

        if (rc < 0) {
                printf("Tx Q full\n");
                return 0;
        }

        while (1) {
                rc = pfe_tx_done();
                if (rc == 0)
                        break;
		
		udelay(100);
		i++;
		if(i == 30000)
			printf("Tx timeout, send failed\n");
			break;

	}

	return 0;
}

static int ls1012a_eth_recv(struct eth_device *dev)
{
        struct ls1012a_eth_dev *priv = (struct ls1012a_eth_dev *)dev->priv;
        u32 pkt_buf;
        int len;
        int phy_port;

        len = pfe_recv(&pkt_buf, &phy_port);

        if (len < 0)
                return 0; //no packet in rx

        dprint("Rx pkt: pkt_buf(%08x), phy_port(%d), len(%d)\n", pkt_buf, phy_port, len);
        if (phy_port != priv->gemac_port)  {
                printf("Rx pkt not on expected port\n");
                return 0;
        }

	// Pass the packet up to the protocol layers.
	net_process_received_packet((void *)(long int)pkt_buf, len);

	return 0;
}

#if defined(CONFIG_PHYLIB)

#define MDIO_TIMEOUT    5000
static int ls1012a_phy_read(struct mii_dev *bus, int phy_addr, int dev_addr, int reg_addr)
{
	void *reg_base = bus->priv;
	u32 reg;
	u32 phy;
	u32 reg_data;
	u16 val;
	int timeout = MDIO_TIMEOUT;

	reg = ((reg_addr & EMAC_MII_DATA_RA_MASK) << EMAC_MII_DATA_RA_SHIFT);
	phy = ((phy_addr & EMAC_MII_DATA_PA_MASK) << EMAC_MII_DATA_PA_SHIFT);

	reg_data = (EMAC_MII_DATA_ST | EMAC_MII_DATA_OP_RD | EMAC_MII_DATA_TA | phy | reg );

	//dprint("%s write data %x %x %x\n", __func__, reg_data, reg_addr, phy_addr);
	writel(reg_data, reg_base + EMAC_MII_DATA_REG);

        /*
         * wait for the MII interrupt
         */
	while(!(readl(reg_base + EMAC_IEVENT_REG) & EMAC_IEVENT_MII))
	{
		if (timeout-- <= 0) {
			printf("Phy MDIO read/write timeout\n");
			return -1;
		}
	}

	/*
         * clear MII interrupt
         */
        writel(EMAC_IEVENT_MII, reg_base + EMAC_IEVENT_REG);

        /*
         * it's now safe to read the PHY's register
         */
        val = (u16)readl(reg_base + EMAC_MII_DATA_REG);
        dprint("%s: %x phy: %02x reg:%02x val:%#x\n", __func__, reg_base, phy_addr, reg_addr, val);

        return val;
}

static int ls1012a_phy_write(struct mii_dev *bus, int phy_addr, int dev_addr, int reg_addr, u16 data)
{
	void *reg_base = bus->priv;
	u32 reg;
	u32 phy;
	u32 reg_data;
	int timeout = MDIO_TIMEOUT;
	int val;

	reg = ((reg_addr & EMAC_MII_DATA_RA_MASK) << EMAC_MII_DATA_RA_SHIFT);
	phy = ((phy_addr & EMAC_MII_DATA_PA_MASK) << EMAC_MII_DATA_PA_SHIFT);

	reg_data = (EMAC_MII_DATA_ST | EMAC_MII_DATA_OP_WR | EMAC_MII_DATA_TA | phy | reg | data);

	//dprint("%s write data %x\n", __func__, reg_data);
	writel(reg_data, reg_base + EMAC_MII_DATA_REG);

        /*
         * wait for the MII interrupt
         */
	while(!(readl(reg_base + EMAC_IEVENT_REG) & EMAC_IEVENT_MII))
	{
		if (timeout-- <= 0) {
			printf("Phy MDIO read/write timeout\n");
			return -1;
		}
	}

	/*
         * clear MII interrupt
         */
        writel(EMAC_IEVENT_MII, reg_base + EMAC_IEVENT_REG);

        dprint("%s: phy: %02x reg:%02x val:%#x\n", __func__, phy_addr, reg_addr, data);

        return val;
}


struct mii_dev *ls1012a_mdio_init(struct mdio_info *mdio_info)
{
        struct mii_dev *bus;
        int ret;
	u32 mdio_speed;
	u32 pclk = 250000000;

        bus = mdio_alloc();
        if (!bus) {
                printf("mdio_alloc failed\n");
                return NULL;
        }
        bus->read = ls1012a_phy_read;
        bus->write = ls1012a_phy_write;
	/* MAC1 MDIO used to communicate with external PHYS */
        bus->priv = mdio_info->reg_base;
	sprintf(bus->name, mdio_info->name);

	/*configure mdio speed */
	mdio_speed = (DIV_ROUND_UP(pclk, 4000000) << EMAC_MII_SPEED_SHIFT);
	mdio_speed |= EMAC_HOLDTIME(0x5);
	writel(mdio_speed, mdio_info->reg_base + EMAC_MII_CTRL_REG);

        ret = mdio_register(bus);
        if (ret) {
                printf("mdio_register failed\n");
                free(bus);
                return NULL;
        }
	return bus;
}

static void ls1012a_configure_serdes(struct ls1012a_eth_dev *priv)
{
	struct mii_dev bus;
	int value,sgmii_2500=0;

	printf("%s %d\n", __func__, priv->gemac_port);
	/* PCS configuration done with corresponding GEMAC */
	bus.priv = gem_info[priv->gemac_port].gemac_base;

	ls1012a_phy_read(&bus, 0, MDIO_DEVAD_NONE, 0x0);
	ls1012a_phy_read(&bus, 0, MDIO_DEVAD_NONE, 0x1);
	ls1012a_phy_read(&bus, 0, MDIO_DEVAD_NONE, 0x2);
	ls1012a_phy_read(&bus, 0, MDIO_DEVAD_NONE, 0x3);
#if 0
	/*These settings taken from validtion team */
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x0, 0x8000);
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x14, 0xb); //3 in case our code
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x4, 0x1a1);
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x12, 0x400);
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x13, 0x0);
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x0, 0x1140);
	return;
#endif

	/*Reset serdes */
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x0, 0x8000);

	/* SGMII IF mode + AN enable only for 1G SGMII, not for 2.5G */
	value = PHY_SGMII_IF_MODE_SGMII;
	if (!sgmii_2500)
		value |= PHY_SGMII_IF_MODE_AN;

	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x14, value);

	/* Dev ability according to SGMII specification */
	value = PHY_SGMII_DEV_ABILITY_SGMII;
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x4, value);

	/* Adjust link timer for SGMII  -
	1.6 ms in units of 8 ns = 2 * 10^5 = 0x30d40 */
	//ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x13, 0x3);
	//ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x12, 0xd40);

	//These values taken from validation team
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x13, 0x0);
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0x12, 0x400);

	/* Restart AN */
	value = PHY_SGMII_CR_DEF_VAL;
	if (!sgmii_2500)
		value |= PHY_SGMII_CR_RESET_AN;
	ls1012a_phy_write(&bus, 0, MDIO_DEVAD_NONE, 0, value);


}

void ls1012a_set_mdio(int dev_id, struct mii_dev *bus)
{
	gem_info[dev_id].bus = bus;
}

void ls1012a_set_phy_address_mode(int dev_id, int phy_id, int phy_mode)
{
	gem_info[dev_id].phy_address = phy_id;
	gem_info[dev_id].phy_mode  = phy_mode;
}

int ls1012a_phy_configure(struct ls1012a_eth_dev *priv, int dev_id, int phy_id)
{
	struct phy_device *phydev = NULL;
	struct eth_device *dev = priv->dev;
        struct gemac_s *gem = priv->gem;
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;

	//Configure SGMII  PCS
	if(gem->phy_mode == PHY_INTERFACE_MODE_SGMII ||
			gem->phy_mode == PHY_INTERFACE_MODE_SGMII_2500)
	{
		//printf("Select MDIO from serdes\n");
		out_be32(&scfg->mdioselcr, 0x00000000);
		ls1012a_configure_serdes(priv);
	}

	/*By this time on-chip SGMII initialization is done
	* we can swith mdio interface to external PHYs */
	//printf("Select MDIO from PAD\n");
	out_be32(&scfg->mdioselcr, 0x80000000);

	if(! gem->bus) return -1;
        phydev = phy_connect(gem->bus, phy_id, dev, gem->phy_mode);
        if (!phydev) {
                printf("phy_connect failed\n");
                return -1;
        }

        phy_config(phydev);

	priv->phydev = phydev;

        return 0;
}
#endif

int ls1012a_gemac_initialize(bd_t * bis, int dev_id, char *devname)
{
        struct eth_device *dev;
        struct ls1012a_eth_dev *priv;
        struct pfe *pfe;
	int i;

	if(dev_id > 1)
	{
		printf("Invalid port\n");
		return -1;
	}

        dev = (struct eth_device *)malloc(sizeof(struct eth_device));
        if (!dev)
                return -1;

        memset(dev, 0, sizeof(struct eth_device));

        priv = (struct ls1012a_eth_dev *)malloc(sizeof(struct ls1012a_eth_dev));
        if (!priv)
                return -1;

        gemac_list[dev_id] = priv;
	priv->gemac_port = dev_id;
        priv->gem = &gem_info[priv->gemac_port];
        priv->dev = dev;

        pfe = &priv->pfe;

        pfe->cbus_baseaddr = (void *)CONFIG_SYS_PPFE_ADDR;
        pfe->ddr_baseaddr = (void *)CONFIG_DDR_PPFE_BASEADDR;
        pfe->ddr_phys_baseaddr = (unsigned long)CONFIG_DDR_PPFE_PHYS_BASEADDR;

	sprintf(dev->name, devname);
        dev->priv = priv;
        dev->init = ls1012a_eth_init;
        dev->halt = ls1012a_eth_halt;
        dev->send = ls1012a_eth_send;
        dev->recv = ls1012a_eth_recv;

        /* Tell u-boot to get the addr from the env */
        for (i = 0; i < 6; i++)
                dev->enetaddr[i] = 0;

        pfe_probe(pfe);

        switch(priv->gemac_port)  {
                case EMAC_PORT_0:
                default:
                        priv->gem->gemac_base = EMAC1_BASE_ADDR;
                        priv->gem->egpi_base = EGPI1_BASE_ADDR;
                break;
                case EMAC_PORT_1:
                        priv->gem->gemac_base = EMAC2_BASE_ADDR;
                        priv->gem->egpi_base = EGPI2_BASE_ADDR;
                break;
        }


#ifndef CONFIG_EMU
#if defined(CONFIG_PHYLIB)
	if(ls1012a_phy_configure(priv, dev_id, gem_info[priv->gemac_port].phy_address))
		return -1;
#else
	#error ("Please enable CONFIG_PHYLIB")
#endif
#endif

        eth_register(dev);

	return 0;
}
