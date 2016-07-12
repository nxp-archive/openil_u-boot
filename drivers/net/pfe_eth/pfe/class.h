/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef _CLASS_H_
#define _CLASS_H_

#define CLASS_DMEM_BASE_ADDR	0x00000000
#define CLASS_DMEM_SIZE		0x2000
#define CLASS_DMEM_END		(CLASS_DMEM_BASE_ADDR + CLASS_DMEM_SIZE)
#define CLASS_PMEM_BASE_ADDR	0x00010000

#define CBUS_BASE_ADDR		0xc0000000
#define CLASS_APB_BASE_ADDR	0xc1000000
#define CLASS_AHB1_BASE_ADDR	0xc2000000
#define CLASS_AHB2_BASE_ADDR	0xc3000000

#include "cbus.h"

#define GPT_BASE_ADDR		(CLASS_APB_BASE_ADDR + 0x00000)
#define UART_BASE_ADDR		(CLASS_APB_BASE_ADDR + 0x10000)
#define PERG_BASE_ADDR		(CLASS_APB_BASE_ADDR + 0x20000)
#define EFET_BASE_ADDR		(CLASS_APB_BASE_ADDR + 0x40000)

#define MAC_HASH_BASE_ADDR	(CLASS_AHB1_BASE_ADDR + 0x30000)
#define VLAN_HASH_BASE_ADDR	(CLASS_AHB1_BASE_ADDR + 0x50000)

#define PE_LMEM_BASE_ADDR	(CLASS_AHB2_BASE_ADDR + 0x10000)
#define PE_LMEM_SIZE		0x8000
#define PE_LMEM_END		(PE_LMEM_BASE_ADDR + PE_LMEM_SIZE)
#define CCU_BASE_ADDR		(CLASS_AHB2_BASE_ADDR + 0x20000)

#define IS_DMEM(addr, len)	(((unsigned long)(addr) >= CLASS_DMEM_BASE_ADDR\
					) && (((unsigned long)(addr) +\
					(len)) <= CLASS_DMEM_END))
#define IS_PE_LMEM(addr, len)	(((unsigned long)(addr) >= PE_LMEM_BASE_ADDR\
					) && (((unsigned long)(addr) +\
					(len)) <= PE_LMEM_END))


#include "gpt.h"
#include "uart.h"
#include "class/perg.h"
#include "class/efet.h"
#include "class/mac_hash.h"
#include "class/vlan_hash.h"
#include "class/ccu.h"


#define CLASS_MAX_PBUFFERS	4

#define PBUF_HWPARSE_OFFSET	0x10	/* Fixed by hardware */

#define PAYLOAD_DMEM_MAX_SIZE	(CLASS_PBUF_SIZE - CLASS_PBUF_HEADER_OFFSET \
					- sizeof(class_rx_hdr_t))

#define PHYPORT_0               0x0
#define PHYPORT_1               0x1
#define HOST_PORT_NO		0x3

#define   ACT_SRC_MAC_REPLACE	(1 << (4+0))
#define   ACT_VLAN_REPLACE		(1 << (4+3))
#define   ACT_TCPCHKSUM_REPLACE	(1 << (4+2))
#define   ACT_VLAN_ADD			(1 << (4+1))

#define MIN_PKT_SIZE		56

#define PARSE_ETH_TYPE		(1 << 0)
#define PARSE_VLAN_TYPE		(1 << 1)
#define PARSE_PPPOE_TYPE	(1 << 2)
#define PARSE_ARP_TYPE		(1 << 3)
#define PARSE_MCAST_TYPE	(1 << 4)
#define PARSE_IP_TYPE		(1 << 5)
#define PARSE_IPV6_TYPE		(1 << 6)
#define PARSE_IPV4_TYPE		(1 << 7)

#define PARSE_IPX_TYPE		(1 << 9)

#define PARSE_UDP_FLOW		(1 << 11)
#define PARSE_TCP_FLOW		(1 << 12)
#define PARSE_ICMP_FLOW		(1 << 13)
#define PARSE_IGMP_FLOW		(1 << 14)
#define PARSE_FRAG_FLOW		(1 << 15)

#define PARSE_HIF_PKT		(1 << 23)
#define PARSE_ARC_HIT		(1 << 24)
#define PARSE_PKT_OVERFLOW	(1 << 25)

#define PARSE_PROTO_MISMATCH	(1 << 28)
#define PARSE_L3_MISMATCH	(1 << 29)
#define PARSE_L2_MISMATCH	(1 << 30)
#define PARSE_INCOMPLETE	(1 << 31)


typedef struct _hwparse_t {
	u16	sid;
	u16	connid;
	u8	toevec;
	u8	pLayer2Hdr;
	u8	pLayer3Hdr;
	u8	pLayer4Hdr;
	u16	vlanid;
	u16	ifParseFlags;
	u32	parseFlags;
	u16	srcport;
	u16	dstport;
	u32	proto:8;
	u32	port:4;
	u32	hash:20;
	u64	rte_res_valid:1;
	u64	vlan_res_valid:1;
	u64	dst_res_valid:1;
	u64	src_res_valid:1;
	u64	vlan_lookup:20;
	u64	dst_lookup:20;
	u64	src_lookup:20;
} hwparse_t;


typedef struct {
	u32	next_ptr;      /* ptr to the start of the first DDR
				* buffer
				*/
	u16	length;		/* total packet length */
	u16	phyno;		/* input physical port number */
	u32	status;		/* gemac status bits */
	u32	res;		/* reserved for software usage */
} class_rx_hdr_t;


typedef struct {
	u8	num_cpy;       /* no of copies to send out from RO block,
				* for each there must be a corresponding
				* tx pre-header
				*/
	u8	dma_len;       /* len to be DMAed to DDR mem, including
				* all tx pre-headers
				*/
	u16	src_addr;      /* class dmem source address, pointing to
				* first tx pre-header
				*/
	u32	dst_addr;      /* DDR memory destination address of
				* first tx pre-header, must be so packet
				* data is continuous in DDR
				*/
	u32	res1;	       /* reserved for software usage -
				* queue number?
				*/
	u16	res2;		/* reserved for software usage */
	u16	tsv;		/* time stamp val */
} class_tx_desc_t;


typedef struct {
	u8	start_data_off;	       /* packet data start
					* offset, relative to start of
					* this tx pre-header
					*/
	u8	start_buf_off;	       /* this tx pre-header start
					* offset, relative to start of
					* DDR buffer
					*/
	u16	pkt_length;		/* total packet lenght */
	u8	act_phyno;		/* action phy number */
	u8	queueno;		/* queueno */
	u16	src_mac_msb;		/* indicates src_mac 47:32 */
	u32	src_mac_lsb;		/* indicates src_mac 31:0 */
	u32	vlanid;			/* vlanid */
} class_tx_hdr_t;

#endif /* _CLASS_H_ */
