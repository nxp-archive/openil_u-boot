/*
 * ENETC ethernet controller driver
 *
 * Copyright 2017-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <memalign.h>
#include <asm/io.h>
#include <pci.h>
#include <net.h>
#include <asm/processor.h>
#include <config.h>
#include "fsl_enetc.h"

DECLARE_GLOBAL_DATA_PTR;

/* number of VFs per PF; only the first two PFs contain VFs */
static u8 enetc_vfs[ENETC_NUM_PFS] = { 2, 2 };

/* number of station interfaces per port */
static u8 enetc_ports[] = {3, 3, 1, 1};

/* ENETC IERB MAC address registers accept big-endian format */
static inline void enetc_set_ierb_primary_mac(u8 port, u8 si, u8 *addr)
{
	u16 lower = *(const u16 *)(addr + 4);
	u32 upper = *(const u32 *)addr;

	enetc_write_reg(ENETC_IERB_PMAR(0, port, si), upper);
	enetc_write_reg(ENETC_IERB_PMAR(1, port, si), lower);
}

/* configure MAC addresses for all station interfaces on all ports */
static void enetc_setup_port_macs(void)
{
	u8 pnum = ARRAY_SIZE(enetc_ports) - 1;
	u8 ethno, p, si, vsin = 0;
	u8 addr[6] = {0};

	for (p = 0; p <= pnum; p++)
		for (si = 0; si < enetc_ports[p]; si++) {
			/* virtual SIs */
			ethno = si ? pnum + vsin++ : p;
			eth_env_get_enetaddr_by_index("eth", ethno, addr);
			if (is_zero_ethaddr(addr))
				continue;
			enetc_set_ierb_primary_mac(p, si, addr);
			memset(addr, 0, sizeof(addr));
		}
}

void enetc_setup(void *blob)
{
	int streamid, sid_base, off;
	int pf, vf, vfnn = 1;
	u32 iommu_map[4];
	int err;

	debug("configuring ENETC AMQs, MACs..\n");
	/* find integrated endpoints, we should only have one node */
	off = fdt_node_offset_by_compatible(blob, 0, "pci-host-ecam-generic");
	if (off < 0) {
		debug("ENETC IEP node not found\n");
		goto enetc_resume_setup;
	}

	err = fdtdec_get_int_array(blob, off, "iommu-map", iommu_map, 4);
	if (err) {
		debug("\"iommu-map\" not found, using default SID base %04x\n",
		      ENETC_IERB_STREAMID_START);
		sid_base = ENETC_IERB_STREAMID_START;
	} else {
		sid_base = iommu_map[2];
	}
	/* set up AMQs for all IEPs */
	for (pf = 0; pf < ENETC_NUM_PFS; pf++) {
		streamid = sid_base + pf;
		enetc_write_reg(ENETC_IERB_PFAMQ(pf, 0), streamid);

		/* set up AMQs for VFs, if any */
		for (vf = 0; vf < enetc_vfs[pf]; vf++, vfnn++) {
			streamid = sid_base + ENETC_NUM_PFS + vfnn;
			enetc_write_reg(ENETC_IERB_PFAMQ(pf, vf + 1), streamid);
		}
	}

enetc_resume_setup:
	/* set MSI cache attributes */
	enetc_write_reg(ENETC_IERB_MSICAR, ENETC_MSICAR_VALUE);
	/* initialize MAC addresses */
	enetc_setup_port_macs();
}

static struct pci_device_id enetc_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_FREESCALE, 0xe100) },
	{}
};

static int pcie_enetc_ofdata_to_platdata(struct udevice *dev)
{
	struct enetc_rcie *pcie = dev_get_priv(dev);
	struct fdt_resource resr;
	int err;

	err = fdt_get_resource(gd->fdt_blob, dev_of_offset(dev), "reg", 0,
			       &resr);
	if (err < 0) {
		debug("\"reg\" resource not found\n");
		return err;
	}

	pcie->cfg_base = map_physmem(resr.start, fdt_resource_size(&resr),
				     MAP_NOCACHE);

	pcie->busno = dev->seq;
	return 0;
}

