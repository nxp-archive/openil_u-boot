/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#include "hal.h"
#include "pfe/pfe.h"
#include "pfe_driver.h"
#include "pfe_firmware.h"


static struct tx_desc_s *g_tx_desc;
static struct rx_desc_s *g_rx_desc;

#define wmb()	asm volatile("dsb st" : : : "memory")

/** HIF Rx interface function
 * Reads the rx descriptor from the current location (rxToRead).
 * - If the descriptor has a valid data/pkt, then get the data pointer
 * - check for the input rx phy number
 * - increments the rx data pointer by pkt_head_room_size
 * - decrements the data length by pkt_head_room_size
 * - handover the packet to caller.
 *
 * @param[out]	pkt_ptr	Pointer to store rx packet pointer
 * @param[out] phy_port Pointer to store recv phy port
 *
 * @return	-1 if no packet, else returns length of packet.
 */
int pfe_recv(unsigned int *pkt_ptr, int *phy_port)
{
	struct rx_desc_s *rx_desc = g_rx_desc;
	struct bufDesc *bd;
	int len = -1;

	volatile u32 ctrl;
	struct hif_header_s *hif_header;

	bd = rx_desc->rxBase + rx_desc->rxToRead;

	if (bd->ctrl & BD_CTRL_DESC_EN)
		return len; /* No pending Rx packet */

	/* this len include hif_header(8bytes) */
	len = bd->ctrl & 0xFFFF;

	hif_header = (struct hif_header_s *)DDR_PFE_TO_VIRT(bd->data);


	/* Get the recive port info from the packet */
	dprint(
		"Pkt recv'd: Pkt ptr(%p), len(%d), gemac_port(%d) status(%08x)\n",
		hif_header, len, hif_header->port_no, bd->status);

#if DEBUG
	{
		int i;
		unsigned char *p = (unsigned char *)hif_header;

		for (i = 0; i < len; i++) {
			if (!(i % 16))
				printf("\n");
			printf(" %02x", p[i]);
		}
		printf("\n");
	}
#endif

	*pkt_ptr = (unsigned long)(hif_header + 1);
	*phy_port = hif_header->port_no;
	len -= sizeof(struct hif_header_s);

	/* reset bd control field */
	ctrl = (MAX_FRAME_SIZE | BD_CTRL_LIFM | BD_CTRL_DESC_EN | BD_CTRL_DIR);
	rx_desc->rxToRead = (rx_desc->rxToRead + 1) & (rx_desc->rxRingSize -
				1);

	bd->ctrl = ctrl;
	bd->status = 0;


	/* Give START_STROBE to BDP to fetch the descriptor __NOW__,
	 * BDP need not to wait for rx_poll_cycle time to fetch the descriptor,
	 * In idle state (ie., no rx pkt), BDP will not fetch
	 * the descriptor even if strobe is given(I think)
	 */
	writel((readl(HIF_RX_CTRL) | HIF_CTRL_BDP_CH_START_WSTB), HIF_RX_CTRL);

	return len;
}

void pfe_recv_ack(void)
{
	struct rx_desc_s *rx_desc = g_rx_desc;
	struct bufDesc *bd;
	volatile u32 ctrl;

	bd = rx_desc->rxBase + rx_desc->rxToRead;

	/* reset bd control field */
	ctrl = (MAX_FRAME_SIZE | BD_CTRL_DESC_EN | BD_CTRL_DIR);
	bd->ctrl = ctrl;
	bd->status = 0;

	rx_desc->rxToRead = (rx_desc->rxToRead + 1) & (rx_desc->rxRingSize -
				1);

	/* Give START_STROBE to BDP to fetch the descriptor __NOW__,
	 * BDP need not to wait for rx_poll_cycle time to fetch the descriptor,
	 * In idle state (ie., no rx pkt), BDP will not fetch
	 * the descriptor even if strobe is given(I think)
	 */
	writel((readl(HIF_RX_CTRL) | HIF_CTRL_BDP_CH_START_WSTB), HIF_RX_CTRL);
}


