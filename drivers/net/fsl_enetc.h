/*
 * ENETC ethernet controller driver
 *
 * Copyright 2017-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DEBUG
#define ENETC_DEBUG
#endif

#define ENETC_ERR(hw, fmt, args...) \
	printf("enetc[%u]: ERROR: " fmt, (hw)->devno, ##args)

#ifdef ENETC_DEBUG
#define ENETC_DBG(hw, fmt, args...) \
	printf("enetc[%u]: DEBUG: " fmt, (hw)->devno, ##args)
#else
#define ENETC_DBG(devno, args...)	do { } while (0)
#endif

/* PCI and RCIE */
#define PCIE_CONFIG_SPACE_SIZE 0x1000
#define ENETC_PF_HDR_ADD(b, d, f) ((((b) << 8) + ((d) << 3) + (f)) \
					* PCIE_CONFIG_SPACE_SIZE)
/* PCI Config header command */
#define PCI_CFH_CMD 0x4
/* Enable memory access */
#define PCI_CFH_CMD_IO_MEM_EN (BIT(1) | BIT(2))

/* PCIe device capabilities */
#define PCI_CFC_PCIE_DEV_CAP 0x44
#define PCI_CFC_PCIE_FLR_CAP_ID BIT(28)
#define PCI_CFC_PCIE_DEV_CTL 0x48
#define PCI_CFC_PCIE_DEV_CTL_INIT_FLR BIT(15)
#define PCI_CFC_PCIE_DEV_STAT 0x4a
#define PCI_CFC_PCIE_DEV_STAT_TRANS_PEND BIT(5)

/* After initiating FLR SW must wait before reading the reset status */
#define ENETC_DEV_FLR_WAIT_MS 100

/* PCI Enhanced Allocation capabilities */
#define PCI_CFC_EA_CAP_BASE 0x9c
/* Hardwired value for EA capability */
#define PCI_CFC_EA_CAP_ID 0x14
#define PCI_CFC_EA_CAP 0x9e
/* number of BARs of PF for ENETC ports */
#define ENETC_PF_HDR_T0_NUM_BAR 4

/* BAR 0 ID for device regs. access */
#define PCI_EA_PF_BEI_BAR0 0

/* offsets within PCI EA capability structures only */
#define PCI_EA_PF_REG_FORMAT (PCI_CFC_EA_CAP_BASE + 0x4)
#define PCI_EA_PF_BAR_LOW (PCI_CFC_EA_CAP_BASE + 0x8)
#define PCI_EA_PF_BAR_SIZE (PCI_CFC_EA_CAP_BASE + 0xc)
#define PCI_EA_PF_BAR_HIGH (PCI_CFC_EA_CAP_BASE + 0x10)
#define PCI_EA_PF_REG_FORMAT_BEI(x) (((x) >> 4) & 0xf)
#define PCI_EA_PF_REG_FORMAT_ENABLE(x) (((x) >> 31) & 0x1)
#define PCI_EA_PF_BASE_REG(x) ((x) & ~0x3)
#define PCI_EA_PF_SIZE(x) ((x) & ~0x3)
#define PCI_EA_PF_BASE_REG_IS_64BIT(x) ((x) & 0x2)

/* LS1028 - IEPs use stream IDs starting from 0x4000 */
#define ENETC_IERB_STREAMID_START 0x4000
#define ENETC_IERB_STREAMID_END   0x400e

/* IERB reg info */
#define ENETC_IERB_ADDR 0x1F0800000
#define ENETC_IERB_SIZE 0x4000000
#define ENETC_NUM_PFS 7
#define ENETC_IERB_PFAMQ(pf, si) ((void *)(ENETC_IERB_ADDR + 0x800 + \
					(pf) * 0x1000 + (si) * 4))
#define ENETC_IERB_MSICAR	((void *)ENETC_IERB_ADDR + 0xa400)
#define ENETC_MSICAR_VALUE	0x30
#define ENETC_IERB_PMAR(i, port, s) ((void *)(ENETC_IERB_ADDR + 0x8000 + \
					(i) * 4 + (port) * 0x100 + (si) * 8))

/* PCI Root Complex info */
struct enetc_rcie {
	const char *name;
	int busno;
	void *cfg_base;
};

/* ENETC controller registers */

/* Station interface register offsets */
#define ENETC_SIMR	0
#define ENETC_SIMR_EN	BIT(31)
#define ENETC_SICAR0	0x40
/* write cache cfg: snoop, no allocate, update full (data), partial (BD) */
#define ENETC_SICAR_WR_CFG	0x6767
/* read cache cfg: coherent copy, look up, no allocate */
#define ENETC_SICAR_RD_CFG	0x27270000

#define ENETC_SIROCT	0x300
#define ENETC_SIRFRM	0x308
#define ENETC_SITOCT	0x320
#define ENETC_SITFRM	0x328

/* Station Interface Rx/Tx Buffer Descriptor Ring registers */
enum enetc_bdr_type {TX, RX};
#define ENETC_BDR(type, n, off)	(0x8000 + (type) * 0x100 + (n) * 0x200 + (off))
#define ENETC_BDR_IDX_MASK	0xffff