static inline int pcie_enetc_ecam_config_address(struct enetc_rcie *pcie,
						 pci_dev_t bdf,
						 uint offset, void **paddress)
{
	unsigned int bus = PCI_BUS(bdf);
	unsigned int dev = PCI_DEV(bdf);
	unsigned int fn = PCI_FUNC(bdf);
	void *addr;

	addr = pcie->cfg_base;
	addr += ENETC_PF_HDR_ADD(bus - pcie->busno, dev, fn);
	addr += offset;
	*paddress = addr;

	return 0;
}

static int pcie_enetc_ecam_read_config(struct udevice *bus, pci_dev_t bdf,
				       uint offset, ulong *valuep,
				       enum pci_size_t size)
{
	struct enetc_rcie *pcie = dev_get_priv(bus);
	void *address;
	int err;

	err = pcie_enetc_ecam_config_address(pcie, bdf, offset, &address);
	if (err < 0) {
		*valuep = pci_get_ff(size);
		return 0;
	}

	switch (size) {
	case PCI_SIZE_8:
		*valuep = readb(address);
		return 0;
	case PCI_SIZE_16:
		*valuep = readw(address);
		return 0;
	case PCI_SIZE_32:
		*valuep = readl(address);
		return 0;
	default:
		return -EINVAL;
	}
}

static int pcie_enetc_ecam_write_config(struct udevice *bus, pci_dev_t bdf,
					uint offset, ulong value,
					enum pci_size_t size)
{
	struct enetc_rcie *pcie = dev_get_priv(bus);
	void *address;
	int err;

	err = pcie_enetc_ecam_config_address(pcie, bdf, offset, &address);
	if (err < 0)
		return 0;

	switch (size) {
	case PCI_SIZE_8:
		writeb(value, address);
		return 0;
	case PCI_SIZE_16:
		writew(value, address);
		return 0;
	case PCI_SIZE_32:
		writel(value, address);
		return 0;
	default:
		return -EINVAL;
	}
}

static int num_devices;
static int enetc_bind(struct udevice *dev)
{
	char name[16];

	sprintf(name, "enetc#%u", num_devices++);
	return device_set_name(dev, name);
}

int enetc_init_pf_regs(struct udevice *dev)
{
	struct enetc_devfn *hw = dev_get_priv(dev);
	u64 regs_addr64 = 0ULL;
	u32 val = 0;

	/* Get PF BARs from EA (enhanced allocation) structures;
	 * ENETC uses EA and standard PCI header BARs are hardwired to 0
	 */
	dm_pci_read_config32(dev, PCI_CFC_EA_CAP_BASE, &val);

	/* check if EA capability exists */
	if ((val & 0xff) != PCI_CFC_EA_CAP_ID) {
		ENETC_DBG(hw, "PCIe EA not supported\n");
		return -EEXIST;
	}

	/* check number of BARs */
	if (((val >> 16) & 0x3f) != ENETC_PF_HDR_T0_NUM_BAR) {
		ENETC_DBG(hw, "No PF regs BARs in EA struct\n");
		return -EINVAL;
	}

	val = 0;
	dm_pci_read_config32(dev, PCI_EA_PF_REG_FORMAT, &val);

	/* check enable and BEI (BAR equiv. indicator) */
	if (!PCI_EA_PF_REG_FORMAT_ENABLE(val) ||
	    PCI_EA_PF_REG_FORMAT_BEI(val) != PCI_EA_PF_BEI_BAR0)
		return -EINVAL;

	hw->regs_base = 0;
	hw->port_regs = 0;
	/* read low address */
	val = 0;
	dm_pci_read_config32(dev, PCI_EA_PF_BAR_LOW, &val);
	regs_addr64 = PCI_EA_PF_BASE_REG(val);

	/* read high address if necessary */
	if (PCI_EA_PF_BASE_REG_IS_64BIT(val)) {
		val = 0;
		dm_pci_read_config32(dev, PCI_EA_PF_BAR_HIGH, &val);
		regs_addr64 |= ((u64)val) << 32;
	}

	/* read reg. region size */
	val = 0;
	dm_pci_read_config32(dev, PCI_EA_PF_BAR_SIZE, &val);

	hw->regs_size = val;
	/* map BAR address to virtual address; also must have a MMU entry */
	hw->regs_base = map_physmem(regs_addr64, hw->regs_size, MAP_NOCACHE);
	hw->port_regs = map_physmem(regs_addr64 + ENETC_PORT_REGS_OFF, 0,
				    MAP_NOCACHE);

	ENETC_DBG(hw, "regs:0x%x%08x, size:0x%x\n", upper_32_bits(regs_addr64),
		  lower_32_bits(regs_addr64), hw->regs_size);

	return 0;
}

