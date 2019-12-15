// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2016-2018,2020 NXP
 * Copyright 2018, Sensor-Technik Wiedemann GmbH
 *
 * Ported from Linux (drivers/net/dsa/sja1105/).
 */

#include <common.h>
#include <linux/if_vlan.h>
#include <linux/packing.h>
#include <linux/bitrev.h>
#include <net/dsa.h>
#include <stdlib.h>
#include <spi.h>

#define ETHER_CRC32_POLY				0x04C11DB7
#define ETH_P_SJA1105					0xdadb
#define SJA1105_NUM_PORTS				5
#define SJA1105_NUM_TC					8
#define SJA1105ET_FDB_BIN_SIZE				4
#define SJA1105_SIZE_CGU_CMD				4
#define SJA1105_SIZE_RESET_CMD				4
#define SJA1105_SIZE_SPI_MSG_HEADER			4
#define SJA1105_SIZE_SPI_MSG_MAXLEN			(64 * 4)
#define SJA1105_SIZE_DEVICE_ID				4
#define SJA1105_SIZE_TABLE_HEADER			12
#define SJA1105_SIZE_L2_POLICING_ENTRY			8
#define SJA1105_SIZE_VLAN_LOOKUP_ENTRY			8
#define SJA1105_SIZE_L2_FORWARDING_ENTRY		8
#define SJA1105_SIZE_L2_FORWARDING_PARAMS_ENTRY		12
#define SJA1105_SIZE_XMII_PARAMS_ENTRY			4
#define SJA1105ET_SIZE_MAC_CONFIG_ENTRY			28
#define SJA1105ET_SIZE_L2_LOOKUP_PARAMS_ENTRY		4
#define SJA1105ET_SIZE_GENERAL_PARAMS_ENTRY		40
#define SJA1105PQRS_SIZE_MAC_CONFIG_ENTRY		32
#define SJA1105PQRS_SIZE_L2_LOOKUP_PARAMS_ENTRY		16
#define SJA1105PQRS_SIZE_GENERAL_PARAMS_ENTRY		44

#define SJA1105_MAX_L2_LOOKUP_COUNT			1024
#define SJA1105_MAX_L2_POLICING_COUNT			45
#define SJA1105_MAX_VLAN_LOOKUP_COUNT			4096
#define SJA1105_MAX_L2_FORWARDING_COUNT			13
#define SJA1105_MAX_MAC_CONFIG_COUNT			5
#define SJA1105_MAX_L2_LOOKUP_PARAMS_COUNT		1
#define SJA1105_MAX_L2_FORWARDING_PARAMS_COUNT		1
#define SJA1105_MAX_GENERAL_PARAMS_COUNT		1
#define SJA1105_MAX_XMII_PARAMS_COUNT			1

#define SJA1105_SGMII_PORT				4

/* DIGITAL_CONTROL_1 (address 1f8000h) */
#define SJA1105_DC1_EN_VSMMD1				BIT(13)
#define SJA1105_DC1_CLOCK_STOP_EN			BIT(10)
#define SJA1105_DC1_MAC_AUTO_SW				BIT(9)
#define SJA1105_DC1_INIT				BIT(8)

/* DIGITAL_CONTROL_2 register (address 1f80e1h) */
#define SJA1105_DC2_TX_POL_INV_DISABLE			BIT(4)
#define SJA1105_DC2_RX_POL_INV				BIT(0)

/* AUTONEG_CONTROL register (address 1f8001h) */
#define SJA1105_AC_SGMII_LINK				BIT(4)
#define SJA1105_AC_PHY_MODE				BIT(3)
#define SJA1105_AC_AUTONEG_MODE_SGMII			(2 << 1)

#define SJA1105_MAX_FRAME_MEMORY			929

#define SJA1105E_DEVICE_ID				0x9C00000Cull
#define SJA1105T_DEVICE_ID				0x9E00030Eull
#define SJA1105PR_DEVICE_ID				0xAF00030Eull
#define SJA1105QS_DEVICE_ID				0xAE00030Eull

#define SJA1105ET_PART_NO				0x9A83
#define SJA1105P_PART_NO				0x9A84
#define SJA1105Q_PART_NO				0x9A85
#define SJA1105R_PART_NO				0x9A86
#define SJA1105S_PART_NO				0x9A87

#define DSA_8021Q_DIR_RX		BIT(10)
#define DSA_8021Q_PORT_SHIFT		0
#define DSA_8021Q_PORT_MASK		GENMASK(3, 0)
#define DSA_8021Q_PORT(x)		(((x) << DSA_8021Q_PORT_SHIFT) & \
						 DSA_8021Q_PORT_MASK)

#define SJA1105_RATE_MBPS(speed) (((speed) * 64000) / 1000)

/* UM10944.pdf Page 11, Table 2. Configuration Blocks */
enum {
	BLKID_L2_POLICING				= 0x06,
	BLKID_VLAN_LOOKUP				= 0x07,
	BLKID_L2_FORWARDING				= 0x08,
	BLKID_MAC_CONFIG				= 0x09,
	BLKID_L2_LOOKUP_PARAMS				= 0x0D,
	BLKID_L2_FORWARDING_PARAMS			= 0x0E,
	BLKID_GENERAL_PARAMS				= 0x11,
	BLKID_XMII_PARAMS				= 0x4E,
};

enum sja1105_blk_idx {
	BLK_IDX_L2_POLICING = 0,
	BLK_IDX_VLAN_LOOKUP,
	BLK_IDX_L2_FORWARDING,
	BLK_IDX_MAC_CONFIG,
	BLK_IDX_L2_LOOKUP_PARAMS,
	BLK_IDX_L2_FORWARDING_PARAMS,
	BLK_IDX_GENERAL_PARAMS,
	BLK_IDX_XMII_PARAMS,
	BLK_IDX_MAX,
};

struct sja1105_general_params_entry {
	u64 mac_fltres1;
	u64 mac_fltres0;
	u64 mac_flt1;
	u64 mac_flt0;
	u64 casc_port;
	u64 host_port;
	u64 mirr_port;
	u64 tpid;
	u64 tpid2;
};

struct sja1105_vlan_lookup_entry {
	u64 vmemb_port;
	u64 vlan_bc;
	u64 tag_port;
	u64 vlanid;
};

struct sja1105_l2_lookup_params_entry {
	u64 maxaddrp[5];
	u64 start_dynspc;
	u64 use_static;
	u64 owr_dyn;
	u64 dyn_tbsz;
	u64 poly;
};

struct sja1105_l2_forwarding_entry {
	u64 bc_domain;
	u64 reach_port;
	u64 fl_domain;
};

struct sja1105_l2_forwarding_params_entry {
	u64 part_spc[8];
};

struct sja1105_l2_policing_entry {
	u64 sharindx;
	u64 smax;
	u64 rate;
	u64 maxlen;
	u64 partition;
};

struct sja1105_mac_config_entry {
	u64 top[8];
	u64 base[8];
	u64 enabled[8];
	u64 speed;
	u64 maxage;
	u64 vlanprio;
	u64 vlanid;
	u64 dyn_learn;
	u64 egress;
	u64 ingress;
};

struct sja1105_xmii_params_entry {
	u64 phy_mac[5];
	u64 xmii_mode[5];
};

struct sja1105_table_header {
	u64 block_id;
	u64 len;
	u64 crc;
};

struct sja1105_table_ops {
	size_t (*packing)(void *buf, void *entry_ptr, enum packing_op op);
	size_t unpacked_entry_size;
	size_t packed_entry_size;
	size_t max_entry_count;
};

struct sja1105_table {
	const struct sja1105_table_ops *ops;
	size_t entry_count;
	void *entries;
};

struct sja1105_static_config {
	u64 device_id;
	struct sja1105_table tables[BLK_IDX_MAX];
};

struct sja1105_sgmii_cfg {
	bool inband_an;
	u16 pcs_speed;
};

struct sja1105_private {
	struct sja1105_static_config static_config;
	bool rgmii_rx_delay[SJA1105_NUM_PORTS];
	bool rgmii_tx_delay[SJA1105_NUM_PORTS];
	int phy_modes[SJA1105_NUM_PORTS];
	u16 pvid[SJA1105_NUM_PORTS];
	struct sja1105_sgmii_cfg sgmii_cfg;
	const struct sja1105_info *info;
	struct udevice *dev;
};

typedef enum {
	SPI_READ = 0,
	SPI_WRITE = 1,
} sja1105_spi_rw_mode_t;

typedef enum {
	XMII_MAC = 0,
	XMII_PHY = 1,
} sja1105_mii_role_t;

typedef enum {
	XMII_MODE_MII		= 0,
	XMII_MODE_RMII		= 1,
	XMII_MODE_RGMII		= 2,
	XMII_MODE_SGMII		= 3,
} sja1105_phy_interface_t;

typedef enum {
	SJA1105_SPEED_10MBPS	= 3,
	SJA1105_SPEED_100MBPS	= 2,
	SJA1105_SPEED_1000MBPS	= 1,
} sja1105_speed_t;

/* Keeps the different addresses between E/T and P/Q/R/S */
struct sja1105_regs {
	u64 device_id;
	u64 prod_id;
	u64 status;
	u64 port_control;
	u64 rgu;
	u64 config;
	u64 sgmii;
	u64 rmii_pll1;
	u64 pad_mii_tx[SJA1105_NUM_PORTS];
	u64 pad_mii_id[SJA1105_NUM_PORTS];
	u64 cgu_idiv[SJA1105_NUM_PORTS];
	u64 mii_tx_clk[SJA1105_NUM_PORTS];
	u64 mii_rx_clk[SJA1105_NUM_PORTS];
	u64 mii_ext_tx_clk[SJA1105_NUM_PORTS];
	u64 mii_ext_rx_clk[SJA1105_NUM_PORTS];
	u64 rgmii_tx_clk[SJA1105_NUM_PORTS];
	u64 rmii_ref_clk[SJA1105_NUM_PORTS];
	u64 rmii_ext_tx_clk[SJA1105_NUM_PORTS];
};

struct sja1105_info {
	u64 device_id;
	u64 part_no;
	const struct sja1105_table_ops *static_ops;
	const struct sja1105_regs *regs;
	int (*reset_cmd)(struct sja1105_private *priv);
	int (*setup_rgmii_delay)(struct sja1105_private *priv, int port);
	const char *name;
};

struct sja1105_chunk {
	u8	*buf;
	size_t	len;
	u64	reg_addr;
};

struct sja1105_spi_message {
	u64 access;
	u64 read_count;
	u64 address;
};

struct sja1105_cfg_pad_mii_tx {
	u64 d32_os;
	u64 d32_ipud;
	u64 d10_os;
	u64 d10_ipud;
	u64 ctrl_os;
	u64 ctrl_ipud;
	u64 clk_os;
	u64 clk_ih;
	u64 clk_ipud;
};

struct sja1105_cfg_pad_mii_id {
	u64 rxc_stable_ovr;
	u64 rxc_delay;
	u64 rxc_bypass;
	u64 rxc_pd;
	u64 txc_stable_ovr;
	u64 txc_delay;
	u64 txc_bypass;
	u64 txc_pd;
};

struct sja1105_cgu_idiv {
	u64 clksrc;
	u64 autoblock;
	u64 idiv;
	u64 pd;
};

struct sja1105_cgu_pll_ctrl {
	u64 pllclksrc;
	u64 msel;
	u64 autoblock;
	u64 psel;
	u64 direct;
	u64 fbsel;
	u64 bypass;
	u64 pd;
};

