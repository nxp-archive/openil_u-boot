/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef __PFE_DRIVER_H__
#define __PFE_DRIVER_H__

#include "hal.h"

#include "pfe/pfe.h"
#include "pfe/cbus.h"
#include "pfe/cbus/bmu.h"



typedef struct bufDesc {
	volatile u32 ctrl;
	volatile u32 status;
	volatile u32 data;
	volatile u32 next;
		 /* struct bufDesc *next; */
} __attribute__((packed)) bufDesc_t;

#if defined(CONFIG_PLATFORM_PCI)
#define HIF_RX_DESC_NT		4
#define	HIF_TX_DESC_NT		4
#else
#define HIF_RX_DESC_NT		64
#define	HIF_TX_DESC_NT		64
#endif
#define RX_BD_BASEADDR		(HIF_DESC_BASEADDR)
#define TX_BD_BASEADDR		(HIF_DESC_BASEADDR + HIF_TX_DESC_SIZE)

#define MIN_PKT_SIZE		56
#define MAX_FRAME_SIZE     2048


typedef struct hif_header_s {
	u8	port_no; /* Carries input port no for host rx packets and
			  * output port no for tx pkts
			  */
	u8 reserved0;
	u32 reserved2;
} __attribute__((packed)) hif_header_t;


typedef struct rx_desc_s {
	struct bufDesc *rxBase;
	unsigned int rxBase_pa;
	int rxToRead;
	int rxRingSize;
} rx_desc_t;

typedef struct tx_desc_s {
	struct bufDesc *txBase;
	unsigned int txBase_pa;
	int txToSend;
	int txRingSize;
} tx_desc_t;


/* The set of statistics registers implemented in the Cadence MAC.
 * The statistics registers implemented are a subset of all the statistics
 * available, but contains all the compulsory ones.
 */
typedef struct gem_stats {
	u32 octets_tx_bot;      /* Lower 32-bits for number of octets tx'd */
	u32 octets_tx_top;      /* Upper 16-bits for number of octets tx'd */
	u32 frames_tx;          /* Number of frames transmitted OK */
	u32 broadcast_tx;       /* Number of broadcast frames transmitted */
	u32 multicast_tx;       /* Number of multicast frames transmitted */
	u32 pause_tx;           /* Number of pause frames transmitted. */
	u32 frame64_tx;         /* Number of 64byte frames transmitted */
	u32 frame65_127_tx;     /* Number of 65-127 byte frames transmitted */
	u32 frame128_255_tx;    /* Number of 128-255 byte frames transmitted */
	u32 frame256_511_tx;    /* Number of 256-511 byte frames transmitted */
	u32 frame512_1023_tx;   /* Number of 512-1023 byte frames transmitted */
	u32 frame1024_1518_tx;  /* Number of 1024-1518 byte
				 * frames transmitted
				 */
	u32 frame1519_tx;       /* Number of frames greater than
				 * 1518 bytes tx
				 */
	u32 tx_urun;            /* Transmit underrun errors due to DMA */
	u32 single_col;         /* Number of single collision frames */
	u32 multi_col;          /* Number of multi collision frames */
	u32 excess_col;         /* Number of excessive collision frames. */
	u32 late_col;           /* Collisions occuring after slot time */
	u32 def_tx;             /* Frames deferred due to crs */
	u32 crs_errors;         /* Errors caused by crs not being asserted. */
	u32 octets_rx_bot;      /* Lower 32-bits for number of octets rx'd */
	u32 octets_rx_top;      /* Upper 16-bits for number of octets rx'd */
	u32 frames_rx;          /* Number of frames received OK */
	u32 broadcast_rx;       /* Number of broadcast frames received */
	u32 multicast_rx;       /* Number of multicast frames received */
	u32 pause_rx;           /* Number of pause frames received. */
	u32 frame64_rx;         /* Number of 64byte frames received */
	u32 frame65_127_rx;     /* Number of 65-127 byte frames received */
	u32 frame128_255_rx;    /* Number of 128-255 byte frames received */
	u32 frame256_511_rx;    /* Number of 256-511 byte frames received */
	u32 frame512_1023_rx;   /* Number of 512-1023 byte frames received */
	u32 frame1024_1518_rx;  /* Number of 1024-1518 byte frames received */
	u32 frame1519_rx;       /* Number of frames greater than
				 * 1518 bytes rx
				 */
	u32 usize_frames;       /* Frames received less than min of 64 bytes */
	u32 excess_length;      /* Number of excessive length frames rx */
	u32 jabbers;            /* Excessive length + crc or align errors. */
	u32 fcs_errors;         /* Number of frames received with crc errors */
	u32 length_check_errors;/* Number of frames with incorrect length */
	u32 rx_symbol_errors;   /* Number of times rx_er asserted during rx */
	u32 align_errors;       /* Frames received without integer no. bytes */
	u32 rx_res_errors;      /* Number of times buffers ran out during rx */
	u32 rx_orun;            /* Receive overrun errors due to DMA */
	u32 ip_cksum;           /* IP header checksum errors */
	u32 tcp_cksum;           /* TCP checksum errors */
	u32 udp_cksum;           /* UDP checksum errors */
} volatile GEM_STATS;


int pfe_send(int phy_port, void *data, int length);
int pfe_recv(unsigned int *pkt_ptr, int *phy_port);
void pfe_recv_ack(void);
int pfe_tx_done(void);
void pfe_gem_enable_all(void);
void pfe_gemac_init(void *gemac_base, u32 mode, u32 speed, u32 duplex);


#endif