/* Initiate PCIe PF ENETC function level reset */
static int enetc_pf_flr(struct udevice *dev)
{
	struct enetc_devfn *hw = dev_get_priv(dev);
	u32 val = 0;

	dm_pci_read_config32(dev, PCI_CFC_PCIE_DEV_CAP, &val);
	/* check if FLR capability exists */
	if (!(val & PCI_CFC_PCIE_FLR_CAP_ID)) {
		ENETC_ERR(hw, "FLR not supported\n");
		return -EEXIST;
	}
	/* initiate FLR request */
	dm_pci_write_config16(dev, PCI_CFC_PCIE_DEV_CTL,
			      PCI_CFC_PCIE_DEV_CTL_INIT_FLR);
	/* wait ~100ms for FLR completion */
	udelay(ENETC_DEV_FLR_WAIT_MS);

	/* check if FLR completed */
	dm_pci_read_config16(dev, PCI_CFC_PCIE_DEV_STAT, (u16 *)&val);

	return val & PCI_CFC_PCIE_DEV_STAT_TRANS_PEND ? -EINVAL : 0;
}

int enetc_enable_si_port(struct enetc_devfn *hw)
{
	u32 val;

	/* set Rx/Tx BDR count */
	val = ENETC_PSICFGR_SET_TXBDR(ENETC_TX_BDR_CNT);
	val |= ENETC_PSICFGR_SET_RXBDR(ENETC_RX_BDR_CNT);
	enetc_write_port(hw, ENETC_PSICFGR(0), val);
	/* set Rx max frame size */
	enetc_write_port(hw, ENETC_PM_MAXFRM, ENETC_RX_MAXFRM_SIZE);
	/* enable MAC port */
	enetc_write_port(hw, ENETC_PM_CC, ENETC_PM_CC_RX_TX_EN);
	/* enable port */
	enetc_write_port(hw, ENETC_PMR, ENETC_PMR_SI0_EN);
	/* set SI cache policy */
	enetc_write(hw, ENETC_SICAR0, ENETC_SICAR_RD_CFG | ENETC_SICAR_WR_CFG);
	/* enable SI */
	enetc_write(hw, ENETC_SIMR, ENETC_SIMR_EN);

	return 0;
}

static int enetc_disable_si_port(struct enetc_devfn *hw)
{
	/* disable MAC port */
	enetc_write_port(hw, ENETC_PM_CC, ENETC_PM_CC_DEFAULT);

	/* disable station interface */
	enetc_write(hw, ENETC_SIMR, 0);

	/* disable station interface ports */
	enetc_write_port(hw, ENETC_PMR, 0);

	return 0;
}

/*
 * Probe ENETC driver:
 * - initialize port and station interface BARs
 */
static int enetc_probe(struct udevice *dev)
{
	struct enetc_devfn *hw = dev_get_priv(dev);
	int ret = 0;

	hw->name = dev->name;
	hw->devno = trailing_strtol(dev->name);
	ENETC_DBG(hw, "probing driver ...\n");

	/* initialize register */
	ret = enetc_init_pf_regs(dev);
	if (ret < 0) {
		ENETC_DBG(hw, "failed to initialize PSI reg base\n");
		return ret;
	}

	return ret;
}

/**
 * Remove ENETC driver:
 * - disable port and station interface
 * - run function level reset
 */
static int enetc_remove(struct udevice *dev)
{
	struct enetc_devfn *hw = dev_get_priv(dev);

	ENETC_DBG(hw, "removing driver ...\n");
	enetc_disable_si_port(hw);

	/* reset device */
	enetc_pf_flr(dev);

	return 0;
}

/* ENETC Port MAC address registers accept big-endian format */
static void enetc_set_primary_mac_addr(struct enetc_devfn *hw, const u8 *addr)
{
	u16 lower = *(const u16 *)(addr + 4);
	u32 upper = *(const u32 *)addr;

	enetc_write_port(hw, ENETC_PSIPMAR0(hw->devno), upper);
	enetc_write_port(hw, ENETC_PSIPMAR1(hw->devno), lower);
}

/* Share rx/tx descriptor buffers since U-Boot does not support multiple
 * active Ethernet connections.
 */