enum {
	CLKSRC_MII0_TX_CLK	= 0x00,
	CLKSRC_MII0_RX_CLK	= 0x01,
	CLKSRC_MII1_TX_CLK	= 0x02,
	CLKSRC_MII1_RX_CLK	= 0x03,
	CLKSRC_MII2_TX_CLK	= 0x04,
	CLKSRC_MII2_RX_CLK	= 0x05,
	CLKSRC_MII3_TX_CLK	= 0x06,
	CLKSRC_MII3_RX_CLK	= 0x07,
	CLKSRC_MII4_TX_CLK	= 0x08,
	CLKSRC_MII4_RX_CLK	= 0x09,
	CLKSRC_PLL0		= 0x0B,
	CLKSRC_PLL1		= 0x0E,
	CLKSRC_IDIV0		= 0x11,
	CLKSRC_IDIV1		= 0x12,
	CLKSRC_IDIV2		= 0x13,
	CLKSRC_IDIV3		= 0x14,
	CLKSRC_IDIV4		= 0x15,
};

struct sja1105_cgu_mii_ctrl {
	u64 clksrc;
	u64 autoblock;
	u64 pd;
};

static void sja1105_packing(void *buf, u64 *val, int start, int end,
			    size_t len, enum packing_op op)
{
	int rc;

	rc = packing(buf, val, start, end, len, op, QUIRK_LSW32_IS_FIRST);
	if (likely(!rc))
		return;

	printf("Invalid use of packing API: start %d end %d returned %d\n",
	       start, end, rc);
}

static u32 crc32_add(u32 crc, u8 byte)
{
	u32 byte32 = bitrev32(byte);
	int i;

	for (i = 0; i < 8; i++) {
		if ((crc ^ byte32) & BIT(31)) {
			crc <<= 1;
			crc ^= ETHER_CRC32_POLY;
		} else {
			crc <<= 1;
		}
		byte32 <<= 1;
	}
	return crc;
}

/* Little-endian Ethernet CRC32 of data packed as big-endian u32 words */
static uint32_t sja1105_crc32(void *buf, size_t len)
{
	unsigned int i;
	u64 chunk;
	u32 crc;

	/* seed */
	crc = 0xFFFFFFFF;
	for (i = 0; i < len; i += 4) {
		sja1105_packing(buf + i, &chunk, 31, 0, 4, UNPACK);
		crc = crc32_add(crc, chunk & 0xFF);
		crc = crc32_add(crc, (chunk >> 8) & 0xFF);
		crc = crc32_add(crc, (chunk >> 16) & 0xFF);
		crc = crc32_add(crc, (chunk >> 24) & 0xFF);
	}
	return bitrev32(~crc);
}

static void sja1105_spi_message_pack(void *buf, struct sja1105_spi_message *msg)
{
	const int size = SJA1105_SIZE_SPI_MSG_HEADER;

	memset(buf, 0, size);

	sja1105_packing(buf, &msg->access,     31, 31, size, PACK);
	sja1105_packing(buf, &msg->read_count, 30, 25, size, PACK);
	sja1105_packing(buf, &msg->address,    24,  4, size, PACK);
}

static int sja1105_xfer_buf(const struct sja1105_private *priv,
			    sja1105_spi_rw_mode_t rw, u64 reg_addr,
			    u8 *buf, size_t len)
{
	struct udevice *dev = priv->dev;
	struct sja1105_chunk chunk = {
		.len = min_t(size_t, len, SJA1105_SIZE_SPI_MSG_MAXLEN),
		.reg_addr = reg_addr,
		.buf = buf,
	};
	int num_chunks;
	int rc, i;

	rc = dm_spi_claim_bus(dev);
	if (rc)
		return rc;

	num_chunks = DIV_ROUND_UP(len, SJA1105_SIZE_SPI_MSG_MAXLEN);

	for (i = 0; i < num_chunks; i++) {
		u8 hdr_buf[SJA1105_SIZE_SPI_MSG_HEADER];
		struct sja1105_spi_message msg;
		u8 *rx_buf = NULL;
		u8 *tx_buf = NULL;

		/* Populate the transfer's header buffer */
		msg.address = chunk.reg_addr;
		msg.access = rw;
		if (rw == SPI_READ)
			msg.read_count = chunk.len / 4;
		else
			/* Ignored */
			msg.read_count = 0;
		sja1105_spi_message_pack(hdr_buf, &msg);
		rc = dm_spi_xfer(dev, SJA1105_SIZE_SPI_MSG_HEADER * 8, hdr_buf,
				 NULL, SPI_XFER_BEGIN);
		if (rc)
			goto out;

		/* Populate the transfer's data buffer */
		if (rw == SPI_READ)
			rx_buf = chunk.buf;
		else
			tx_buf = chunk.buf;
		rc = dm_spi_xfer(dev, chunk.len * 8, tx_buf, rx_buf,
				 SPI_XFER_END);
		if (rc)
			goto out;

		/* Calculate next chunk */
		chunk.buf += chunk.len;
		chunk.reg_addr += chunk.len / 4;
		chunk.len = min_t(size_t, (ptrdiff_t)(buf + len - chunk.buf),
				  SJA1105_SIZE_SPI_MSG_MAXLEN);
	}

out:
	dm_spi_release_bus(dev);

	return rc;
}

static int sja1105et_reset_cmd(struct sja1105_private *priv)
{
	const struct sja1105_regs *regs = priv->info->regs;
	u8 packed_buf[SJA1105_SIZE_RESET_CMD] = {0};
	const int size = SJA1105_SIZE_RESET_CMD;
	u64 cold_rst = 1;

	sja1105_packing(packed_buf, &cold_rst, 3, 3, size, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->rgu, packed_buf,
				SJA1105_SIZE_RESET_CMD);
}

static int sja1105pqrs_reset_cmd(struct sja1105_private *priv)
{
	const struct sja1105_regs *regs = priv->info->regs;
	u8 packed_buf[SJA1105_SIZE_RESET_CMD] = {0};
	const int size = SJA1105_SIZE_RESET_CMD;
	u64 cold_rst = 1;

	sja1105_packing(packed_buf, &cold_rst, 2, 2, size, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->rgu, packed_buf,
				SJA1105_SIZE_RESET_CMD);
}

static size_t sja1105et_general_params_entry_packing(void *buf, void *entry_ptr,
						     enum packing_op op)
{
	const size_t size = SJA1105ET_SIZE_GENERAL_PARAMS_ENTRY;
	struct sja1105_general_params_entry *entry = entry_ptr;

	sja1105_packing(buf, &entry->mac_fltres1, 311, 264, size, op);
	sja1105_packing(buf, &entry->mac_fltres0, 263, 216, size, op);
	sja1105_packing(buf, &entry->mac_flt1,    215, 168, size, op);
	sja1105_packing(buf, &entry->mac_flt0,    167, 120, size, op);
	sja1105_packing(buf, &entry->casc_port,   115, 113, size, op);
	sja1105_packing(buf, &entry->host_port,   112, 110, size, op);
	sja1105_packing(buf, &entry->mirr_port,   109, 107, size, op);
	sja1105_packing(buf, &entry->tpid,         42,  27, size, op);
	sja1105_packing(buf, &entry->tpid2,        25,  10, size, op);
	return size;
}

static size_t
sja1105pqrs_general_params_entry_packing(void *buf, void *entry_ptr,
					 enum packing_op op)
{
	const size_t size = SJA1105PQRS_SIZE_GENERAL_PARAMS_ENTRY;
	struct sja1105_general_params_entry *entry = entry_ptr;

	sja1105_packing(buf, &entry->mac_fltres1, 343, 296, size, op);
	sja1105_packing(buf, &entry->mac_fltres0, 295, 248, size, op);
	sja1105_packing(buf, &entry->mac_flt1,    247, 200, size, op);
	sja1105_packing(buf, &entry->mac_flt0,    199, 152, size, op);
	sja1105_packing(buf, &entry->casc_port,   147, 145, size, op);
	sja1105_packing(buf, &entry->host_port,   144, 142, size, op);
	sja1105_packing(buf, &entry->mirr_port,   141, 139, size, op);
	sja1105_packing(buf, &entry->tpid,         74,  59, size, op);
	sja1105_packing(buf, &entry->tpid2,        57,  42, size, op);
	return size;
}

static size_t
sja1105_l2_forwarding_params_entry_packing(void *buf, void *entry_ptr,
					   enum packing_op op)
{
	const size_t size = SJA1105_SIZE_L2_FORWARDING_PARAMS_ENTRY;
	struct sja1105_l2_forwarding_params_entry *entry = entry_ptr;
	int offset, i;

	for (i = 0, offset = 13; i < 8; i++, offset += 10)
		sja1105_packing(buf, &entry->part_spc[i],
				offset + 9, offset + 0, size, op);
	return size;
}

static size_t sja1105_l2_forwarding_entry_packing(void *buf, void *entry_ptr,
						  enum packing_op op)
{
	const size_t size = SJA1105_SIZE_L2_FORWARDING_ENTRY;
	struct sja1105_l2_forwarding_entry *entry = entry_ptr;

	sja1105_packing(buf, &entry->bc_domain,  63, 59, size, op);
	sja1105_packing(buf, &entry->reach_port, 58, 54, size, op);
	sja1105_packing(buf, &entry->fl_domain,  53, 49, size, op);
	return size;
}

static size_t
sja1105et_l2_lookup_params_entry_packing(void *buf, void *entry_ptr,
					 enum packing_op op)
{
	const size_t size = SJA1105ET_SIZE_L2_LOOKUP_PARAMS_ENTRY;
	struct sja1105_l2_lookup_params_entry *entry = entry_ptr;

	sja1105_packing(buf, &entry->dyn_tbsz,       16, 14, size, op);
	sja1105_packing(buf, &entry->poly,           13,  6, size, op);
	return size;
}

static size_t
sja1105pqrs_l2_lookup_params_entry_packing(void *buf, void *entry_ptr,
					   enum packing_op op)
{
	const size_t size = SJA1105PQRS_SIZE_L2_LOOKUP_PARAMS_ENTRY;
	struct sja1105_l2_lookup_params_entry *entry = entry_ptr;
	int offset, i;

	for (i = 0, offset = 58; i < 5; i++, offset += 11)
		sja1105_packing(buf, &entry->maxaddrp[i],
				offset + 10, offset + 0, size, op);
	sja1105_packing(buf, &entry->start_dynspc,   42,  33, size, op);
	sja1105_packing(buf, &entry->use_static,     24,  24, size, op);
	sja1105_packing(buf, &entry->owr_dyn,        23,  23, size, op);
	return size;
}

static size_t sja1105_l2_policing_entry_packing(void *buf, void *entry_ptr,
						enum packing_op op)
{
	const size_t size = SJA1105_SIZE_L2_POLICING_ENTRY;
	struct sja1105_l2_policing_entry *entry = entry_ptr;

	sja1105_packing(buf, &entry->sharindx,  63, 58, size, op);
	sja1105_packing(buf, &entry->smax,      57, 42, size, op);
	sja1105_packing(buf, &entry->rate,      41, 26, size, op);
	sja1105_packing(buf, &entry->maxlen,    25, 15, size, op);
	sja1105_packing(buf, &entry->partition, 14, 12, size, op);
	return size;
}

static size_t sja1105et_mac_config_entry_packing(void *buf, void *entry_ptr,
						 enum packing_op op)
{
	const size_t size = SJA1105ET_SIZE_MAC_CONFIG_ENTRY;
	struct sja1105_mac_config_entry *entry = entry_ptr;
	int offset, i;

