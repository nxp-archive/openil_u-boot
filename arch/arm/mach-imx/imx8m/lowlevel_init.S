/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2017 NXP
 */

#include <config.h>

.align 8
.global rom_pointer
rom_pointer:
	.space 256

/*
 * Routine: save_boot_params (called after reset from start.S)
 */

.global save_boot_params
save_boot_params:
	/* The firmware provided ATAG/FDT address can be found in r2/x0 */
	adr	x0, rom_pointer
	stp	x1, x2, [x0], #16
	stp	x3, x4, [x0], #16
	stp	x5, x6, [x0], #16
	stp	x7, x8, [x0], #16
	stp	x9, x10, [x0], #16
	stp	x11, x12, [x0], #16
	stp	x13, x14, [x0], #16
	stp	x15, x16, [x0], #16
	stp	x17, x18, [x0], #16
	stp	x19, x20, [x0], #16
	stp	x21, x22, [x0], #16
	stp	x23, x24, [x0], #16
	stp	x25, x26, [x0], #16
	stp	x27, x28, [x0], #16
	stp	x29, x30, [x0], #16
	mov	x30, sp
	str	x30, [x0], #8

	/* Returns */
	b	save_boot_params_ret

.global restore_boot_params
restore_boot_params:
	adr	x0, rom_pointer
	ldp	x1, x2, [x0], #16
	ldp	x3, x4, [x0], #16
	ldp	x5, x6, [x0], #16
	ldp	x7, x8, [x0], #16
	ldp	x9, x10, [x0], #16
	ldp	x11, x12, [x0], #16
	ldp	x13, x14, [x0], #16
	ldp	x15, x16, [x0], #16
	ldp	x17, x18, [x0], #16
	ldp	x19, x20, [x0], #16
	ldp	x21, x22, [x0], #16
	ldp	x23, x24, [x0], #16
	ldp	x25, x26, [x0], #16
	ldp	x27, x28, [x0], #16
	ldp	x29, x30, [x0], #16
	ldr	x0, [x0]
	mov	sp, x0
	ret

/* Get GIC offset
* For LS1043a rev1.0, GIC base address align with 4k.
* For LS1043a rev1.1, if DCFG_GIC400_ALIGN[GIC_ADDR_BIT]
* is set, GIC base address align with 4K, or else align
* with 64k.
* output:
*	x0: the base address of GICD
*	x1: the base address of GICC
*/
.global get_gic_offset
get_gic_offset:
	ldr     x0, =GICD_BASE
#ifdef CONFIG_GICV2
	ldr     x1, =GICC_BASE
#endif
#ifdef CONFIG_HAS_FEATURE_GIC64K_ALIGN
	ldr     x2, =DCFG_CCSR_SVR
	ldr	w2, [x2]
	rev	w2, w2
	lsr	w3, w2, #16
	ldr	w4, =SVR_DEV(SVR_LS1043A)
	cmp	w3, w4
	b.ne	1f
	ands	w2, w2, #0xff
	cmp	w2, #REV1_0
	b.eq	1f
	ldr	x2, =SCFG_GIC400_ALIGN
	ldr	w2, [x2]
	rev	w2, w2
	tbnz	w2, #GIC_ADDR_BIT, 1f
	ldr     x0, =GICD_BASE_64K
#ifdef CONFIG_GICV2
	ldr     x1, =GICC_BASE_64K
#endif
1:
#endif
	ret