DEFINE_ALIGN_BUFFER(struct enetc_tx_bd, enetc_txbd, ENETC_BD_CNT, ENETC_ALIGN);
DEFINE_ALIGN_BUFFER(union enetc_rx_bd, enetc_rxbd, ENETC_BD_CNT, ENETC_ALIGN);
DEFINE_ALIGN_BUFFER(u8, enetc_rx_buff, ENETC_RX_MBUFF_SIZE, ENETC_ALIGN);

inline u64 enetc_rxb_address(struct udevice *dev, int i)
{
	int off = i * ENETC_RX_MAXFRM_SIZE;

	return cpu_to_le64(dm_pci_virt_to_mem(dev, enetc_rx_buff + off));
}

/**
 * Setup a single Tx BD Ring (ID = 0):
 * - set Tx buffer descriptor address
 * - set the BD count
 * - initialize the producer and consumer index
 */
static void enetc_setup_tx_bdr(struct enetc_devfn *hw)
{
	struct bd_ring *tx_bdr = &hw->tx_bdr;
	u64 tx_bd_add = (u64)enetc_txbd;

	/* used later to advance to the next Tx BD */
	tx_bdr->bd_count = ENETC_BD_CNT;
	tx_bdr->next_prod_idx = 0;
	tx_bdr->next_cons_idx = 0;
	tx_bdr->cons_idx = hw->regs_base +
				ENETC_BDR(TX, ENETC_TX_BDR_ID, ENETC_TBCIR);
	tx_bdr->prod_idx = hw->regs_base +
				ENETC_BDR(TX, ENETC_TX_BDR_ID, ENETC_TBPIR);

	/* set Tx BD address */
	enetc_bdr_write(hw, TX, ENETC_TX_BDR_ID, ENETC_TBBAR0,
			lower_32_bits(tx_bd_add));
	enetc_bdr_write(hw, TX, ENETC_TX_BDR_ID, ENETC_TBBAR1,
			upper_32_bits(tx_bd_add));
	/* set Tx 8 BD count */
	enetc_bdr_write(hw, TX, ENETC_TX_BDR_ID, ENETC_TBLENR,
			tx_bdr->bd_count);

	/* reset both producer/consumer indexes */
	enetc_write_reg(tx_bdr->cons_idx, tx_bdr->next_cons_idx);
	enetc_write_reg(tx_bdr->prod_idx, tx_bdr->next_prod_idx);

	/* enable TX ring */
	enetc_bdr_write(hw, TX, ENETC_TX_BDR_ID, ENETC_TBMR, ENETC_TBMR_EN);
}

/**
 * Setup a single Rx BD Ring (ID = 0):
 * - set Rx buffer descriptors address (one descriptor per buffer)
 * - set buffer size as max frame size
 * - enable Rx ring
 * - reset consumer and producer indexes
 * - set buffer for each descriptor
 */
static void enetc_setup_rx_bdr(struct udevice *dev, struct enetc_devfn *hw)
{
	struct bd_ring *rx_bdr = &hw->rx_bdr;
	u64 rx_bd_add = (u64)enetc_rxbd;
	int i;

	/* used later to advance to the next BD produced by ENETC HW */
	rx_bdr->bd_count = ENETC_BD_CNT;
	rx_bdr->next_prod_idx = 0;
	rx_bdr->next_cons_idx = ENETC_RBCI_INIT;
	rx_bdr->cons_idx = hw->regs_base +
				ENETC_BDR(RX, ENETC_RX_BDR_ID, ENETC_RBCIR);
	rx_bdr->prod_idx = hw->regs_base +
				ENETC_BDR(RX, ENETC_RX_BDR_ID, ENETC_RBPIR);

	/* set Rx BD address */
	enetc_bdr_write(hw, RX, ENETC_RX_BDR_ID, ENETC_RBBAR0,
			lower_32_bits(rx_bd_add));
	enetc_bdr_write(hw, RX, ENETC_RX_BDR_ID, ENETC_RBBAR1,
			upper_32_bits(rx_bd_add));
	/* set Rx BD count (multiple of 8) */
	enetc_bdr_write(hw, RX, ENETC_RX_BDR_ID, ENETC_RBLENR,
			rx_bdr->bd_count);
	/* set Rx buffer  size */
	enetc_bdr_write(hw, RX, ENETC_RX_BDR_ID, ENETC_RBBSR, ENETC_BUFF_SIZE);

	/* reset producer (ENETC owned) and consumer (SW owned) index */
	enetc_write_reg(rx_bdr->cons_idx, rx_bdr->next_cons_idx);
	enetc_write_reg(rx_bdr->prod_idx, rx_bdr->next_prod_idx);

	/* fill Rx BD */
	memset(enetc_rxbd, 0, rx_bdr->bd_count * sizeof(union enetc_rx_bd));
	for (i = 0; i < rx_bdr->bd_count; i++) {
		enetc_rxbd[i].w.addr = enetc_rxb_address(dev, i);
		/* each RX buffer must be aligned to 64B */
		WARN_ON(enetc_rxbd[i].w.addr & (ENETC_ALIGN - 1));
	}
	/* enable Rx ring */
	enetc_bdr_write(hw, RX, ENETC_RX_BDR_ID, ENETC_RBMR, ENETC_RBMR_EN);
}