	for (i = 0, offset = 72; i < 8; i++, offset += 19) {
		sja1105_packing(buf, &entry->enabled[i],
				offset +  0, offset +  0, size, op);
		sja1105_packing(buf, &entry->base[i],
				offset +  9, offset +  1, size, op);
		sja1105_packing(buf, &entry->top[i],
				offset + 18, offset + 10, size, op);
	}
	sja1105_packing(buf, &entry->speed,     66, 65, size, op);
	sja1105_packing(buf, &entry->maxage,    32, 25, size, op);
	sja1105_packing(buf, &entry->vlanprio,  24, 22, size, op);
	sja1105_packing(buf, &entry->vlanid,    21, 10, size, op);
	sja1105_packing(buf, &entry->dyn_learn,  3,  3, size, op);
	sja1105_packing(buf, &entry->egress,     2,  2, size, op);
	sja1105_packing(buf, &entry->ingress,    1,  1, size, op);
	return size;
}

static size_t sja1105pqrs_mac_config_entry_packing(void *buf, void *entry_ptr,
						   enum packing_op op)
{
	const size_t size = SJA1105PQRS_SIZE_MAC_CONFIG_ENTRY;
	struct sja1105_mac_config_entry *entry = entry_ptr;
	int offset, i;

	for (i = 0, offset = 104; i < 8; i++, offset += 19) {
		sja1105_packing(buf, &entry->enabled[i],
				offset +  0, offset +  0, size, op);
		sja1105_packing(buf, &entry->base[i],
				offset +  9, offset +  1, size, op);
		sja1105_packing(buf, &entry->top[i],
				offset + 18, offset + 10, size, op);
	}
	sja1105_packing(buf, &entry->speed,      98, 97, size, op);
	sja1105_packing(buf, &entry->maxage,     64, 57, size, op);
	sja1105_packing(buf, &entry->vlanprio,   56, 54, size, op);
	sja1105_packing(buf, &entry->vlanid,     53, 42, size, op);
	sja1105_packing(buf, &entry->dyn_learn,  33, 33, size, op);
	sja1105_packing(buf, &entry->egress,     32, 32, size, op);
	sja1105_packing(buf, &entry->ingress,    31, 31, size, op);
	return size;
}

static size_t sja1105_vlan_lookup_entry_packing(void *buf, void *entry_ptr,
						enum packing_op op)
{
	const size_t size = SJA1105_SIZE_VLAN_LOOKUP_ENTRY;
	struct sja1105_vlan_lookup_entry *entry = entry_ptr;

	sja1105_packing(buf, &entry->vmemb_port, 53, 49, size, op);
	sja1105_packing(buf, &entry->vlan_bc,    48, 44, size, op);
	sja1105_packing(buf, &entry->tag_port,   43, 39, size, op);
	sja1105_packing(buf, &entry->vlanid,     38, 27, size, op);
	return size;
}

static size_t sja1105_xmii_params_entry_packing(void *buf, void *entry_ptr,
						enum packing_op op)
{
	const size_t size = SJA1105_SIZE_XMII_PARAMS_ENTRY;
	struct sja1105_xmii_params_entry *entry = entry_ptr;
	int offset, i;

	for (i = 0, offset = 17; i < 5; i++, offset += 3) {
		sja1105_packing(buf, &entry->xmii_mode[i],
				offset + 1, offset + 0, size, op);
		sja1105_packing(buf, &entry->phy_mac[i],
				offset + 2, offset + 2, size, op);
	}
	return size;
}

static size_t sja1105_table_header_packing(void *buf, void *entry_ptr,
					   enum packing_op op)
{
	const size_t size = SJA1105_SIZE_TABLE_HEADER;
	struct sja1105_table_header *entry = entry_ptr;

	sja1105_packing(buf, &entry->block_id, 31, 24, size, op);
	sja1105_packing(buf, &entry->len,      55, 32, size, op);
	sja1105_packing(buf, &entry->crc,      95, 64, size, op);
	return size;
}

static void
sja1105_table_header_pack_with_crc(void *buf, struct sja1105_table_header *hdr)
{
	/* First pack the table as-is, then calculate the CRC, and
	 * finally put the proper CRC into the packed buffer
	 */
	memset(buf, 0, SJA1105_SIZE_TABLE_HEADER);
	sja1105_table_header_packing(buf, hdr, PACK);
	hdr->crc = sja1105_crc32(buf, SJA1105_SIZE_TABLE_HEADER - 4);
	sja1105_packing(buf + SJA1105_SIZE_TABLE_HEADER - 4, &hdr->crc,
			31, 0, 4, PACK);
}

static void sja1105_table_write_crc(u8 *table_start, u8 *crc_ptr)
{
	u64 computed_crc;
	int len_bytes;

	len_bytes = (uintptr_t)(crc_ptr - table_start);
	computed_crc = sja1105_crc32(table_start, len_bytes);
	sja1105_packing(crc_ptr, &computed_crc, 31, 0, 4, PACK);
}

/* The block IDs that the switches support are unfortunately sparse, so keep a
 * mapping table to "block indices" and translate back and forth.
 */
static u64 blk_id_map[BLK_IDX_MAX] = {
	[BLK_IDX_L2_POLICING] = BLKID_L2_POLICING,
	[BLK_IDX_VLAN_LOOKUP] = BLKID_VLAN_LOOKUP,
	[BLK_IDX_L2_FORWARDING] = BLKID_L2_FORWARDING,
	[BLK_IDX_MAC_CONFIG] = BLKID_MAC_CONFIG,
	[BLK_IDX_L2_LOOKUP_PARAMS] = BLKID_L2_LOOKUP_PARAMS,
	[BLK_IDX_L2_FORWARDING_PARAMS] = BLKID_L2_FORWARDING_PARAMS,
	[BLK_IDX_GENERAL_PARAMS] = BLKID_GENERAL_PARAMS,
	[BLK_IDX_XMII_PARAMS] = BLKID_XMII_PARAMS,
};

static void
sja1105_static_config_pack(void *buf, struct sja1105_static_config *config)
{
	struct sja1105_table_header header = {0};
	enum sja1105_blk_idx i;
	u8 *p = buf;
	int j;

	sja1105_packing(p, &config->device_id, 31, 0, 4, PACK);
	p += SJA1105_SIZE_DEVICE_ID;

	for (i = 0; i < BLK_IDX_MAX; i++) {
		const struct sja1105_table *table;
		u8 *table_start;

		table = &config->tables[i];
		if (!table->entry_count)
			continue;

		header.block_id = blk_id_map[i];
		header.len = table->entry_count *
			     table->ops->packed_entry_size / 4;
		sja1105_table_header_pack_with_crc(p, &header);
		p += SJA1105_SIZE_TABLE_HEADER;
		table_start = p;
		for (j = 0; j < table->entry_count; j++) {
			u8 *entry_ptr = table->entries;

			entry_ptr += j * table->ops->unpacked_entry_size;
			memset(p, 0, table->ops->packed_entry_size);
			table->ops->packing(p, entry_ptr, PACK);
			p += table->ops->packed_entry_size;
		}
		sja1105_table_write_crc(table_start, p);
		p += 4;
	}
	/* Final header:
	 * Block ID does not matter
	 * Length of 0 marks that header is final
	 * CRC will be replaced on-the-fly
	 */
	header.block_id = 0;
	header.len = 0;
	header.crc = 0xDEADBEEF;
	memset(p, 0, SJA1105_SIZE_TABLE_HEADER);
	sja1105_table_header_packing(p, &header, PACK);
}

static size_t
sja1105_static_config_get_length(const struct sja1105_static_config *config)
{
	unsigned int header_count;
	enum sja1105_blk_idx i;
	unsigned int sum;

	/* Ending header */
	header_count = 1;
	sum = SJA1105_SIZE_DEVICE_ID;

	/* Tables (headers and entries) */
	for (i = 0; i < BLK_IDX_MAX; i++) {
		const struct sja1105_table *table;

		table = &config->tables[i];
		if (table->entry_count)
			header_count++;

		sum += table->ops->packed_entry_size * table->entry_count;
	}
	/* Headers have an additional CRC at the end */
	sum += header_count * (SJA1105_SIZE_TABLE_HEADER + 4);
	/* Last header does not have an extra CRC because there is no data */
	sum -= 4;

	return sum;
}

/* Compatibility matrices */
static struct sja1105_table_ops sja1105et_table_ops[BLK_IDX_MAX] = {
	[BLK_IDX_L2_POLICING] = {
		.packing = sja1105_l2_policing_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_l2_policing_entry),
		.packed_entry_size = SJA1105_SIZE_L2_POLICING_ENTRY,
		.max_entry_count = SJA1105_MAX_L2_POLICING_COUNT,
	},
	[BLK_IDX_VLAN_LOOKUP] = {
		.packing = sja1105_vlan_lookup_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_vlan_lookup_entry),
		.packed_entry_size = SJA1105_SIZE_VLAN_LOOKUP_ENTRY,
		.max_entry_count = SJA1105_MAX_VLAN_LOOKUP_COUNT,
	},
	[BLK_IDX_L2_FORWARDING] = {
		.packing = sja1105_l2_forwarding_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_l2_forwarding_entry),
		.packed_entry_size = SJA1105_SIZE_L2_FORWARDING_ENTRY,
		.max_entry_count = SJA1105_MAX_L2_FORWARDING_COUNT,
	},
	[BLK_IDX_MAC_CONFIG] = {
		.packing = sja1105et_mac_config_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_mac_config_entry),
		.packed_entry_size = SJA1105ET_SIZE_MAC_CONFIG_ENTRY,
		.max_entry_count = SJA1105_MAX_MAC_CONFIG_COUNT,
	},
	[BLK_IDX_L2_LOOKUP_PARAMS] = {
		.packing = sja1105et_l2_lookup_params_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_l2_lookup_params_entry),
		.packed_entry_size = SJA1105ET_SIZE_L2_LOOKUP_PARAMS_ENTRY,
		.max_entry_count = SJA1105_MAX_L2_LOOKUP_PARAMS_COUNT,
	},
	[BLK_IDX_L2_FORWARDING_PARAMS] = {
		.packing = sja1105_l2_forwarding_params_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_l2_forwarding_params_entry),
		.packed_entry_size = SJA1105_SIZE_L2_FORWARDING_PARAMS_ENTRY,
		.max_entry_count = SJA1105_MAX_L2_FORWARDING_PARAMS_COUNT,
	},
	[BLK_IDX_GENERAL_PARAMS] = {
		.packing = sja1105et_general_params_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_general_params_entry),
		.packed_entry_size = SJA1105ET_SIZE_GENERAL_PARAMS_ENTRY,
		.max_entry_count = SJA1105_MAX_GENERAL_PARAMS_COUNT,
	},
	[BLK_IDX_XMII_PARAMS] = {
		.packing = sja1105_xmii_params_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_xmii_params_entry),
		.packed_entry_size = SJA1105_SIZE_XMII_PARAMS_ENTRY,
		.max_entry_count = SJA1105_MAX_XMII_PARAMS_COUNT,
	},
};