/* Rx BDR reg offsets */
#define ENETC_RBMR	0
#define ENETC_RBMR_EN	BIT(31)
#define ENETC_RBBSR	0x8
#define ENETC_RBCIR	0xc
/* initial consumer index for Rx BDR */
#define ENETC_RBCI_INIT 4
#define ENETC_RBBAR0	0x10
#define ENETC_RBBAR1	0x14
#define ENETC_RBPIR	0x18
#define ENETC_RBLENR	0x20

/* Tx BDR reg offsets */
#define ENETC_TBMR	0
#define ENETC_TBMR_EN	BIT(31)
#define ENETC_TBBAR0	0x10
#define ENETC_TBBAR1	0x14
#define ENETC_TBPIR	0x18
#define ENETC_TBCIR	0x1c
#define ENETC_TBLENR	0x20

/* Port registers offset */
#define ENETC_PORT_REGS_OFF 0x10000
#define ENETC_PMR	0x00000

#define ENETC_PMR_SI0_EN	BIT(16)
#define ENETC_PSIPMMR	0x18
#define ENETC_PSIPMAR0(n)	(0x00100 + (n) * 0x20)
#define ENETC_PSIPMAR1(n)	(0x00104 + (n) * 0x20)
#define ENETC_PSICFGR(n)	(0x00940 + (n) * 0x10)
#define ENETC_PSICFGR_SET_TXBDR(val)	((val) & 0xff)
#define ENETC_PSICFGR_SET_RXBDR(val)	(((val) & 0xff) << 16)
#define ENETC_EMDIO_CFG 0x1c00
#define ENETC_PM_CC	0x8008
/* Port config: enable MAC Tx/Rx, Tx padding, MAC promisc */
#define ENETC_PM_CC_DEFAULT 0x810
#define ENETC_PM_CC_RX_TX_EN 0x8813
#define ENETC_PM_MAXFRM	0x8014
#define ENETC_RX_MAXFRM_SIZE	PKTSIZE_ALIGN
#define ENETC_BUFF_SIZE	PKTSIZE_ALIGN
/* buffer descriptors count must be multiple of 8 and aligned to 128 bytes */
#define ENETC_BD_CNT 16
#define ENETC_ALIGN 128
#define ENETC_RX_MBUFF_SIZE (ENETC_BD_CNT * ENETC_RX_MAXFRM_SIZE)

/* single pair of Rx/Tx rings */
#define ENETC_RX_BDR_CNT 1
#define ENETC_TX_BDR_CNT 1
#define ENETC_RX_BDR_ID 0
#define ENETC_TX_BDR_ID 0

/* Tx buffer descriptor */
struct enetc_tx_bd {
	__le64 addr;
	__le16 buf_len;
	__le16 frm_len;
	__le16 err_csum;
	__le16 flags;
};

#define ENETC_TXBD_FLAGS_F	BIT(15)
#define ENETC_POLL_TRIES 0x8000

/* Rx buffer descriptor */
union enetc_rx_bd {
	/* SW provided BD format */
	struct {
		__le64 addr;
		u8 reserved[8];
	} w;

	/* ENETC returned BD format */
	struct {
		__le16 inet_csum;
		__le16 parse_summary;
		__le32 rss_hash;
		__le16 buf_len;
		__le16 vlan_opt;
		union {
			struct {
				__le16 flags;
				__le16 error;
			};
			__le32 lstatus;
		};
	} r;
};

#define ENETC_RXBD_STATUS_R(status)	(((status) >> 30) & 0x1)
#define ENETC_RXBD_STATUS_F(status)	(((status) >> 31) & 0x1)
#define ENETC_RXBD_STATUS_ERRORS(status)	(((status) >> 16) & 0xff)
#define ENETC_RXBD_STATUS(flags)	((flags) << 16)

/* Tx/Rx ring info */
struct bd_ring {
	void *cons_idx;
	void *prod_idx;
	/* next BD index to use */
	int next_prod_idx;
	int next_cons_idx;
	int bd_count;
};

/* ENETC HW access info */
struct enetc_devfn {
	const char *name; /* device name */
	int devno; /* */
	void *regs_base; /* base ENETC registers */
	u32 regs_size;
	void *port_regs; /* base ENETC port registers */

	/* Rx/Tx buffer descriptor rings info */
	struct bd_ring tx_bdr;
	struct bd_ring rx_bdr;

	struct phy_device *phydev;
	phy_interface_t phy_intf;
	int phy_addr;
	struct mii_dev *bus;
};

/* register accessors */
#define enetc_read_reg(x)	readl((x))
#define enetc_write_reg(x, val)	writel((val), (x))
#define enetc_read(hw, off)	enetc_read_reg((hw)->regs_base + (off))
#define enetc_write(hw, off, v)	enetc_write_reg((hw)->regs_base + (off), v)

/* port register accessors */
#define enetc_port_regs(hw, off) ((hw)->port_regs + (off))
#define enetc_read_port(hw, off) enetc_read_reg(enetc_port_regs((hw), (off)))
#define enetc_write_port(hw, off, v) \
				enetc_write_reg(enetc_port_regs((hw), (off)), v)

/* BDR register accessors, see ENETC_BDR() */
#define enetc_bdr_read(hw, t, n, off) \
				enetc_read(hw, ENETC_BDR(t, n, off))
#define enetc_bdr_write(hw, t, n, off, val) \
				enetc_write(hw, ENETC_BDR(t, n, off), val)