/** HIF Tx interface function
 * This function sends a single packet to PFE from HIF interface.
 * - No interrupt indication on tx completion.
 * - After tx descriptor is updated and TX DMA is enabled.
 * - To support both chipit and read c2k environment, data is copied to
 *   tx buffers. After verification this copied can be avoided.
 *
 * @param[in] phy_port	Phy port number to send out this packet
 * @param[in] data	Pointer to the data
 * @param[in] length	Length of the ethernet packet to be transfered.
 *
 * @return -1 if tx Q is full, else returns the tx location where the pkt is
 * placed.
 */
int pfe_send(int phy_port, void *data, int length)
{
	struct tx_desc_s *tx_desc = g_tx_desc;
	struct bufDesc *bd;
	struct hif_header_s hif_header;
	u8 *tx_buf_va;
	volatile u32 ctrl_word;

	dprint("%s:pkt: %p, len: %d, txBase: %p, txToSend: %d\n", __func__,
			data, length, tx_desc->txBase, tx_desc->txToSend);

	bd = tx_desc->txBase + tx_desc->txToSend;

	/* check queue-full condition */
	if (bd->ctrl & BD_CTRL_DESC_EN) {
		printf("Tx queue full\n");
		return -1;
	}

	/* PFE checks for min pkt size */
	if (length < MIN_PKT_SIZE)
		length = MIN_PKT_SIZE;

	tx_buf_va = (void *)DDR_PFE_TO_VIRT(bd->data);
	dprint("%s: tx_buf_va: %p, tx_buf_pa: %08x\n", __func__, tx_buf_va,
			bd->data);

	/* Fill the gemac/phy port number to send this packet out */
	memset(&hif_header, 0, sizeof(struct hif_header_s));
	hif_header.port_no = phy_port;

	memcpy(tx_buf_va, (u8 *)&hif_header, sizeof(struct hif_header_s));
	memcpy(tx_buf_va + sizeof(struct hif_header_s), data, length);
	length += sizeof(struct hif_header_s);

#if 0
	{
		int i;
		unsigned char *p = (unsigned char *)tx_buf_va;

		for (i = 0; i < length; i++) {
			if (!(i % 16))
				printf("\n");
			printf("%02x ", p[i]);
		}
	}
#endif

	dprint("before0: Tx Done, status: %08x, ctrl: %08x\n", bd->status,
		bd->ctrl);

	/* fill the tx desc */
	ctrl_word = (u32)(BD_CTRL_DESC_EN | BD_CTRL_LIFM | (length & 0xFFFF));
	bd->ctrl = ctrl_word;
	bd->status = 0;

	/* NOTE: This code can be removed after verification */
#if 1 /* SRAM_RETENTION_BUG */
	ctrl_word = 0;
	bd->status = 0xF0;
	ctrl_word = bd->ctrl;
#endif
	wmb();

	/* Indicate Tx DMA to start fetching the Tx Descriptor,
	 * set START_STOBE
	 */
	/* writel((readl(HIF_TX_CTRL) | HIF_TX_BDP_CH_START_WSTB), HIF_TX_CTRL);
	 */
	/* writel((readl(HIF_TX_CTRL) | (HIF_TX_DMA_EN |
	 * HIF_TX_BDP_CH_START_WSTB)), HIF_TX_CTRL);
	 */
	writel((HIF_CTRL_DMA_EN | HIF_CTRL_BDP_CH_START_WSTB), HIF_TX_CTRL);

	udelay(100);

	return tx_desc->txToSend;
}

/** HIF to check the Tx done
 *  This function will chceck the tx done indication of the current txToSend
 * locations
 *  if success, moves the txToSend to next location.
 *
 * @return -1 if TX ownership bit is not cleared by hw.
 * else on success (tx done copletion) returns zero.
 */