static struct sja1105_table_ops sja1105pqrs_table_ops[BLK_IDX_MAX] = {
	[BLK_IDX_L2_POLICING] = {
		.packing = sja1105_l2_policing_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_l2_policing_entry),
		.packed_entry_size = SJA1105_SIZE_L2_POLICING_ENTRY,
		.max_entry_count = SJA1105_MAX_L2_POLICING_COUNT,
	},
	[BLK_IDX_VLAN_LOOKUP] = {
		.packing = sja1105_vlan_lookup_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_vlan_lookup_entry),
		.packed_entry_size = SJA1105_SIZE_VLAN_LOOKUP_ENTRY,
		.max_entry_count = SJA1105_MAX_VLAN_LOOKUP_COUNT,
	},
	[BLK_IDX_L2_FORWARDING] = {
		.packing = sja1105_l2_forwarding_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_l2_forwarding_entry),
		.packed_entry_size = SJA1105_SIZE_L2_FORWARDING_ENTRY,
		.max_entry_count = SJA1105_MAX_L2_FORWARDING_COUNT,
	},
	[BLK_IDX_MAC_CONFIG] = {
		.packing = sja1105pqrs_mac_config_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_mac_config_entry),
		.packed_entry_size = SJA1105PQRS_SIZE_MAC_CONFIG_ENTRY,
		.max_entry_count = SJA1105_MAX_MAC_CONFIG_COUNT,
	},
	[BLK_IDX_L2_LOOKUP_PARAMS] = {
		.packing = sja1105pqrs_l2_lookup_params_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_l2_lookup_params_entry),
		.packed_entry_size = SJA1105PQRS_SIZE_L2_LOOKUP_PARAMS_ENTRY,
		.max_entry_count = SJA1105_MAX_L2_LOOKUP_PARAMS_COUNT,
	},
	[BLK_IDX_L2_FORWARDING_PARAMS] = {
		.packing = sja1105_l2_forwarding_params_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_l2_forwarding_params_entry),
		.packed_entry_size = SJA1105_SIZE_L2_FORWARDING_PARAMS_ENTRY,
		.max_entry_count = SJA1105_MAX_L2_FORWARDING_PARAMS_COUNT,
	},
	[BLK_IDX_GENERAL_PARAMS] = {
		.packing = sja1105pqrs_general_params_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_general_params_entry),
		.packed_entry_size = SJA1105PQRS_SIZE_GENERAL_PARAMS_ENTRY,
		.max_entry_count = SJA1105_MAX_GENERAL_PARAMS_COUNT,
	},
	[BLK_IDX_XMII_PARAMS] = {
		.packing = sja1105_xmii_params_entry_packing,
		.unpacked_entry_size = sizeof(struct sja1105_xmii_params_entry),
		.packed_entry_size = SJA1105_SIZE_XMII_PARAMS_ENTRY,
		.max_entry_count = SJA1105_MAX_XMII_PARAMS_COUNT,
	},
};

static int
sja1105_static_config_init(struct sja1105_static_config *config,
			   const struct sja1105_table_ops *static_ops,
			   u64 device_id)
{
	enum sja1105_blk_idx i;

	*config = (struct sja1105_static_config) {0};

	/* Transfer static_ops array from priv into per-table ops
	 * for handier access
	 */
	for (i = 0; i < BLK_IDX_MAX; i++)
		config->tables[i].ops = &static_ops[i];

	config->device_id = device_id;
	return 0;
}

static void sja1105_static_config_free(struct sja1105_static_config *config)
{
	enum sja1105_blk_idx i;

	for (i = 0; i < BLK_IDX_MAX; i++) {
		if (config->tables[i].entry_count) {
			free(config->tables[i].entries);
			config->tables[i].entry_count = 0;
		}
	}
}

static void sja1105_cgu_idiv_packing(void *buf, struct sja1105_cgu_idiv *idiv,
				     enum packing_op op)
{
	const int size = 4;

	sja1105_packing(buf, &idiv->clksrc,    28, 24, size, op);
	sja1105_packing(buf, &idiv->autoblock, 11, 11, size, op);
	sja1105_packing(buf, &idiv->idiv,       5,  2, size, op);
	sja1105_packing(buf, &idiv->pd,         0,  0, size, op);
}

static int sja1105_cgu_idiv_config(struct sja1105_private *priv, int port,
				   bool enabled, int factor)
{
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_cgu_idiv idiv;
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};

	if (enabled && factor != 1 && factor != 10)
		return -ERANGE;

	/* Payload for packed_buf */
	idiv.clksrc    = 0x0A;            /* 25MHz */
	idiv.autoblock = 1;               /* Block clk automatically */
	idiv.idiv      = factor - 1;      /* Divide by 1 or 10 */
	idiv.pd        = enabled ? 0 : 1; /* Power down? */
	sja1105_cgu_idiv_packing(packed_buf, &idiv, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->cgu_idiv[port],
				packed_buf, SJA1105_SIZE_CGU_CMD);
}

static void
sja1105_cgu_mii_control_packing(void *buf, struct sja1105_cgu_mii_ctrl *cmd,
				enum packing_op op)
{
	const int size = 4;

	sja1105_packing(buf, &cmd->clksrc,    28, 24, size, op);
	sja1105_packing(buf, &cmd->autoblock, 11, 11, size, op);
	sja1105_packing(buf, &cmd->pd,         0,  0, size, op);
}

static int sja1105_cgu_mii_tx_clk_config(struct sja1105_private *priv,
					 int port, sja1105_mii_role_t role)
{
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_cgu_mii_ctrl mii_tx_clk;
	const int mac_clk_sources[] = {
		CLKSRC_MII0_TX_CLK,
		CLKSRC_MII1_TX_CLK,
		CLKSRC_MII2_TX_CLK,
		CLKSRC_MII3_TX_CLK,
		CLKSRC_MII4_TX_CLK,
	};
	const int phy_clk_sources[] = {
		CLKSRC_IDIV0,
		CLKSRC_IDIV1,
		CLKSRC_IDIV2,
		CLKSRC_IDIV3,
		CLKSRC_IDIV4,
	};
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};
	int clksrc;

	if (role == XMII_MAC)
		clksrc = mac_clk_sources[port];
	else
		clksrc = phy_clk_sources[port];

	/* Payload for packed_buf */
	mii_tx_clk.clksrc    = clksrc;
	mii_tx_clk.autoblock = 1;  /* Autoblock clk while changing clksrc */
	mii_tx_clk.pd        = 0;  /* Power Down off => enabled */
	sja1105_cgu_mii_control_packing(packed_buf, &mii_tx_clk, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->mii_tx_clk[port],
				packed_buf, SJA1105_SIZE_CGU_CMD);
}

static int
sja1105_cgu_mii_rx_clk_config(struct sja1105_private *priv, int port)
{
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_cgu_mii_ctrl mii_rx_clk;
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};
	const int clk_sources[] = {
		CLKSRC_MII0_RX_CLK,
		CLKSRC_MII1_RX_CLK,
		CLKSRC_MII2_RX_CLK,
		CLKSRC_MII3_RX_CLK,
		CLKSRC_MII4_RX_CLK,
	};

	/* Payload for packed_buf */
	mii_rx_clk.clksrc    = clk_sources[port];
	mii_rx_clk.autoblock = 1;  /* Autoblock clk while changing clksrc */
	mii_rx_clk.pd        = 0;  /* Power Down off => enabled */
	sja1105_cgu_mii_control_packing(packed_buf, &mii_rx_clk, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->mii_rx_clk[port],
				packed_buf, SJA1105_SIZE_CGU_CMD);
}

static int
sja1105_cgu_mii_ext_tx_clk_config(struct sja1105_private *priv, int port)
{
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_cgu_mii_ctrl mii_ext_tx_clk;
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};
	const int clk_sources[] = {
		CLKSRC_IDIV0,
		CLKSRC_IDIV1,
		CLKSRC_IDIV2,
		CLKSRC_IDIV3,
		CLKSRC_IDIV4,
	};

	/* Payload for packed_buf */
	mii_ext_tx_clk.clksrc    = clk_sources[port];
	mii_ext_tx_clk.autoblock = 1; /* Autoblock clk while changing clksrc */
	mii_ext_tx_clk.pd        = 0; /* Power Down off => enabled */
	sja1105_cgu_mii_control_packing(packed_buf, &mii_ext_tx_clk, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->mii_ext_tx_clk[port],
				packed_buf, SJA1105_SIZE_CGU_CMD);
}

static int
sja1105_cgu_mii_ext_rx_clk_config(struct sja1105_private *priv, int port)
{
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_cgu_mii_ctrl mii_ext_rx_clk;
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};
	const int clk_sources[] = {
		CLKSRC_IDIV0,
		CLKSRC_IDIV1,
		CLKSRC_IDIV2,
		CLKSRC_IDIV3,
		CLKSRC_IDIV4,
	};

	/* Payload for packed_buf */
	mii_ext_rx_clk.clksrc    = clk_sources[port];
	mii_ext_rx_clk.autoblock = 1; /* Autoblock clk while changing clksrc */
	mii_ext_rx_clk.pd        = 0; /* Power Down off => enabled */
	sja1105_cgu_mii_control_packing(packed_buf, &mii_ext_rx_clk, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->mii_ext_rx_clk[port],
				packed_buf, SJA1105_SIZE_CGU_CMD);
}

static int sja1105_mii_clocking_setup(struct sja1105_private *priv, int port,
				      sja1105_mii_role_t role)
{
	int rc;

	rc = sja1105_cgu_idiv_config(priv, port, (role == XMII_PHY), 1);
	if (rc < 0)
		return rc;

	rc = sja1105_cgu_mii_tx_clk_config(priv, port, role);
	if (rc < 0)
		return rc;

	rc = sja1105_cgu_mii_rx_clk_config(priv, port);
	if (rc < 0)
		return rc;

	if (role == XMII_PHY) {
		rc = sja1105_cgu_mii_ext_tx_clk_config(priv, port);
		if (rc < 0)
			return rc;

		rc = sja1105_cgu_mii_ext_rx_clk_config(priv, port);
		if (rc < 0)
			return rc;
	}
	return 0;
}

static void
sja1105_cgu_pll_control_packing(void *buf, struct sja1105_cgu_pll_ctrl *cmd,
				enum packing_op op)
{
	const int size = 4;

	sja1105_packing(buf, &cmd->pllclksrc, 28, 24, size, op);
	sja1105_packing(buf, &cmd->msel,      23, 16, size, op);
	sja1105_packing(buf, &cmd->autoblock, 11, 11, size, op);
	sja1105_packing(buf, &cmd->psel,       9,  8, size, op);
	sja1105_packing(buf, &cmd->direct,     7,  7, size, op);
	sja1105_packing(buf, &cmd->fbsel,      6,  6, size, op);
	sja1105_packing(buf, &cmd->bypass,     1,  1, size, op);
	sja1105_packing(buf, &cmd->pd,         0,  0, size, op);
}

static int sja1105_cgu_rgmii_tx_clk_config(struct sja1105_private *priv,
					   int port, sja1105_speed_t speed)
{
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_cgu_mii_ctrl txc;
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};
	int clksrc;

	if (speed == SJA1105_SPEED_1000MBPS) {
		clksrc = CLKSRC_PLL0;
	} else {
		int clk_sources[] = {CLKSRC_IDIV0, CLKSRC_IDIV1, CLKSRC_IDIV2,
				     CLKSRC_IDIV3, CLKSRC_IDIV4};
		clksrc = clk_sources[port];
	}

	/* RGMII: 125MHz for 1000, 25MHz for 100, 2.5MHz for 10 */
	txc.clksrc = clksrc;
	/* Autoblock clk while changing clksrc */
	txc.autoblock = 1;
	/* Power Down off => enabled */
	txc.pd = 0;
	sja1105_cgu_mii_control_packing(packed_buf, &txc, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->rgmii_tx_clk[port],
				packed_buf, SJA1105_SIZE_CGU_CMD);
}

/* AGU */
static void
sja1105_cfg_pad_mii_tx_packing(void *buf, struct sja1105_cfg_pad_mii_tx *cmd,
			       enum packing_op op)
{
	const int size = 4;

