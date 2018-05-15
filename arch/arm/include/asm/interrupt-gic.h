/*
 * Copyright 2018, NXP Semiconductor
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifndef _FSL_LAYERSCAPE_INT_GIC_H
#define _FSL_LAYERSCAPE_INT_GIC_H

#ifndef __ASSEMBLY__
void gic_irq_register(int irq_num, void (*irq_handle)(int, int));
void gic_set_target(u32 core_mask, unsigned long hw_irq);
void gic_set_type(unsigned long hw_irq);
void gic_set_pri_per_cpu(void);
#ifdef CONFIG_ARM64
void gic_set_offset(void);
#endif
void gic_enable_dist(void);
void gic_set_pri_common(void);
void gic_set_pri_irq(u32 hw_irq, u8 pri);
void gic_set_sgi(int core_mask, u32 hw_irq);
#endif
#endif /* _FSL_LAYERSCAPE_INT_GIC_H */