int pfe_tx_done(void)
{
	struct tx_desc_s *tx_desc = g_tx_desc;
	struct bufDesc *bd;
	volatile u32 ctrl_word;

	dprint("%s:txBase: %p, txToSend: %d\n", __func__, tx_desc->txBase,
		tx_desc->txToSend);

	bd = tx_desc->txBase + tx_desc->txToSend;

	/* check queue-full condition */
	ctrl_word = bd->ctrl;
	if (ctrl_word & BD_CTRL_DESC_EN)
		return -1;

	/* reset the control field */
	bd->ctrl = 0;
	/* bd->data = (u32)NULL; */
	bd->status = 0;

	dprint("Tx Done : status: %08x, ctrl: %08x\n", bd->status, bd->ctrl);

	/* increment the txtosend index to next location */
	tx_desc->txToSend = (tx_desc->txToSend + 1) & (tx_desc->txRingSize -
				1);

	dprint("Tx next pkt location: %d\n", tx_desc->txToSend);

	return 0;
}
#if defined CONFIG_LS1024A
/** GEMAC initialization
 * Initializes the GEMAC registers.
 *
 * @param[in] gemac_base   Pointer to GEMAC reg base
 * @param[in] mode GEMAC mode to configure (MII config)
 * @param[in] speed GEMAC speed
 * @param[in] duplex
 */
void pfe_gemac_init(void *gemac_base, u32 mode, u32 speed, u32 duplex)
{
	GEMAC_CFG gemac_cfg  = {
		.mode = mode,
		.speed = speed,
		.duplex = duplex,
	};

	dprint("%s: gemac_base=%p\n", __func__, gemac_base);

	gemac_init(gemac_base, &gemac_cfg);

	/* gemac_set_loop(gemac_base, LB_NONE); */
	/* gemac_disable_copy_all(gemac_base); */
	/* gemac_disable_rx_checksum_offload(gemac_base); */

	gemac_allow_broadcast(gemac_base);
	gemac_disable_unicast(gemac_base); /* unicast hash disabled  */
	gemac_disable_multicast(gemac_base); /* multicast hash disabled */
	gemac_disable_fcs_rx(gemac_base);
	gemac_disable_1536_rx(gemac_base);
	gemac_enable_pause_rx(gemac_base);
	gemac_enable_rx_checksum_offload(gemac_base);
}
#endif
/** Helper function to dump Rx descriptors.
 */
void hif_rx_desc_dump(void)
{
	struct bufDesc *bd_va;
	int i;
	struct rx_desc_s *rx_desc;

	if (g_rx_desc == NULL) {
		printf("%s: HIF Rx desc no init\n", __func__);
		return;
	}

	rx_desc = g_rx_desc;
	bd_va = rx_desc->rxBase;

	dprint("HIF rx desc: base_va: %p, base_pa: %08x\n", rx_desc->rxBase,
		rx_desc->rxBase_pa);
	for (i = 0; i < rx_desc->rxRingSize; i++) {
		dprint("status: %08x, ctrl: %08x, data: %08x, next: %p\n",
			bd_va->status, bd_va->ctrl, bd_va->data, bd_va->next);
		bd_va++;
	}
}

/** This function mark all Rx descriptors as LAST_BD.
 */
void hif_rx_desc_disable(void)
{
	int i;
	struct rx_desc_s *rx_desc;
	struct bufDesc *bd_va;

	if (g_rx_desc == NULL) {
		printf("%s: HIF Rx desc not initialized\n", __func__);
		return;
	}

	rx_desc = g_rx_desc;
	bd_va = rx_desc->rxBase;

	for (i = 0; i < rx_desc->rxRingSize; i++) {
		bd_va->ctrl |= BD_CTRL_LAST_BD;
		bd_va++;
	}
}


/** HIF Rx Desc initialization function.
 */