	sja1105_packing(buf, &cmd->d32_os,   28, 27, size, op);
	sja1105_packing(buf, &cmd->d32_ipud, 25, 24, size, op);
	sja1105_packing(buf, &cmd->d10_os,   20, 19, size, op);
	sja1105_packing(buf, &cmd->d10_ipud, 17, 16, size, op);
	sja1105_packing(buf, &cmd->ctrl_os,  12, 11, size, op);
	sja1105_packing(buf, &cmd->ctrl_ipud, 9,  8, size, op);
	sja1105_packing(buf, &cmd->clk_os,    4,  3, size, op);
	sja1105_packing(buf, &cmd->clk_ih,    2,  2, size, op);
	sja1105_packing(buf, &cmd->clk_ipud,  1,  0, size, op);
}

static int sja1105_rgmii_cfg_pad_tx_config(struct sja1105_private *priv,
					   int port)
{
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_cfg_pad_mii_tx pad_mii_tx;
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};

	/* Payload */
	pad_mii_tx.d32_os    = 3; /* TXD[3:2] output stage: */
				  /*          high noise/high speed */
	pad_mii_tx.d10_os    = 3; /* TXD[1:0] output stage: */
				  /*          high noise/high speed */
	pad_mii_tx.d32_ipud  = 2; /* TXD[3:2] input stage: */
				  /*          plain input (default) */
	pad_mii_tx.d10_ipud  = 2; /* TXD[1:0] input stage: */
				  /*          plain input (default) */
	pad_mii_tx.ctrl_os   = 3; /* TX_CTL / TX_ER output stage */
	pad_mii_tx.ctrl_ipud = 2; /* TX_CTL / TX_ER input stage (default) */
	pad_mii_tx.clk_os    = 3; /* TX_CLK output stage */
	pad_mii_tx.clk_ih    = 0; /* TX_CLK input hysteresis (default) */
	pad_mii_tx.clk_ipud  = 2; /* TX_CLK input stage (default) */
	sja1105_cfg_pad_mii_tx_packing(packed_buf, &pad_mii_tx, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->pad_mii_tx[port],
				packed_buf, SJA1105_SIZE_CGU_CMD);
}

static void
sja1105_cfg_pad_mii_id_packing(void *buf, struct sja1105_cfg_pad_mii_id *cmd,
			       enum packing_op op)
{
	const int size = SJA1105_SIZE_CGU_CMD;

	sja1105_packing(buf, &cmd->rxc_stable_ovr, 15, 15, size, op);
	sja1105_packing(buf, &cmd->rxc_delay,      14, 10, size, op);
	sja1105_packing(buf, &cmd->rxc_bypass,      9,  9, size, op);
	sja1105_packing(buf, &cmd->rxc_pd,          8,  8, size, op);
	sja1105_packing(buf, &cmd->txc_stable_ovr,  7,  7, size, op);
	sja1105_packing(buf, &cmd->txc_delay,       6,  2, size, op);
	sja1105_packing(buf, &cmd->txc_bypass,      1,  1, size, op);
	sja1105_packing(buf, &cmd->txc_pd,          0,  0, size, op);
}

/* Valid range in degrees is an integer between 73.8 and 101.7 */
static u64 sja1105_rgmii_delay(u64 phase)
{
	/* UM11040.pdf: The delay in degree phase is 73.8 + delay_tune * 0.9.
	 * To avoid floating point operations we'll multiply by 10
	 * and get 1 decimal point precision.
	 */
	phase *= 10;
	return (phase - 738) / 9;
}

static int sja1105pqrs_setup_rgmii_delay(struct sja1105_private *priv, int port)
{
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_cfg_pad_mii_id pad_mii_id = {0};
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};
	int rc;

	if (priv->rgmii_rx_delay[port])
		pad_mii_id.rxc_delay = sja1105_rgmii_delay(90);
	if (priv->rgmii_tx_delay[port])
		pad_mii_id.txc_delay = sja1105_rgmii_delay(90);

	/* Stage 1: Turn the RGMII delay lines off. */
	pad_mii_id.rxc_bypass = 1;
	pad_mii_id.rxc_pd = 1;
	pad_mii_id.txc_bypass = 1;
	pad_mii_id.txc_pd = 1;
	sja1105_cfg_pad_mii_id_packing(packed_buf, &pad_mii_id, PACK);

	rc = sja1105_xfer_buf(priv, SPI_WRITE, regs->pad_mii_id[port],
			      packed_buf, SJA1105_SIZE_CGU_CMD);
	if (rc < 0)
		return rc;

	/* Stage 2: Turn the RGMII delay lines on. */
	if (priv->rgmii_rx_delay[port]) {
		pad_mii_id.rxc_bypass = 0;
		pad_mii_id.rxc_pd = 0;
	}
	if (priv->rgmii_tx_delay[port]) {
		pad_mii_id.txc_bypass = 0;
		pad_mii_id.txc_pd = 0;
	}
	sja1105_cfg_pad_mii_id_packing(packed_buf, &pad_mii_id, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->pad_mii_id[port],
				packed_buf, SJA1105_SIZE_CGU_CMD);
}

static int sja1105_rgmii_clocking_setup(struct sja1105_private *priv, int port,
					sja1105_mii_role_t role)
{
	struct sja1105_mac_config_entry *mac;
	sja1105_speed_t speed;
	int rc;

	mac = priv->static_config.tables[BLK_IDX_MAC_CONFIG].entries;
	speed = mac[port].speed;

	switch (speed) {
	case SJA1105_SPEED_1000MBPS:
		/* 1000Mbps, IDIV disabled (125 MHz) */
		rc = sja1105_cgu_idiv_config(priv, port, false, 1);
		break;
	case SJA1105_SPEED_100MBPS:
		/* 100Mbps, IDIV enabled, divide by 1 (25 MHz) */
		rc = sja1105_cgu_idiv_config(priv, port, true, 1);
		break;
	case SJA1105_SPEED_10MBPS:
		/* 10Mbps, IDIV enabled, divide by 10 (2.5 MHz) */
		rc = sja1105_cgu_idiv_config(priv, port, true, 10);
		break;
	default:
		rc = -EINVAL;
	}

	if (rc < 0) {
		dev_err(dev, "Failed to configure idiv\n");
		return rc;
	}
	rc = sja1105_cgu_rgmii_tx_clk_config(priv, port, speed);
	if (rc < 0) {
		dev_err(dev, "Failed to configure RGMII Tx clock\n");
		return rc;
	}
	rc = sja1105_rgmii_cfg_pad_tx_config(priv, port);
	if (rc < 0) {
		dev_err(dev, "Failed to configure Tx pad registers\n");
		return rc;
	}
	if (!priv->info->setup_rgmii_delay)
		return 0;
	/* The role has no hardware effect for RGMII. However we use it as
	 * a proxy for this interface being a MAC-to-MAC connection, with
	 * the RGMII internal delays needing to be applied by us.
	 */
	if (role == XMII_MAC)
		return 0;

	return priv->info->setup_rgmii_delay(priv, port);
}

static int sja1105_cgu_rmii_ref_clk_config(struct sja1105_private *priv,
					   int port)
{
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_cgu_mii_ctrl ref_clk;
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};
	const int clk_sources[] = {
		CLKSRC_MII0_TX_CLK,
		CLKSRC_MII1_TX_CLK,
		CLKSRC_MII2_TX_CLK,
		CLKSRC_MII3_TX_CLK,
		CLKSRC_MII4_TX_CLK,
	};

	/* Payload for packed_buf */
	ref_clk.clksrc    = clk_sources[port];
	ref_clk.autoblock = 1;      /* Autoblock clk while changing clksrc */
	ref_clk.pd        = 0;      /* Power Down off => enabled */
	sja1105_cgu_mii_control_packing(packed_buf, &ref_clk, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->rmii_ref_clk[port],
				packed_buf, SJA1105_SIZE_CGU_CMD);
}

static int
sja1105_cgu_rmii_ext_tx_clk_config(struct sja1105_private *priv, int port)
{
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_cgu_mii_ctrl ext_tx_clk;
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};

	/* Payload for packed_buf */
	ext_tx_clk.clksrc    = CLKSRC_PLL1;
	ext_tx_clk.autoblock = 1;   /* Autoblock clk while changing clksrc */
	ext_tx_clk.pd        = 0;   /* Power Down off => enabled */
	sja1105_cgu_mii_control_packing(packed_buf, &ext_tx_clk, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->rmii_ext_tx_clk[port],
				packed_buf, SJA1105_SIZE_CGU_CMD);
}

static int sja1105_cgu_rmii_pll_config(struct sja1105_private *priv)
{
	const struct sja1105_regs *regs = priv->info->regs;
	u8 packed_buf[SJA1105_SIZE_CGU_CMD] = {0};
	struct sja1105_cgu_pll_ctrl pll = {0};
	int rc;

	/* Step 1: PLL1 setup for 50Mhz */
	pll.pllclksrc = 0xA;
	pll.msel      = 0x1;
	pll.autoblock = 0x1;
	pll.psel      = 0x1;
	pll.direct    = 0x0;
	pll.fbsel     = 0x1;
	pll.bypass    = 0x0;
	pll.pd        = 0x1;

	sja1105_cgu_pll_control_packing(packed_buf, &pll, PACK);
	rc = sja1105_xfer_buf(priv, SPI_WRITE, regs->rmii_pll1, packed_buf,
			      SJA1105_SIZE_CGU_CMD);
	if (rc < 0)
		return rc;

	/* Step 2: Enable PLL1 */
	pll.pd = 0x0;

	sja1105_cgu_pll_control_packing(packed_buf, &pll, PACK);
	rc = sja1105_xfer_buf(priv, SPI_WRITE, regs->rmii_pll1, packed_buf,
			      SJA1105_SIZE_CGU_CMD);
	return rc;
}

static int sja1105_rmii_clocking_setup(struct sja1105_private *priv, int port,
				       sja1105_mii_role_t role)
{
	int rc;

	/* AH1601.pdf chapter 2.5.1. Sources */
	if (role == XMII_MAC) {
		/* Configure and enable PLL1 for 50Mhz output */
		rc = sja1105_cgu_rmii_pll_config(priv);
		if (rc < 0)
			return rc;
	}
	/* Disable IDIV for this port */
	rc = sja1105_cgu_idiv_config(priv, port, false, 1);
	if (rc < 0)
		return rc;
	/* Source to sink mappings */
	rc = sja1105_cgu_rmii_ref_clk_config(priv, port);
	if (rc < 0)
		return rc;
	if (role == XMII_MAC) {
		rc = sja1105_cgu_rmii_ext_tx_clk_config(priv, port);
		if (rc < 0)
			return rc;
	}
	return 0;
}

static int sja1105_sgmii_write(struct sja1105_private *priv, int pcs_reg,
			       u16 pcs_val)
{
	const struct sja1105_regs *regs = priv->info->regs;
	u8 packed_buf[4] = {0};
	u64 val = pcs_val;

	sja1105_packing(packed_buf, &val, 31, 0, 4, PACK);

	return sja1105_xfer_buf(priv, SPI_WRITE, regs->sgmii + pcs_reg,
				packed_buf, 4);
}