/**
 * Start ENETC port:
 * - run function lever reset (FLR)
 * - enable access to port and SI registers
 * - set mac address
 * - setup TX/RX buffer descriptors
 * - enable Tx/Rx rings
 */
static int enetc_start(struct udevice *dev)
{
	struct eth_pdata *plat = dev_get_platdata(dev);
	struct enetc_devfn *hw = dev_get_priv(dev);
	int ret;

	ENETC_DBG(hw, "starting ...\n");

	/* run FLR on current PF */
	ENETC_DBG(hw, "reseting ...\n");
	ret = enetc_pf_flr(dev);
	if (ret < 0)
		ENETC_ERR(hw, "failed to reset\n");

	/* enable issue memory I/O requests by this PF - required after FLR */
	dm_pci_write_config16(dev, PCI_CFH_CMD, PCI_CFH_CMD_IO_MEM_EN);

	if (!is_valid_ethaddr(plat->enetaddr)) {
		ENETC_DBG(hw, "invalid MAC address, generate random ...\n");
		net_random_ethaddr(plat->enetaddr);
	}
	enetc_set_primary_mac_addr(hw, plat->enetaddr);

	ENETC_DBG(hw, "enabling port and rings ...\n");
	enetc_enable_si_port(hw);

	/* setup Tx/Rx buffer descriptors */
	enetc_setup_tx_bdr(hw);
	enetc_setup_rx_bdr(dev, hw);

	return 0;
}

/**
 * Stop ENETC port:
 * - disable Tx/Rx rings
 * - disable port and SI
 */
static void enetc_stop(struct udevice *dev)
{
	struct enetc_devfn *hw = dev_get_priv(dev);
	u16 v = 0;

	ENETC_DBG(hw, "stopping ...\n");

	/* check if this device was started and we have access to it */
	dm_pci_read_config16(dev, PCI_CFH_CMD, &v);
	if ((v & PCI_CFH_CMD_IO_MEM_EN) != PCI_CFH_CMD_IO_MEM_EN)
		return;

	ENETC_DBG(hw, "disabling port and rings ...\n");

	/* disable Tx/Rx rings */
	enetc_bdr_write(hw, TX, ENETC_TX_BDR_ID, ENETC_TBMR, 0);
	enetc_bdr_write(hw, RX, ENETC_RX_BDR_ID, ENETC_RBMR, 0);

	/* disable port and SI */
	enetc_disable_si_port(hw);
}

/**
 * ENETC transmit packet:
 * - check if Tx BD ring is full
 * - set buffer/packet address (dma address)
 * - set final fragment flag
 * - try while producer index equals consumer index or timeout
 */