static int hif_rx_desc_init(struct pfe *pfe)
{
	u32 ctrl;
	struct bufDesc *bd_va;
	struct bufDesc *bd_pa;
	struct rx_desc_s *rx_desc;
	u32 rx_buf_pa;
	int i;

	/* sanity check */
	if (g_rx_desc) {
		printf("%s: HIF Rx desc re-init request\n", __func__);
		return 0;
	}

	rx_desc = (struct rx_desc_s *)malloc(sizeof(struct rx_desc_s));
	if (rx_desc == NULL) {
		printf("%s:%d:Memory allocation failure\n", __func__,
			__LINE__);
		return -1;
	}
	memset(rx_desc, 0, sizeof(struct rx_desc_s));

	/* init: Rx ring buffer */
	rx_desc->rxRingSize = HIF_RX_DESC_NT;

	/* NOTE: must be 64bit aligned  */
	bd_va = (struct bufDesc *)(pfe->ddr_baseaddr + RX_BD_BASEADDR);
	bd_pa = (struct bufDesc *)(pfe->ddr_phys_baseaddr + RX_BD_BASEADDR);

	rx_desc->rxBase = bd_va;
	rx_desc->rxBase_pa = (unsigned long)bd_pa;

	rx_buf_pa = pfe->ddr_phys_baseaddr + HIF_RX_PKT_DDR_BASEADDR;


	printf("%s: Rx desc base: %p, base_pa: %08x, desc_count: %d\n",
		__func__, rx_desc->rxBase, rx_desc->rxBase_pa,
		rx_desc->rxRingSize);

	memset(bd_va, 0, sizeof(struct bufDesc) * rx_desc->rxRingSize);

	ctrl = (MAX_FRAME_SIZE | BD_CTRL_DESC_EN | BD_CTRL_DIR | BD_CTRL_LIFM);

	for (i = 0; i < rx_desc->rxRingSize; i++) {
		bd_va->next = (unsigned long)(bd_pa + 1);
		bd_va->ctrl = ctrl;
		bd_va->data = rx_buf_pa + (i * MAX_FRAME_SIZE);
		bd_va++;
		bd_pa++;
	}
	--bd_va;
	bd_va->next = (u32)rx_desc->rxBase_pa;

	/* !!! This is a redundent information for h/w as we are also
	 * maintaining next address in the buffer descriptor
	 * Posedge: reference code does not using this bit to go back to
	 * base address
	 */
	/* bd->ctrl |= BD_CTRL_LAST_BD; */

	writel(rx_desc->rxBase_pa, HIF_RX_BDP_ADDR);
	writel((readl(HIF_RX_CTRL) | HIF_CTRL_BDP_CH_START_WSTB), HIF_RX_CTRL);

	g_rx_desc = rx_desc;

	return 0;
}

/** Helper function to dump Tx Descriptors.
 */
void hif_tx_desc_dump(void)
{
	struct tx_desc_s *tx_desc;
	int i;
	struct bufDesc *bd_va;

	if (g_tx_desc == NULL) {
		printf("%s: HIF Tx desc no init\n", __func__);
		return;
	}

	tx_desc = g_tx_desc;
	bd_va = tx_desc->txBase;

	printf("HIF tx desc: base_va: %p, base_pa: %08x\n", tx_desc->txBase,
		tx_desc->txBase_pa);
	for (i = 0; i < tx_desc->txRingSize; i++)
		bd_va++;
}

/** HIF Tx descriptor initialization function.
 */
static int hif_tx_desc_init(struct pfe *pfe)
{
	struct bufDesc *bd_va;
	struct bufDesc *bd_pa;
	int i;
	struct tx_desc_s *tx_desc;
	u32 tx_buf_pa;

	/* sanity check */
	if (g_tx_desc) {
		printf("%s: HIF Tx desc re-init request\n", __func__);
		return 0;
	}

	tx_desc = (struct tx_desc_s *)malloc(sizeof(struct tx_desc_s));
	if (tx_desc == NULL) {
		printf("%s:%d:Memory allocation failure\n", __func__,
			__LINE__);
		return -1;
	}
	memset(tx_desc, 0, sizeof(struct tx_desc_s));

	/* init: Tx ring buffer */
	tx_desc->txRingSize = HIF_TX_DESC_NT;
	/* NOTE: must be 64bit aligned  */
	bd_va = (struct bufDesc *)(pfe->ddr_baseaddr + TX_BD_BASEADDR);
	bd_pa = (struct bufDesc *)(pfe->ddr_phys_baseaddr + TX_BD_BASEADDR);

	tx_desc->txBase_pa = (unsigned long)bd_pa;
	tx_desc->txBase = bd_va;

	printf("%s: Tx desc_base: %p, base_pa: %08x, desc_count: %d\n",
			__func__, tx_desc->txBase, tx_desc->txBase_pa,
		tx_desc->txRingSize);

	memset(bd_va, 0, sizeof(struct bufDesc) * tx_desc->txRingSize);

	tx_buf_pa = pfe->ddr_phys_baseaddr + HIF_TX_PKT_DDR_BASEADDR;

	for (i = 0; i < tx_desc->txRingSize; i++) {
		bd_va->next = (unsigned long)(bd_pa + 1);
		bd_va->data = tx_buf_pa + (i * MAX_FRAME_SIZE);
		bd_va++;
		bd_pa++;
	}
	--bd_va;
	bd_va->next = (u32)tx_desc->txBase_pa;

	/* !!! This is a redundent information for h/w as we are also
	 * maintaining next address in the buffer descriptor,
	 * Posedge: reference code does not using LAST_BD for moving back
	 * to base address
	 */
	/* bd->ctrl |= BD_CTRL_LAST_BD; */

	writel(tx_desc->txBase_pa, HIF_TX_BDP_ADDR);

	g_tx_desc = tx_desc;

	return 0;
}