static int sja1105_sgmii_setup(struct sja1105_private *priv, int port)
{
	int bmcr = BMCR_FULLDPLX | priv->sgmii_cfg.pcs_speed;

	if (priv->sgmii_cfg.inband_an)
		bmcr |= BMCR_ANENABLE | BMCR_ANRESTART;

	sja1105_sgmii_write(priv, MII_BMCR, bmcr);
	/* DIGITAL_CONTROL_1: Enable vendor-specific MMD1, allow the PHY to
	 * stop the clock during LPI mode, make the MAC reconfigure
	 * autonomously after PCS autoneg is done, flush the internal FIFOs.
	 */
	sja1105_sgmii_write(priv, 0x8000, SJA1105_DC1_EN_VSMMD1 |
					  SJA1105_DC1_CLOCK_STOP_EN |
					  SJA1105_DC1_MAC_AUTO_SW |
					  SJA1105_DC1_INIT);
	/* DIGITAL_CONTROL_2: No polarity inversion for TX and RX lanes */
	sja1105_sgmii_write(priv, 0x80e1, SJA1105_DC2_TX_POL_INV_DISABLE);
	/* AUTONEG_CONTROL: Use SGMII autoneg */
	sja1105_sgmii_write(priv, 0x8001, SJA1105_AC_AUTONEG_MODE_SGMII);

	return 0;
}

static int sja1105_clocking_setup_port(struct sja1105_private *priv, int port)
{
	struct sja1105_xmii_params_entry *mii;
	sja1105_phy_interface_t phy_mode;
	sja1105_mii_role_t role;
	int rc;

	mii = priv->static_config.tables[BLK_IDX_XMII_PARAMS].entries;

	/* RGMII etc */
	phy_mode = mii->xmii_mode[port];
	/* MAC or PHY, for applicable types (not RGMII) */
	role = mii->phy_mac[port];

	switch (phy_mode) {
	case XMII_MODE_MII:
		rc = sja1105_mii_clocking_setup(priv, port, role);
		break;
	case XMII_MODE_RMII:
		rc = sja1105_rmii_clocking_setup(priv, port, role);
		break;
	case XMII_MODE_RGMII:
		rc = sja1105_rgmii_clocking_setup(priv, port, role);
		break;
	case XMII_MODE_SGMII:
		rc = sja1105_sgmii_setup(priv, port);
		break;
	default:
		return -EINVAL;
	}
	return rc;
}

static int sja1105_clocking_setup(struct sja1105_private *priv)
{
	int port, rc;

	for (port = 0; port < SJA1105_NUM_PORTS; port++) {
		rc = sja1105_clocking_setup_port(priv, port);
		if (rc < 0)
			return rc;
	}
	return 0;
}

static struct sja1105_regs sja1105et_regs = {
	.device_id = 0x0,
	.prod_id = 0x100BC3,
	.status = 0x1,
	.port_control = 0x11,
	.config = 0x020000,
	.rgu = 0x100440,
	/* UM10944.pdf, Table 86, ACU Register overview */
	.pad_mii_tx = {0x100800, 0x100802, 0x100804, 0x100806, 0x100808},
	.rmii_pll1 = 0x10000A,
	.cgu_idiv = {0x10000B, 0x10000C, 0x10000D, 0x10000E, 0x10000F},
	/* UM10944.pdf, Table 78, CGU Register overview */
	.mii_tx_clk = {0x100013, 0x10001A, 0x100021, 0x100028, 0x10002F},
	.mii_rx_clk = {0x100014, 0x10001B, 0x100022, 0x100029, 0x100030},
	.mii_ext_tx_clk = {0x100018, 0x10001F, 0x100026, 0x10002D, 0x100034},
	.mii_ext_rx_clk = {0x100019, 0x100020, 0x100027, 0x10002E, 0x100035},
	.rgmii_tx_clk = {0x100016, 0x10001D, 0x100024, 0x10002B, 0x100032},
	.rmii_ref_clk = {0x100015, 0x10001C, 0x100023, 0x10002A, 0x100031},
	.rmii_ext_tx_clk = {0x100018, 0x10001F, 0x100026, 0x10002D, 0x100034},
};

static struct sja1105_regs sja1105pqrs_regs = {
	.device_id = 0x0,
	.prod_id = 0x100BC3,
	.status = 0x1,
	.port_control = 0x12,
	.config = 0x020000,
	.rgu = 0x100440,
	/* UM10944.pdf, Table 86, ACU Register overview */
	.pad_mii_tx = {0x100800, 0x100802, 0x100804, 0x100806, 0x100808},
	.pad_mii_id = {0x100810, 0x100811, 0x100812, 0x100813, 0x100814},
	.rmii_pll1 = 0x10000A,
	.cgu_idiv = {0x10000B, 0x10000C, 0x10000D, 0x10000E, 0x10000F},
	/* UM11040.pdf, Table 114 */
	.mii_tx_clk = {0x100013, 0x100019, 0x10001F, 0x100025, 0x10002B},
	.mii_rx_clk = {0x100014, 0x10001A, 0x100020, 0x100026, 0x10002C},
	.mii_ext_tx_clk = {0x100017, 0x10001D, 0x100023, 0x100029, 0x10002F},
	.mii_ext_rx_clk = {0x100018, 0x10001E, 0x100024, 0x10002A, 0x100030},
	.rgmii_tx_clk = {0x100016, 0x10001C, 0x100022, 0x100028, 0x10002E},
	.rmii_ref_clk = {0x100015, 0x10001B, 0x100021, 0x100027, 0x10002D},
	.rmii_ext_tx_clk = {0x100017, 0x10001D, 0x100023, 0x100029, 0x10002F},
	.sgmii = 0x1F0000,
};

enum sja1105_switch_id {
	SJA1105E = 0,
	SJA1105T,
	SJA1105P,
	SJA1105Q,
	SJA1105R,
	SJA1105S,
};

struct sja1105_info sja1105_info[] = {
	[SJA1105E] = {
		.device_id		= SJA1105E_DEVICE_ID,
		.part_no		= SJA1105ET_PART_NO,
		.static_ops		= sja1105et_table_ops,
		.reset_cmd		= sja1105et_reset_cmd,
		.regs			= &sja1105et_regs,
		.name			= "SJA1105E",
	},
	[SJA1105T] = {
		.device_id		= SJA1105T_DEVICE_ID,
		.part_no		= SJA1105ET_PART_NO,
		.static_ops		= sja1105et_table_ops,
		.reset_cmd		= sja1105et_reset_cmd,
		.regs			= &sja1105et_regs,
		.name			= "SJA1105T",
	},
	[SJA1105P] = {
		.device_id		= SJA1105PR_DEVICE_ID,
		.part_no		= SJA1105P_PART_NO,
		.static_ops		= sja1105pqrs_table_ops,
		.setup_rgmii_delay	= sja1105pqrs_setup_rgmii_delay,
		.reset_cmd		= sja1105pqrs_reset_cmd,
		.regs			= &sja1105pqrs_regs,
		.name			= "SJA1105P",
	},
	[SJA1105Q] = {
		.device_id		= SJA1105QS_DEVICE_ID,
		.part_no		= SJA1105Q_PART_NO,
		.static_ops		= sja1105pqrs_table_ops,
		.setup_rgmii_delay	= sja1105pqrs_setup_rgmii_delay,
		.reset_cmd		= sja1105pqrs_reset_cmd,
		.regs			= &sja1105pqrs_regs,
		.name			= "SJA1105Q",
	},
	[SJA1105R] = {
		.device_id		= SJA1105PR_DEVICE_ID,
		.part_no		= SJA1105R_PART_NO,
		.static_ops		= sja1105pqrs_table_ops,
		.setup_rgmii_delay	= sja1105pqrs_setup_rgmii_delay,
		.reset_cmd		= sja1105pqrs_reset_cmd,
		.regs			= &sja1105pqrs_regs,
		.name			= "SJA1105R",
	},
	[SJA1105S] = {
		.device_id		= SJA1105QS_DEVICE_ID,
		.part_no		= SJA1105S_PART_NO,
		.static_ops		= sja1105pqrs_table_ops,
		.setup_rgmii_delay	= sja1105pqrs_setup_rgmii_delay,
		.reset_cmd		= sja1105pqrs_reset_cmd,
		.regs			= &sja1105pqrs_regs,
		.name			= "SJA1105S",
	},
};

static void
sja1105_port_allow_traffic(struct sja1105_l2_forwarding_entry *l2_fwd,
			   int from, int to)
{
	l2_fwd[from].bc_domain  |= BIT(to);
	l2_fwd[from].reach_port |= BIT(to);
	l2_fwd[from].fl_domain  |= BIT(to);
}

static int sja1105_init_mac_settings(struct sja1105_private *priv)
{
	struct sja1105_mac_config_entry default_mac = {
		/* Enable 1 priority queue on egress. */
		.top  = {0x1FF, 0, 0, 0, 0, 0, 0},
		.base = {0x0, 0, 0, 0, 0, 0, 0, 0},
		.enabled = {1, 0, 0, 0, 0, 0, 0, 0},
		/* Default for all ports including the CPU. FIXME: may not work
		 * for all ports.
		 */
		.speed = SJA1105_SPEED_1000MBPS,
		/* Disable aging for critical TTEthernet traffic */
		.maxage = 0xFF,
		.dyn_learn = true,
		.egress = true,
		.ingress = true,
	};
	struct sja1105_mac_config_entry *mac;
	struct sja1105_table *table;
	int port;

	table = &priv->static_config.tables[BLK_IDX_MAC_CONFIG];

	table->entries = calloc(SJA1105_NUM_PORTS,
				table->ops->unpacked_entry_size);
	if (!table->entries)
		return -ENOMEM;

	table->entry_count = SJA1105_NUM_PORTS;

	mac = table->entries;

	for (port = 0; port < SJA1105_NUM_PORTS; port++) {
		mac[port] = default_mac;
		/* Internal VLAN (pvid) to apply to untagged ingress */
		mac[port].vlanid = priv->pvid[port];
	}

	return 0;
}

static bool sja1105_supports_sgmii(struct sja1105_private *priv, int port)
{
	if (priv->info->part_no != SJA1105R_PART_NO &&
	    priv->info->part_no != SJA1105S_PART_NO)
		return false;

	if (port != SJA1105_SGMII_PORT)
		return false;

	return true;
}

static int sja1105_init_mii_settings(struct sja1105_private *priv)
{
	struct dsa_perdev_platdata *platdata = priv->dev->platdata;
	struct sja1105_xmii_params_entry *mii;
	struct sja1105_table *table;
	int port;

	table = &priv->static_config.tables[BLK_IDX_XMII_PARAMS];

	table->entries = calloc(SJA1105_MAX_XMII_PARAMS_COUNT,
				table->ops->unpacked_entry_size);
	if (!table->entries)
		return -ENOMEM;

	/* Override table based on DT bindings */
	table->entry_count = SJA1105_MAX_XMII_PARAMS_COUNT;

	mii = table->entries;

	for (port = 0; port < SJA1105_NUM_PORTS; port++) {
		struct phy_device *phy = platdata->port[port].phy;

		switch (priv->phy_modes[port]) {
		case PHY_INTERFACE_MODE_MII:
			mii->xmii_mode[port] = XMII_MODE_MII;
			break;
		case PHY_INTERFACE_MODE_RMII:
			mii->xmii_mode[port] = XMII_MODE_RMII;
			break;
		case PHY_INTERFACE_MODE_RGMII:
		case PHY_INTERFACE_MODE_RGMII_ID:
		case PHY_INTERFACE_MODE_RGMII_RXID:
		case PHY_INTERFACE_MODE_RGMII_TXID:
			mii->xmii_mode[port] = XMII_MODE_RGMII;
			break;
		case PHY_INTERFACE_MODE_SGMII:
			if (!sja1105_supports_sgmii(priv, port))
				return -EINVAL;
			mii->xmii_mode[port] = XMII_MODE_SGMII;
			break;
		default:
			return -EINVAL;
		}

		if (priv->phy_modes[port] != PHY_INTERFACE_MODE_SGMII &&
		    (!phy || phy->phy_id == PHY_FIXED_ID))
			mii->phy_mac[port] = XMII_PHY;
		else
			mii->phy_mac[port] = XMII_MAC;
	}
	return 0;
}

