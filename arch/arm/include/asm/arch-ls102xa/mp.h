// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2021 NXP
 *
 */

#ifndef _FSL_LAYERSCAPE_MP_H
#define _FSL_LAYERSCAPE_MP_H

#define id_to_core(x)	((x & 3) | (x >> 6))
#ifndef __ASSEMBLY__
int fsl_layerscape_wakeup_fixed_core(u32 coreid);
int is_core_online(u64 cpu_id);
int get_core_id(void);
int is_core_valid(unsigned int core);
#endif
#endif /* _FSL_LAYERSCAPE_MP_H */