static int enetc_send(struct udevice *dev, void *packet, int length)
{
	struct enetc_devfn *hw = dev_get_priv(dev);
	struct bd_ring *txr = &hw->tx_bdr;
	void *nv_packet = (void *)packet;
	int tries = ENETC_POLL_TRIES;
	u32 pi, ci;

	pi = txr->next_prod_idx;
	ci = enetc_read_reg(txr->cons_idx) & ENETC_BDR_IDX_MASK;
	/* Tx ring is full when */
	if (((pi + 1) % txr->bd_count) == ci) {
		ENETC_DBG(hw, "Tx BDR full\n");
		return -ETIMEDOUT;
	}
	ENETC_DBG(hw, "TxBD[%d]send: pkt_len=%d, buff @0x%x%08x\n", pi, length,
		  upper_32_bits((u64)nv_packet), lower_32_bits((u64)nv_packet));

	/* prepare Tx BD */
	memset(&enetc_txbd[pi], 0x0, sizeof(struct enetc_tx_bd));
	enetc_txbd[pi].addr =  cpu_to_le64(dm_pci_virt_to_mem(dev, nv_packet));
	enetc_txbd[pi].buf_len = cpu_to_le16(length);
	enetc_txbd[pi].frm_len = cpu_to_le16(length);
	enetc_txbd[pi].flags = cpu_to_le16(ENETC_TXBD_FLAGS_F);
	dmb();
	/* send frame: increment producer index */
	pi = (pi + 1) % txr->bd_count;
	txr->next_prod_idx = pi;
	enetc_write_reg(txr->prod_idx, pi);
	while ((--tries >= 0) &&
	       (pi != (enetc_read_reg(txr->cons_idx) & ENETC_BDR_IDX_MASK)))
		cpu_relax();

	return tries > 0 ? 0 : -ETIMEDOUT;
}

/**
 * Handles frame receive and cleans up the BD slot on the Rx ring.
 */
static int enetc_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct enetc_devfn *hw = dev_get_priv(dev);
	struct bd_ring *rxr = &hw->rx_bdr;
	int tries = ENETC_POLL_TRIES;
	int pi = rxr->next_prod_idx;
	int ci = rxr->next_cons_idx;
	u32 status;
	int len;
	u8 rdy;

	do {
		dmb();
		status = le32_to_cpu(enetc_rxbd[pi].r.lstatus);
		/* check if current BD is ready to be consumed */
		rdy = ENETC_RXBD_STATUS_R(status);
	} while (--tries >= 0 && !rdy);

	if (!rdy)
		return -EAGAIN;

	dmb();
	len = le16_to_cpu(enetc_rxbd[pi].r.buf_len);
	*packetp = (uchar *)enetc_rxb_address(dev, pi);
	ENETC_DBG(hw, "RxBD[%d]: len=%d err=%d pkt=0x%x%08x\n", pi, len,
		  ENETC_RXBD_STATUS_ERRORS(status),
		  upper_32_bits((u64)*packetp), lower_32_bits((u64)*packetp));

	/* BD clean up and advance to next in ring */
	memset(&enetc_rxbd[pi], 0, sizeof(union enetc_rx_bd));
	enetc_rxbd[pi].w.addr = enetc_rxb_address(dev, pi);
	rxr->next_prod_idx = (pi + 1) % rxr->bd_count;
	ci = (ci + 1) % rxr->bd_count;
	rxr->next_cons_idx = ci;
	dmb();
	/* let ENETC HW know we free up the slot in the ring */
	enetc_write_reg(rxr->cons_idx, ci);

	return len;
}

static const struct dm_pci_ops pcie_enetc_ops = {
	.read_config	= pcie_enetc_ecam_read_config,
	.write_config	= pcie_enetc_ecam_write_config,
};

static const struct eth_ops enetc_ops = {
	.start	= enetc_start,
	.send	= enetc_send,
	.recv	= enetc_recv,
	.stop	= enetc_stop,
};

static const struct udevice_id ls_enetc_rcie_ids[] = {
	{ .compatible = "fsl,ls-enetc-rcie" },
	{ }
};

static const struct udevice_id ls_enetc_eth_ids[] = {
	{ .compatible = "fsl,ls-enetc-eth" },
	{ }
};

U_BOOT_DRIVER(pci_enetc) = {
	.name	= "ls_enetc_rcie",
	.id		= UCLASS_PCI,
	.of_match	= ls_enetc_rcie_ids,
	.ops	= &pcie_enetc_ops,
	.ofdata_to_platdata	= pcie_enetc_ofdata_to_platdata,
	.priv_auto_alloc_size	= sizeof(struct enetc_rcie),
};

U_BOOT_DRIVER(eth_enetc) = {
	.name	= "ls_enetc_eth",
	.id		= UCLASS_ETH,
	.of_match	= ls_enetc_eth_ids,
	.bind	= enetc_bind,
	.probe	= enetc_probe,
	.remove = enetc_remove,
	.ops	= &enetc_ops,
	.priv_auto_alloc_size = sizeof(struct enetc_devfn),
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
};

U_BOOT_PCI_DEVICE(pci_enetc, enetc_ids);
U_BOOT_PCI_DEVICE(eth_enetc, enetc_ids);