static int sja1105_init_l2_lookup_params(struct sja1105_private *priv)
{
	u64 max_fdb_entries = SJA1105_MAX_L2_LOOKUP_COUNT / SJA1105_NUM_PORTS;
	struct sja1105_l2_lookup_params_entry default_l2_lookup_params = {
		/* All entries within a FDB bin are available for learning */
		.dyn_tbsz = SJA1105ET_FDB_BIN_SIZE,
		/* And the P/Q/R/S equivalent setting: */
		.start_dynspc = 0,
		.maxaddrp = {max_fdb_entries, max_fdb_entries, max_fdb_entries,
			     max_fdb_entries, max_fdb_entries, },
		/* 2^8 + 2^5 + 2^3 + 2^2 + 2^1 + 1 in Koopman notation */
		.poly = 0x97,
		/* P/Q/R/S only */
		.use_static = true,
		/* Dynamically learned FDB entries can overwrite other (older)
		 * dynamic FDB entries
		 */
		.owr_dyn = true,
	};
	struct sja1105_table *table;

	table = &priv->static_config.tables[BLK_IDX_L2_LOOKUP_PARAMS];

	table->entries = calloc(SJA1105_MAX_L2_LOOKUP_PARAMS_COUNT,
				table->ops->unpacked_entry_size);
	if (!table->entries)
		return -ENOMEM;

	table->entry_count = SJA1105_MAX_L2_LOOKUP_PARAMS_COUNT;

	/* This table only has a single entry */
	((struct sja1105_l2_lookup_params_entry *)table->entries)[0] =
				default_l2_lookup_params;

	return 0;
}

static void sja1105_setup_tagging(struct sja1105_private *priv, int port)
{
	struct dsa_perdev_platdata *platdata = priv->dev->platdata;
	struct sja1105_vlan_lookup_entry *vlan;
	int cpu = platdata->cpu_port;

	/* The CPU port is implicitly configured by
	 * configuring the front-panel ports
	 */
	if (port == cpu)
		return;

	vlan = priv->static_config.tables[BLK_IDX_VLAN_LOOKUP].entries;

	priv->pvid[port] = DSA_8021Q_DIR_RX | DSA_8021Q_PORT(port);

	vlan[port].vmemb_port	= BIT(port) | BIT(cpu);
	vlan[port].vlan_bc	= BIT(port) | BIT(cpu);
	vlan[port].tag_port	= BIT(cpu);
	vlan[port].vlanid	= priv->pvid[port];
}

static int sja1105_init_vlan(struct sja1105_private *priv)
{
	struct sja1105_table *table;
	int port;

	table = &priv->static_config.tables[BLK_IDX_VLAN_LOOKUP];

	table->entries = calloc(SJA1105_NUM_PORTS,
				table->ops->unpacked_entry_size);
	if (!table->entries)
		return -ENOMEM;

	table->entry_count = SJA1105_NUM_PORTS;

	for (port = 0; port < SJA1105_NUM_PORTS; port++)
		sja1105_setup_tagging(priv, port);

	return 0;
}

static int sja1105_init_l2_forwarding(struct sja1105_private *priv)
{
	struct dsa_perdev_platdata *platdata = priv->dev->platdata;
	struct sja1105_l2_forwarding_entry *l2fwd;
	int cpu = platdata->cpu_port;
	struct sja1105_table *table;
	int i;

	table = &priv->static_config.tables[BLK_IDX_L2_FORWARDING];

	table->entries = calloc(SJA1105_MAX_L2_FORWARDING_COUNT,
				table->ops->unpacked_entry_size);
	if (!table->entries)
		return -ENOMEM;

	table->entry_count = SJA1105_MAX_L2_FORWARDING_COUNT;

	l2fwd = table->entries;

	/* First 5 entries define the forwarding rules */
	for (i = 0; i < SJA1105_NUM_PORTS; i++) {
		if (i == cpu)
			continue;

		sja1105_port_allow_traffic(l2fwd, i, cpu);
		sja1105_port_allow_traffic(l2fwd, cpu, i);
	}
	/* Next 8 entries define VLAN PCP mapping from ingress to egress.
	 * Leave them unpopulated (implicitly 0) but present.
	 */
	return 0;
}

static int sja1105_init_l2_forwarding_params(struct sja1105_private *priv)
{
	struct sja1105_l2_forwarding_params_entry default_l2fwd_params = {
		/* Use a single memory partition for all ingress queues */
		.part_spc = { SJA1105_MAX_FRAME_MEMORY, 0, 0, 0, 0, 0, 0, 0 },
	};
	struct sja1105_table *table;

	table = &priv->static_config.tables[BLK_IDX_L2_FORWARDING_PARAMS];

	table->entries = calloc(SJA1105_MAX_L2_FORWARDING_PARAMS_COUNT,
				table->ops->unpacked_entry_size);
	if (!table->entries)
		return -ENOMEM;

	table->entry_count = SJA1105_MAX_L2_FORWARDING_PARAMS_COUNT;

	/* This table only has a single entry */
	((struct sja1105_l2_forwarding_params_entry *)table->entries)[0] =
				default_l2fwd_params;

	return 0;
}

static int sja1105_init_general_params(struct sja1105_private *priv)
{
	struct sja1105_general_params_entry default_general_params = {
		/* No frame trapping */
		.mac_fltres1 = 0x0,
		.mac_flt1    = 0xffffffffffff,
		.mac_fltres0 = 0x0,
		.mac_flt0    = 0xffffffffffff,
		.host_port = SJA1105_NUM_PORTS,
		/* No mirroring => specify an out-of-range port value */
		.mirr_port = SJA1105_NUM_PORTS,
		/* No link-local trapping => specify an out-of-range port value
		 */
		.casc_port = SJA1105_NUM_PORTS,
		/* Force the switch to see all traffic as untagged. */
		.tpid = ETH_P_SJA1105,
		.tpid2 = ETH_P_SJA1105,
	};
	struct sja1105_table *table;

	table = &priv->static_config.tables[BLK_IDX_GENERAL_PARAMS];

	table->entries = calloc(SJA1105_MAX_GENERAL_PARAMS_COUNT,
				table->ops->unpacked_entry_size);
	if (!table->entries)
		return -ENOMEM;

	table->entry_count = SJA1105_MAX_GENERAL_PARAMS_COUNT;

	/* This table only has a single entry */
	((struct sja1105_general_params_entry *)table->entries)[0] =
				default_general_params;

	return 0;
}

static void sja1105_setup_policer(struct sja1105_l2_policing_entry *policing,
				  int index, int mtu)
{
	policing[index].sharindx = index;
	policing[index].smax = 65535; /* Burst size in bytes */
	policing[index].rate = SJA1105_RATE_MBPS(1000);
	policing[index].maxlen = mtu;
	policing[index].partition = 0;
}

static int sja1105_init_l2_policing(struct sja1105_private *priv)
{
	struct dsa_perdev_platdata *platdata = priv->dev->platdata;
	struct sja1105_l2_policing_entry *policing;
	int cpu = platdata->cpu_port;
	struct sja1105_table *table;
	int i, j, k;

	table = &priv->static_config.tables[BLK_IDX_L2_POLICING];

	table->entries = calloc(SJA1105_MAX_L2_POLICING_COUNT,
				table->ops->unpacked_entry_size);
	if (!table->entries)
		return -ENOMEM;

	table->entry_count = SJA1105_MAX_L2_POLICING_COUNT;

	policing = table->entries;

	/* k sweeps through all unicast policers (0-39).
	 * bcast sweeps through policers 40-44.
	 */
	for (i = 0, k = 0; i < SJA1105_NUM_PORTS; i++) {
		int bcast = (SJA1105_NUM_PORTS * SJA1105_NUM_TC) + i;
		int mtu = VLAN_ETH_FRAME_LEN + ETH_FCS_LEN;

		if (i == cpu)
			mtu += VLAN_HLEN;

		for (j = 0; j < SJA1105_NUM_TC; j++, k++)
			sja1105_setup_policer(policing, k, mtu);

		/* Set up this port's policer for broadcast traffic */
		sja1105_setup_policer(policing, bcast, mtu);
	}
	return 0;
}

struct sja1105_status {
	u64 configs;
	u64 crcchkl;
	u64 ids;
	u64 crcchkg;
};

static void sja1105_status_unpack(void *buf, struct sja1105_status *status)
{
	sja1105_packing(buf, &status->configs,   31, 31, 4, UNPACK);
	sja1105_packing(buf, &status->crcchkl,   30, 30, 4, UNPACK);
	sja1105_packing(buf, &status->ids,       29, 29, 4, UNPACK);
	sja1105_packing(buf, &status->crcchkg,   28, 28, 4, UNPACK);
}

static int sja1105_status_get(struct sja1105_private *priv,
			      struct sja1105_status *status)
{
	const struct sja1105_regs *regs = priv->info->regs;
	u8 packed_buf[4];
	int rc;

	rc = sja1105_xfer_buf(priv, SPI_READ, regs->status, packed_buf, 4);
	if (rc < 0)
		return rc;

	sja1105_status_unpack(packed_buf, status);

	return 0;
}

/* Not const because unpacking priv->static_config into buffers and preparing
 * for upload requires the recalculation of table CRCs and updating the
 * structures with these.
 */
static int
static_config_buf_prepare_for_upload(struct sja1105_private *priv,
				     void *config_buf, int buf_len)
{
	struct sja1105_static_config *config = &priv->static_config;
	struct sja1105_table_header final_header;
	char *final_header_ptr;
	int crc_len;

	/* Write Device ID and config tables to config_buf */
	sja1105_static_config_pack(config_buf, config);
	/* Recalculate CRC of the last header (right now 0xDEADBEEF).
	 * Don't include the CRC field itself.
	 */
	crc_len = buf_len - 4;
	/* Read the whole table header */
	final_header_ptr = config_buf + buf_len - SJA1105_SIZE_TABLE_HEADER;
	sja1105_table_header_packing(final_header_ptr, &final_header, UNPACK);
	/* Modify */
	final_header.crc = sja1105_crc32(config_buf, crc_len);
	/* Rewrite */
	sja1105_table_header_packing(final_header_ptr, &final_header, PACK);

	return 0;
}

static int sja1105_static_config_upload(struct sja1105_private *priv)
{
	struct sja1105_static_config *config = &priv->static_config;
	const struct sja1105_regs *regs = priv->info->regs;
	struct sja1105_status status;
	u8 *config_buf;
	int buf_len;
	int rc;

	buf_len = sja1105_static_config_get_length(config);
	config_buf = calloc(buf_len, sizeof(char));
	if (!config_buf)
		return -ENOMEM;

	rc = static_config_buf_prepare_for_upload(priv, config_buf, buf_len);
	if (rc < 0) {
		printf("Invalid config, cannot upload\n");
		rc = -EINVAL;
		goto out;
	}
	/* Put the SJA1105 in programming mode */
	rc = priv->info->reset_cmd(priv);
	if (rc < 0) {
		printf("Failed to reset switch\n");
		goto out;
	}
	/* Wait for the switch to come out of reset */
	udelay(1000);
	/* Upload the static config to the device */
	rc = sja1105_xfer_buf(priv, SPI_WRITE, regs->config,
			      config_buf, buf_len);
	if (rc < 0) {
		printf("Failed to upload config\n");
		goto out;
	}
	/* Check that SJA1105 responded well to the config upload */
	rc = sja1105_status_get(priv, &status);
	if (rc < 0)
		goto out;

	if (status.ids == 1) {
		printf("Mismatch between hardware and static config device id. "
		       "Wrote 0x%llx, wants 0x%llx\n",
		       config->device_id, priv->info->device_id);
		rc = -EIO;
		goto out;
	}
	if (status.crcchkl == 1 || status.crcchkg == 1) {
		printf("Switch reported invalid CRC on static config\n");
		rc = -EIO;
		goto out;
	}
	if (status.configs == 0) {
		printf("Switch reported that config is invalid\n");
		rc = -EIO;
		goto out;
	}

out:
	free(config_buf);
	return rc;
}