/** PFE/Class initialization.
 */
static void pfe_class_init(struct pfe *pfe)
{
	CLASS_CFG class_cfg = {
		.route_table_baseaddr = pfe->ddr_phys_baseaddr +
					ROUTE_TABLE_BASEADDR,
		.route_table_hash_bits = ROUTE_TABLE_HASH_BITS,
	};

	class_init(&class_cfg);
	printf("class init complete\n");
}

/** PFE/TMU initialization.
 */
static void pfe_tmu_init(struct pfe *pfe)
{
	TMU_CFG tmu_cfg = {
		.llm_base_addr = pfe->ddr_phys_baseaddr + TMU_LLM_BASEADDR,
		.llm_queue_len = TMU_LLM_QUEUE_LEN,
	};

	tmu_init(&tmu_cfg);
	printf("tmu init complete\n");
}

/** PFE/BMU (both BMU1 & BMU2) initialization.
 */
static void pfe_bmu_init(struct pfe *pfe)
{
	BMU_CFG bmu1_cfg = {
		.baseaddr = CBUS_VIRT_TO_PFE(LMEM_BASE_ADDR +
						BMU1_LMEM_BASEADDR),
		.count = BMU1_BUF_COUNT,
		.size = BMU1_BUF_SIZE,
	};

	BMU_CFG bmu2_cfg = {
		.baseaddr = pfe->ddr_phys_baseaddr + BMU2_DDR_BASEADDR,
		.count = BMU2_BUF_COUNT,
		.size = BMU2_BUF_SIZE,
	};

	bmu_init(BMU1_BASE_ADDR, &bmu1_cfg);
	printf("bmu1 init: done\n");

	bmu_init(BMU2_BASE_ADDR, &bmu2_cfg);
	printf("bmu2 init: done\n");
}

#if !defined(CONFIG_UTIL_PE_DISABLED)
/** PFE/Util initialization function.
 */
static void pfe_util_init(struct pfe *pfe)
{
	UTIL_CFG util_cfg = { };

	util_init(&util_cfg);
	printf("util init complete\n");
}
#endif

/** PFE/GPI initialization function.
 *  - egpi1, egpi2, egpi3, hgpi
 */
static void pfe_gpi_init(struct pfe *pfe)
{
	GPI_CFG egpi1_cfg = {
		.lmem_rtry_cnt = EGPI1_LMEM_RTRY_CNT,
		.tmlf_txthres = EGPI1_TMLF_TXTHRES,
		.aseq_len = EGPI1_ASEQ_LEN,
	};

	GPI_CFG egpi2_cfg = {
		.lmem_rtry_cnt = EGPI2_LMEM_RTRY_CNT,
		.tmlf_txthres = EGPI2_TMLF_TXTHRES,
		.aseq_len = EGPI2_ASEQ_LEN,
	};

#if 0
	GPI_CFG egpi3_cfg = {
		.lmem_rtry_cnt = EGPI3_LMEM_RTRY_CNT,
		.tmlf_txthres = EGPI3_TMLF_TXTHRES,
		.aseq_len = EGPI3_ASEQ_LEN,
	};
#endif

	GPI_CFG hgpi_cfg = {
		.lmem_rtry_cnt = HGPI_LMEM_RTRY_CNT,
		.tmlf_txthres = HGPI_TMLF_TXTHRES,
		.aseq_len = HGPI_ASEQ_LEN,
	};

	gpi_init(EGPI1_BASE_ADDR, &egpi1_cfg);
	printf("GPI1 init complete\n");

	gpi_init(EGPI2_BASE_ADDR, &egpi2_cfg);
	printf("GPI2 init complete\n");

#if 0
	gpi_init(EGPI3_BASE_ADDR, &egpi3_cfg);
#endif

gpi_init(HGPI_BASE_ADDR, &hgpi_cfg);
	printf("HGPI init complete\n");
}


