/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm-offsets.h>
#include <config.h>
#include <asm/ptrace.h>
#include <asm/macro.h>
#include <linux/linkage.h>

.macro	kernel_entry, el, regsize = 64
	sub sp, sp, #S_FRAME_SIZE

	stp x0, x1, [sp, #16 * 0]
	stp x2, x3, [sp, #16 * 1]
	stp x4, x5, [sp, #16 * 2]
	stp x6, x7, [sp, #16 * 3]
	stp x8, x9, [sp, #16 * 4]
	stp x10, x11, [sp, #16 * 5]
	stp x12, x13, [sp, #16 * 6]
	stp x14, x15, [sp, #16 * 7]
	stp x16, x17, [sp, #16 * 8]
	stp x18, x19, [sp, #16 * 9]
	stp x20, x21, [sp, #16 * 10]
	stp x22, x23, [sp, #16 * 11]
	stp x24, x25, [sp, #16 * 12]
	stp x26, x27, [sp, #16 * 13]
	stp x28, x29, [sp, #16 * 14]


	add x21, sp, #S_FRAME_SIZE

	mrs x22, elr_el3
	mrs x23, spsr_el3
	stp lr, x21, [sp, #S_LR]
	stp x22, x23, [sp, #S_PC]



/*
 * Registers that may be useful after this macro is invoked:
 *
 * x21 - aborted SP
 * x22 - aborted PC
 * x23 - aborted PSTATE
*/
.endm


.macro	kernel_exit, el, ret = 0
	ldp	x21, x22, [sp, #S_PC]		// load ELR, SPSR

	msr	elr_el3, x21			// set up the return data
	msr	spsr_el3, x22

	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]
	ldr	lr, [sp, #S_LR]
	add	sp, sp, #S_FRAME_SIZE		// restore sp
	eret					// return to kernel
.endm




/*
 * Enter Exception.
 * This will save the processor state that is ELR/X0~X30
 * to the stack frame.
 */
.macro	exception_entry
	stp	x29, x30, [sp, #-16]!
	stp	x27, x28, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	stp	x17, x18, [sp, #-16]!
	stp	x15, x16, [sp, #-16]!
	stp	x13, x14, [sp, #-16]!
	stp	x11, x12, [sp, #-16]!
	stp	x9, x10, [sp, #-16]!
	stp	x7, x8, [sp, #-16]!
	stp	x5, x6, [sp, #-16]!
	stp	x3, x4, [sp, #-16]!
	stp	x1, x2, [sp, #-16]!

	/* Could be running at EL3/EL2/EL1 */
	switch_el x11, 3f, 2f, 1f
3:	mrs	x1, esr_el3
	mrs	x2, elr_el3
	b	0f
2:	mrs	x1, esr_el2
	mrs	x2, elr_el2
	b	0f
1:	mrs	x1, esr_el1
	mrs	x2, elr_el1
0:
	stp	x2, x0, [sp, #-16]!
	mov	x0, sp
.endm

/*
 * Exception vectors.
 */
	.align	11
	.globl	vectors
vectors:
	.align	7
	b	_do_bad_sync	/* Current EL Synchronous Thread */

	.align	7
	b	_do_bad_irq	/* Current EL IRQ Thread */

	.align	7
	b	_do_bad_fiq	/* Current EL FIQ Thread */

	.align	7
	b	_do_bad_error	/* Current EL Error Thread */

	.align	7
	b	_do_sync	/* Current EL Synchronous Handler */

	.align	7
	b	_do_irq		/* Current EL IRQ Handler */

	.align	7
	b	_do_fiq		/* Current EL FIQ Handler */

	.align	7
	b	_do_error	/* Current EL Error Handler */


_do_bad_sync:
	exception_entry
	bl	do_bad_sync
	b	exception_exit

_do_bad_irq:
	exception_entry
	bl	do_bad_irq
	b	exception_exit

_do_bad_fiq:
	exception_entry
	bl	do_bad_fiq
	b	exception_exit

_do_bad_error:
	exception_entry
	bl	do_bad_error
	b	exception_exit

_do_sync:
	exception_entry
	bl	do_sync
	b	exception_exit

_do_irq:
	/*exception_entry*/
	kernel_entry 3
	bl	do_irq
    kernel_exit 3

_do_fiq:
	exception_entry
	bl	do_fiq
	b	exception_exit

_do_error:
	exception_entry
	bl	do_error
	b	exception_exit

exception_exit:
	ldp	x2, x0, [sp],#16
	switch_el x11, 3f, 2f, 1f
3:	msr	elr_el3, x2
	b	0f
2:	msr	elr_el2, x2
	b	0f
1:	msr	elr_el1, x2
0:
	ldp	x1, x2, [sp],#16
	ldp	x3, x4, [sp],#16
	ldp	x5, x6, [sp],#16
	ldp	x7, x8, [sp],#16
	ldp	x9, x10, [sp],#16
	ldp	x11, x12, [sp],#16
	ldp	x13, x14, [sp],#16
	ldp	x15, x16, [sp],#16
	ldp	x17, x18, [sp],#16
	ldp	x19, x20, [sp],#16
	ldp	x21, x22, [sp],#16
	ldp	x23, x24, [sp],#16
	ldp	x25, x26, [sp],#16
	ldp	x27, x28, [sp],#16
	ldp	x29, x30, [sp],#16
	eret