static int sja1105_static_config_load(struct sja1105_private *priv)
{
	int rc;

	rc = sja1105_static_config_init(&priv->static_config,
					priv->info->static_ops,
					priv->info->device_id);
	if (rc)
		return rc;

	/* Build static configuration */
	rc = sja1105_init_vlan(priv);
	if (rc < 0)
		return rc;
	rc = sja1105_init_mac_settings(priv);
	if (rc < 0)
		return rc;
	rc = sja1105_init_mii_settings(priv);
	if (rc < 0)
		return rc;
	rc = sja1105_init_l2_lookup_params(priv);
	if (rc < 0)
		return rc;
	rc = sja1105_init_l2_forwarding(priv);
	if (rc < 0)
		return rc;
	rc = sja1105_init_l2_forwarding_params(priv);
	if (rc < 0)
		return rc;
	rc = sja1105_init_l2_policing(priv);
	if (rc < 0)
		return rc;
	rc = sja1105_init_general_params(priv);
	if (rc < 0)
		return rc;

	/* Send initial configuration to hardware via SPI */
	return sja1105_static_config_upload(priv);
}

static int sja1105_static_config_reload(struct sja1105_private *priv)
{
	int rc;

	rc = sja1105_static_config_upload(priv);
	if (rc < 0) {
		printf("Failed to load static config: %d\n", rc);
		return rc;
	}

	/* Configure the CGU (PHY link modes and speeds) */
	rc = sja1105_clocking_setup(priv);
	if (rc < 0) {
		printf("Failed to configure MII clocking: %d\n", rc);
		return rc;
	}

	return 0;
}

static int sja1105_port_enable(struct udevice *dev, int port,
			       struct phy_device *phy)
{
	struct sja1105_private *priv = dev_get_priv(dev);
	struct sja1105_xmii_params_entry *mii;
	struct sja1105_mac_config_entry *mac;

	if (phy)
		phy_startup(phy);

	mii = priv->static_config.tables[BLK_IDX_XMII_PARAMS].entries;
	mac = priv->static_config.tables[BLK_IDX_MAC_CONFIG].entries;

	if (mii->xmii_mode[port] == XMII_MODE_SGMII) {
		priv->sgmii_cfg.inband_an = (phy && phy->phy_id != PHY_FIXED_ID);
		priv->sgmii_cfg.pcs_speed = BMCR_SPEED1000;
		mac[port].speed = SJA1105_SPEED_1000MBPS;
	} else {
		int speed = phy ? phy->speed : SPEED_1000;

		switch (speed) {
		case SPEED_1000:
			mac[port].speed = SJA1105_SPEED_1000MBPS;
			break;
		case SPEED_100:
			mac[port].speed = SJA1105_SPEED_100MBPS;
			break;
		case SPEED_10:
			mac[port].speed = SJA1105_SPEED_10MBPS;
			break;
		default:
			printf("Invalid speed %d\n", speed);
			return -EINVAL;
		}
	}

	return sja1105_static_config_reload(priv);
}

static void sja1105_port_disable(struct udevice *dev, int port,
				 struct phy_device *phy)
{
	if (phy)
		phy_shutdown(phy);
}

static int sja1105_xmit(struct udevice *dev, int port, void *packet, int length)
{
	struct sja1105_private *priv = dev_get_priv(dev);
	u8 *from = (u8 *)packet + VLAN_HLEN;
	struct vlan_ethhdr *hdr = packet;
	u8 *dest = (u8 *)packet;

	memmove(dest, from, 2 * ETH_ALEN);
	hdr->h_vlan_proto = htons(ETH_P_SJA1105);
	hdr->h_vlan_TCI = htons(priv->pvid[port]);

	return 0;
}

static int sja1105_rcv(struct udevice *dev, int *port, void *packet, int length)
{
	struct vlan_ethhdr *hdr = packet;
	u8 *dest = packet + VLAN_HLEN;
	u8 *from = packet;

	if (ntohs(hdr->h_vlan_proto) != ETH_P_SJA1105)
		return -EINVAL;

	*port = ntohs(hdr->h_vlan_TCI) & DSA_8021Q_PORT_MASK;
	memmove(dest, from, 2 * ETH_ALEN);

	return 0;
}

static const struct dsa_ops sja1105_dsa_ops = {
	.port_enable	= sja1105_port_enable,
	.port_disable	= sja1105_port_disable,
	.xmit		= sja1105_xmit,
	.rcv		= sja1105_rcv,
};

static int sja1105_init_port(struct sja1105_private *priv, int port)
{
	struct dsa_perdev_platdata *platdata = priv->dev->platdata;
	int phy_mode = PHY_INTERFACE_MODE_NONE;
	struct phy_device *phy;
	const char *if_str;

	if (!ofnode_is_available(platdata->port[port].node)) {
		dev_dbg(priv->dev, "port %d is not available, skipping\n",
			port);
		return 0;
	}

	if_str = ofnode_read_string(platdata->port[port].node, "phy-mode");
	if (if_str)
		phy_mode = phy_get_interface_by_name(if_str);

	phy = platdata->port[port].phy;
	if (phy) {
		if (phy_mode == PHY_INTERFACE_MODE_MII ||
		    phy_mode == PHY_INTERFACE_MODE_RMII) {
			phy->supported &= PHY_BASIC_FEATURES;
			phy->advertising &= PHY_BASIC_FEATURES;
		} else {
			phy->supported &= PHY_GBIT_FEATURES;
			phy->advertising &= PHY_GBIT_FEATURES;
		}
		phy_config(phy);
	}

	if (phy_mode != PHY_INTERFACE_MODE_RGMII_TXID &&
	    phy_mode != PHY_INTERFACE_MODE_RGMII_RXID &&
	    phy_mode != PHY_INTERFACE_MODE_RGMII_ID &&
	    phy_mode != PHY_INTERFACE_MODE_RGMII &&
	    phy_mode != PHY_INTERFACE_MODE_SGMII &&
	    phy_mode != PHY_INTERFACE_MODE_RMII &&
	    phy_mode != PHY_INTERFACE_MODE_MII) {
		printf("Unsupported PHY mode %s!\n", if_str);
		return -EINVAL;
	}

	/* Let the PHY handle the RGMII delays, if present. */
	if (!phy || phy->phy_id == PHY_FIXED_ID) {
		if (phy_mode == PHY_INTERFACE_MODE_RGMII_RXID ||
		    phy_mode == PHY_INTERFACE_MODE_RGMII_ID)
			priv->rgmii_rx_delay[port] = true;

		if (phy_mode == PHY_INTERFACE_MODE_RGMII_TXID ||
		    phy_mode == PHY_INTERFACE_MODE_RGMII_ID)
			priv->rgmii_tx_delay[port] = true;

		if ((priv->rgmii_rx_delay[port] ||
		     priv->rgmii_tx_delay[port]) &&
		     !priv->info->setup_rgmii_delay)
			return -EINVAL;
	}

	priv->phy_modes[port] = phy_mode;

	return 0;
}

static int sja1105_init(struct sja1105_private *priv)
{
	int port;
	int rc;

	for (port = 0; port < SJA1105_NUM_PORTS; port++) {
		rc = sja1105_init_port(priv, port);
		if (rc < 0) {
			printf("Failed to initialize port %d\n", port);
			return rc;
		}
	}

	rc = sja1105_static_config_load(priv);
	if (rc < 0) {
		printf("Failed to load static config: %d\n", rc);
		return rc;
	}

	/* Configure the CGU (PHY link modes and speeds) */
	rc = sja1105_clocking_setup(priv);
	if (rc < 0) {
		printf("Failed to configure MII clocking: %d\n", rc);
		return rc;
	}

	return 0;
}

static int sja1105_check_device_id(struct sja1105_private *priv)
{
	const struct sja1105_regs *regs = priv->info->regs;
	u8 packed_buf[SJA1105_SIZE_DEVICE_ID] = {0};
	u64 device_id;
	u64 part_no;
	int rc;

	rc = sja1105_xfer_buf(priv, SPI_READ, regs->device_id, packed_buf,
			      SJA1105_SIZE_DEVICE_ID);
	if (rc < 0)
		return rc;

	sja1105_packing(packed_buf, &device_id, 31, 0, SJA1105_SIZE_DEVICE_ID,
			UNPACK);

	if (device_id != priv->info->device_id) {
		printf("Expected device ID 0x%llx but read 0x%llx\n",
		       priv->info->device_id, device_id);
		return -ENODEV;
	}

	rc = sja1105_xfer_buf(priv, SPI_READ, regs->prod_id, packed_buf,
			      SJA1105_SIZE_DEVICE_ID);
	if (rc < 0)
		return rc;

	sja1105_packing(packed_buf, &part_no, 19, 4, SJA1105_SIZE_DEVICE_ID,
			UNPACK);

	if (part_no != priv->info->part_no) {
		printf("Expected part number 0x%llx but read 0x%llx\n",
		       priv->info->part_no, part_no);
		return -ENODEV;
	}

	return 0;
}

static int sja1105_probe(struct udevice *dev)
{
	enum sja1105_switch_id id = dev_get_driver_data(dev);
	struct sja1105_private *priv = dev_get_priv(dev);
	int rc;

	if (ofnode_valid(dev->node) && !ofnode_is_available(dev->node)) {
		dev_dbg(dev, "switch disabled\n");
		return -ENODEV;
	}

	priv->info = &sja1105_info[id];
	priv->dev = dev;

	rc = sja1105_check_device_id(priv);
	if (rc < 0) {
		dev_err(dev, "Device ID check failed: %d\n", rc);
		return rc;
	}

	return sja1105_init(priv);
}

static int sja1105_remove(struct udevice *dev)
{
	struct sja1105_private *priv = dev->priv;

	sja1105_static_config_free(&priv->static_config);

	return 0;
}

static int sja1105_bind(struct udevice *dev)
{
	struct dsa_perdev_platdata *pdata = dev->platdata;

	pdata->num_ports = SJA1105_NUM_PORTS;
	pdata->headroom = VLAN_HLEN;

	return 0;
}

static const struct udevice_id sja1105_ids[] = {
	{ .compatible = "nxp,sja1105e", .data = SJA1105E },
	{ .compatible = "nxp,sja1105t", .data = SJA1105T },
	{ .compatible = "nxp,sja1105p", .data = SJA1105P },
	{ .compatible = "nxp,sja1105q", .data = SJA1105Q },
	{ .compatible = "nxp,sja1105r", .data = SJA1105R },
	{ .compatible = "nxp,sja1105s", .data = SJA1105S },
	{ }
};

U_BOOT_DRIVER(sja1105) = {
	.name				= "sja1105",
	.id				= UCLASS_DSA,
	.of_match			= sja1105_ids,
	.bind				= sja1105_bind,
	.probe				= sja1105_probe,
	.remove				= sja1105_remove,
	.ops				= &sja1105_dsa_ops,
	.priv_auto_alloc_size		= sizeof(struct sja1105_private),
	.platdata_auto_alloc_size	= sizeof(struct dsa_perdev_platdata),
};