/** PFE/HIF initialization function.
 */
static void pfe_hif_init(struct pfe *pfe)
{
	hif_tx_disable();
	hif_rx_disable();

	hif_tx_desc_init(pfe);
	hif_rx_desc_init(pfe);

	hif_init();

	hif_tx_enable();
	hif_rx_enable();

	hif_rx_desc_dump();
	hif_tx_desc_dump();

	printf("HIF init complete\n");
}

/** PFE initialization
 * - Firmware loading (CLASS-PE and TMU-PE)
 * - BMU1 and BMU2 init
 * - GEMAC init
 * - GPI init
 * - CLASS-PE init
 * - TMU-PE init
 * - HIF tx and rx descriptors init
 *
 * @param[in]	edev	Pointer to eth device structure.
 *
 * @return 0, on success.
 */
static int pfe_hw_init(struct pfe *pfe)
{

	dprint("%s: start\n", __func__);
#if defined(CONFIG_LS1012A)
	/* This clock workaround needed for LS1012 */
	writel(0x3,     CLASS_PE_SYS_CLK_RATIO);
	writel(0x3,	TMU_PE_SYS_CLK_RATIO);
	writel(0x3,     UTIL_PE_SYS_CLK_RATIO);
	udelay(10);
#endif

	pfe_class_init(pfe);

	pfe_tmu_init(pfe);

	pfe_bmu_init(pfe);

#if !defined(CONFIG_UTIL_PE_DISABLED)
	pfe_util_init(pfe);
#endif

	pfe_gpi_init(pfe);

	pfe_hif_init(pfe);

	bmu_enable(BMU1_BASE_ADDR);
	printf("bmu1 enabled\n");

	bmu_enable(BMU2_BASE_ADDR);
	printf("bmu2 enabled\n");

	printf("%s: done\n", __func__);

	/* NOTE: Load PE specific data (if any) */

	return 0;
}


/** PFE probe function.
 * - Initializes pfe_lib
 * - pfe hw init
 * - fw loading and enables PEs
 * - should be executed once.
 *
 * @param[in] pfe  Pointer the pfe control block
 */
int pfe_probe(struct pfe *pfe)
{
	static int init_done;

	if (init_done)
		return 0;

	printf(
		"cbus_baseaddr: %p, ddr_baseaddr: %p, ddr_phys_baseaddr: %08x\n",
		pfe->cbus_baseaddr, pfe->ddr_baseaddr,
		(u32)pfe->ddr_phys_baseaddr);

	pfe_lib_init(pfe->cbus_baseaddr, pfe->ddr_baseaddr,
			pfe->ddr_phys_baseaddr);


	pfe_hw_init(pfe);

	/* Load the class,TM, Util fw
	 * by now pfe is,
	 * - out of reset + disabled + configured,
	 * Fw loading should be done after pfe_hw_init()
	 */
#ifdef CONFIG_CMD_PFE_START
	/* It loads firmware from DDR locations Class@0x100000 TMU@0x180000
	 * UTIL@200000
	 */
	/* For this firmware should be preloaded in DDR */
	/* pfe_firmware_init((u8 *)0x80100000, (u8 *)0x80180000, 0x80200000); */
	pfe_firmware_init(NULL, NULL, NULL);
#else
	/* It loads default inbuilt sbl firmware */
	pfe_firmware_init(NULL, NULL, NULL);
#endif

	init_done = 1;

	return 0;
}


/** PFE remove function
 *  - stopes PEs
 *  - frees tx/rx descriptor resources
 *  - should be called once.
 *
 * @param[in] pfe Pointer to pfe control block.
 */
int pfe_remove(struct pfe *pfe)
{
	if (g_tx_desc)
		free(g_tx_desc);

	if (g_rx_desc)
		free(g_rx_desc);

	pfe_firmware_exit();

	return 0;
}

