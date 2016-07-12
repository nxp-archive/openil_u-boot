/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef _PE_H_
#define _PE_H_

#include "hal.h"

#define DDR_BASE_ADDR		0x00020000
#define DDR_END			0x86000000
/* This includes ACP and IRAM areas */
#define IRAM_BASE_ADDR		0x83000000

#define IS_DDR(addr, len)	(((unsigned long)(addr) >= DDR_BASE_ADDR) &&\
				(((unsigned long)(addr) + (len)) <= DDR_END))

typedef struct {

} ddr_rx_hdr_t;

typedef struct {

} lmem_rx_hdr_t;


typedef struct {

} tmu_rx_hdr_t;

typedef struct {

} tmu_tx_hdr_t;

typedef struct {

} util_rx_hdr_t;


struct pe_sync_mailbox {
	u32 stop;
	u32 stopped;
};

struct pe_msg_mailbox {
	u32 dst;
	u32 src;
	u32 len;
	u32 request;
};

/** Basic busy loop delay function
 *
 * @param cycles		Number of cycles to delay (actual cpu cycles
 * should be close to 3 x cycles)
 *
 */
static inline void delay(u32 cycles)
{
	volatile int i;

	for (i = 0; i < cycles; i++)
		;
}


/** Read PE id
 *
 * @return	PE id (0 - 5 for CLASS-PE's, 6 - 9 for TMU-PE's, 10 for
 * UTIL-PE)
 *
 */
static inline u32 esi_get_mpid(void)
{
	u32 mpid;

	asm ("rcsr %0, Configuration, MPID" : "=d" (mpid));

	return mpid;
}

/** 64bit aligned memory copy using efet.
 * Either the source or destination address must be in DMEM, the other address
 * can be in LMEM or DDR.
 * Source, destination addresses and len must all be 64bit aligned.
 * Uses efet synchronous interface to copy the data.
 *
 * @param dst	Destination address to write to (must be 64bit aligned)
 * @param src	Source address to read from (must be 64bit aligned)
 * @param len	Number of bytes to copy (must be 64bit aligned)
 *
 */
void efet_memcpy64(void *dst, void *src, unsigned int len);


/** Aligned memory copy using efet.
 * Either the source or destination address must be in DMEM, the other address
 * can be in LMEM or DDR.
 * Both the source and destination must have the same 64bit alignment, there is
 * no restriction on length.
 *
 * @param dst	Destination address to write to (must have the same 64bit
 * alignment as src)
 * @param src	Source address to read from (must have the same 64bit
 * alignment as dst)
 * @param len	Number of bytes to copy
 *
 */
void efet_memcpy(void *dst, void *src, unsigned int len);


/** 32bit aligned memory copy.
 * Source and destination addresses must be 32bit aligned, there is no
 * restriction on the length.
 *
 * @param dst		Destination address (must be 32bit aligned)
 * @param src		Source address (must be 32bit aligned)
 * @param len		Number of bytes to copy
 *
 */
void memcpy_aligned32(void *dst, void *src, unsigned int len);

/** Aligned memory copy.
 * Source and destination addresses must have the same alignment
 * relative to 32bit boundaries (but otherwsie may have any alignment),
 * there is no restriction on the length.
 *
 * @param dst		Destination address
 * @param src		Source address (must have same 32bit alignment as
 * dst)
 * @param len		Number of bytes to copy
 *
 */
void memcpy_aligned(void *dst, void *src, unsigned int len);


/** Generic memory set.
 * Implements a generic memory set. Not very optimal (uses byte writes for the
 * entire range)
 *
 *
 * @param dst		Destination address
 * @param val		Value to set memory to
 * @param len		Number of bytes to set
 *
 */
void memset(void *dst, u8 val, unsigned int len);

/** Generic memory copy.
 * Implements generic memory copy. If source and destination have the same
 * alignment memcpy_aligned() is used, otherwise, we first align the destination
 * to a 32bit boundary (using byte copies) then the src, and finally use a loop
 * of read, shift, write
 *
 * @param dst		Destination address
 * @param src		Source address
 * @param len		Number of bytes to copy
 *
 */
void memcpy(void *dst, void *src, unsigned int len);

#endif /* _PE_H_ */
