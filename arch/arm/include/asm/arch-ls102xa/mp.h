/*
 * Copyright 2015, Freescale Semiconductor
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _FSL_LAYERSCAPE_MP_H
#define _FSL_LAYERSCAPE_MP_H

#define SPIN_TABLE_ELEM_ENTRY_ADDR_IDX	0
#define SPIN_TABLE_ELEM_STATUS_IDX	1
#define SPIN_TABLE_ELEM_LPID_IDX	2
#define WORDS_PER_SPIN_TABLE_ENTRY	8	/* pad to 64 bytes */
#define SPIN_TABLE_ELEM_SIZE		64

#define id_to_core(x)	((x & 3) | (x >> 6))
#ifndef __ASSEMBLY__
extern u64 __spin_table[];
extern u64 __real_cntfrq;
extern u64 *secondary_boot_code;
extern size_t __secondary_boot_code_size;
int fsl_layerscape_wakeup_fixed_core(u32 coreid, u32 addr);
void *get_spin_tbl_addr(void);
phys_addr_t determine_mp_bootpg(void);
void secondary_boot_func(void);
int is_core_online(u64 cpu_id);
int get_core_id(void);
#endif
#endif /* _FSL_LAYERSCAPE_MP_H */
