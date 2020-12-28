// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2003
 * Texas Instruments <www.ti.com>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002-2004
 * Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
 *
 * (C) Copyright 2004
 * Philippe Robin, ARM Ltd. <philippe.robin@arm.com>
 *
 * Copyright 2018-2021 NXP
 */

#include <common.h>
#include <cpu_func.h>
#include <efi_loader.h>
#include <irq_func.h>
#include <asm/proc-armv/ptrace.h>
#include <asm/u-boot-arm.h>

DECLARE_GLOBAL_DATA_PTR;

#define GICD_BASE       0x01401000
#define GICC_BASE       0x01402000
#define GIC_CPU_CTRL			0x00
#define GIC_CPU_PRIMASK			0x04
#define GIC_CPU_BINPOINT		0x08
#define GIC_CPU_INTACK			0x0c
#define GIC_CPU_EOI			0x10
#define GIC_CPU_RUNNINGPRI		0x14
#define GIC_CPU_HIGHPRI			0x18
#define GIC_CPU_ALIAS_BINPOINT		0x1c
#define GIC_CPU_ACTIVEPRIO		0xd0
#define GIC_CPU_IDENT			0xfc

#define GICC_ENABLE			0x7
#define GICC_INT_PRI_THRESHOLD		0xf0
#define GICC_IAR_MASK			0x1fff
#define GICC_IAR_INT_ID_MASK		0x3ff
#define GICC_INT_SPURIOUS		1023
#define GICC_DIS_BYPASS_MASK		0x1e0

#define GIC_DIST_CTRL			0x000
#define GIC_DIST_CTR			0x004
#define GIC_DIST_IGROUP			0x080
#define GIC_DIST_ENABLE_SET		0x100
#define GIC_DIST_ENABLE_CLEAR		0x180
#define GIC_DIST_PENDING_SET		0x200
#define GIC_DIST_PENDING_CLEAR		0x280
#define GIC_DIST_ACTIVE_SET		0x300
#define GIC_DIST_ACTIVE_CLEAR		0x380
#define GIC_DIST_PRI			0x400
#define GIC_DIST_TARGET			0x800
#define GIC_DIST_CONFIG			0xc00
#define GIC_DIST_SOFTINT		0xf00
#define GIC_DIST_SGI_PENDING_CLEAR	0xf10
#define GIC_DIST_SGI_PENDING_SET	0xf20

#define GICD_ENABLE			0x3
#define GICD_DISABLE			0x0
#define GICD_INT_ACTLOW_LVLTRIG		0x0
#define GICD_INT_EN_CLR_X32		0xffffffff
#define GICD_INT_EN_SET_SGI		0x0000ffff
#define GICD_INT_EN_CLR_PPI		0xffff0000
#define GICD_INT_DEF_PRI		0xa0
#define GICD_INT_DEF_PRI_X4		((GICD_INT_DEF_PRI << 24) |\
					(GICD_INT_DEF_PRI << 16) |\
					(GICD_INT_DEF_PRI << 8) |\
					GICD_INT_DEF_PRI)

#define GICH_HCR		0x0
#define GICH_VTR		0x4
#define GICH_VMCR		0x8
#define GICH_MISR		0x10
#define GICH_EISR0		0x20
#define GICH_EISR1		0x24
#define GICH_ELRSR0		0x30
#define GICH_ELRSR1		0x34
#define GICH_APR		0xf0
#define GICH_LR0		0x100

#define GICH_HCR_EN		(1 << 0)
#define GICH_HCR_UIE		(1 << 1)

#define GICH_LR_VIRTUALID		(0x3ff << 0)
#define GICH_LR_PHYSID_CPUID_SHIFT	(10)
#define GICH_LR_PHYSID_CPUID		(7 << GICH_LR_PHYSID_CPUID_SHIFT)
#define GICH_LR_STATE			(3 << 28)
#define GICH_LR_PENDING_BIT		(1 << 28)
#define GICH_LR_ACTIVE_BIT		(1 << 29)
#define GICH_LR_EOI			(1 << 19)

#define GICH_VMCR_CTRL_SHIFT		0
#define GICH_VMCR_CTRL_MASK		(0x21f << GICH_VMCR_CTRL_SHIFT)
#define GICH_VMCR_PRIMASK_SHIFT		27
#define GICH_VMCR_PRIMASK_MASK		(0x1f << GICH_VMCR_PRIMASK_SHIFT)
#define GICH_VMCR_BINPOINT_SHIFT		21
#define GICH_VMCR_BINPOINT_MASK		(0x7 << GICH_VMCR_BINPOINT_SHIFT)
#define GICH_VMCR_ALIAS_BINPOINT_SHIFT	18
#define GICH_VMCR_ALIAS_BINPOINT_MASK	(0x7 << GICH_VMCR_ALIAS_BINPOINT_SHIFT)

#define GICH_MISR_EOI			(1 << 0)
#define GICH_MISR_U			(1 << 1)


#ifdef CONFIG_USE_IRQ
int interrupt_init(void)
{
	unsigned long cpsr;

	/*
	 * setup up stacks if necessary
	 */
	IRQ_STACK_START = gd->irq_sp - 4;
	IRQ_STACK_START_IN = gd->irq_sp + 8;
	FIQ_STACK_START = IRQ_STACK_START - CONFIG_STACKSIZE_IRQ;


	__asm__ __volatile__("mrs %0, cpsr\n"
			     : "=r" (cpsr)
			     :
			     : "memory");

	__asm__ __volatile__("msr cpsr_c, %0\n"
			     "mov sp, %1\n"
			     :
			     : "r" (IRQ_MODE | I_BIT | F_BIT |
				    (cpsr & ~FIQ_MODE)),
			       "r" (IRQ_STACK_START)
			     : "memory");

	__asm__ __volatile__("msr cpsr_c, %0\n"
			     "mov sp, %1\n"
			     :
			     : "r" (FIQ_MODE | I_BIT | F_BIT |
				    (cpsr & ~IRQ_MODE)),
			       "r" (FIQ_STACK_START)
			     : "memory");

	__asm__ __volatile__("msr cpsr_c, %0"
			     :
			     : "r" (cpsr)
			     : "memory");

	return 0;
}

/* enable IRQ interrupts */
void enable_interrupts(void)
{
	unsigned long temp;

	__asm__ __volatile__("mrs %0, cpsr\n"
			     "bic %0, %0, #0x80\n"
			     "msr cpsr_c, %0"
			     : "=r" (temp)
			     :
			     : "memory");
}


/*
 * disable IRQ/FIQ interrupts
 * returns true if interrupts had been enabled before we disabled them
 */
int disable_interrupts(void)
{
	unsigned long old, temp;

	__asm__ __volatile__("mrs %0, cpsr\n"
			     "orr %1, %0, #0xc0\n"
			     "msr cpsr_c, %1"
			     : "=r" (old), "=r" (temp)
			     :
			     : "memory");
	return (old & 0x80) == 0;
}
#else
int interrupt_init(void)
{
	/*
	 * setup up stacks if necessary
	 */
	IRQ_STACK_START_IN = gd->irq_sp + 8;

	return 0;
}

void enable_interrupts(void)
{
	return;
}
int disable_interrupts(void)
{
	return 0;
}
#endif


void bad_mode (void)
{
	panic ("Resetting CPU ...\n");
	reset_cpu(0);
}

static void show_efi_loaded_images(struct pt_regs *regs)
{
	efi_print_image_infos((void *)instruction_pointer(regs));
}

static void dump_instr(struct pt_regs *regs)
{
	unsigned long addr = instruction_pointer(regs);
	const int thumb = thumb_mode(regs);
	const int width = thumb ? 4 : 8;
	int i;

	if (thumb)
		addr &= ~1L;
	else
		addr &= ~3L;
	printf("Code: ");
	for (i = -4; i < 1 + !!thumb; i++) {
		unsigned int val;

		if (thumb)
			val = ((u16 *)addr)[i];
		else
			val = ((u32 *)addr)[i];
		printf(i == 0 ? "(%0*x) " : "%0*x ", width, val);
	}
	printf("\n");
}

void show_regs (struct pt_regs *regs)
{
	unsigned long __maybe_unused flags;
	const char __maybe_unused *processor_modes[] = {
	"USER_26",	"FIQ_26",	"IRQ_26",	"SVC_26",
	"UK4_26",	"UK5_26",	"UK6_26",	"UK7_26",
	"UK8_26",	"UK9_26",	"UK10_26",	"UK11_26",
	"UK12_26",	"UK13_26",	"UK14_26",	"UK15_26",
	"USER_32",	"FIQ_32",	"IRQ_32",	"SVC_32",
	"UK4_32",	"UK5_32",	"UK6_32",	"ABT_32",
	"UK8_32",	"UK9_32",	"HYP_32",	"UND_32",
	"UK12_32",	"UK13_32",	"UK14_32",	"SYS_32",
	};

	flags = condition_codes (regs);

	printf("pc : [<%08lx>]	   lr : [<%08lx>]\n",
	       instruction_pointer(regs), regs->ARM_lr);
	if (gd->flags & GD_FLG_RELOC) {
		printf("reloc pc : [<%08lx>]	   lr : [<%08lx>]\n",
		       instruction_pointer(regs) - gd->reloc_off,
		       regs->ARM_lr - gd->reloc_off);
	}
	printf("sp : %08lx  ip : %08lx	 fp : %08lx\n",
	       regs->ARM_sp, regs->ARM_ip, regs->ARM_fp);
	printf ("r10: %08lx  r9 : %08lx	 r8 : %08lx\n",
		regs->ARM_r10, regs->ARM_r9, regs->ARM_r8);
	printf ("r7 : %08lx  r6 : %08lx	 r5 : %08lx  r4 : %08lx\n",
		regs->ARM_r7, regs->ARM_r6, regs->ARM_r5, regs->ARM_r4);
	printf ("r3 : %08lx  r2 : %08lx	 r1 : %08lx  r0 : %08lx\n",
		regs->ARM_r3, regs->ARM_r2, regs->ARM_r1, regs->ARM_r0);
	printf ("Flags: %c%c%c%c",
		flags & CC_N_BIT ? 'N' : 'n',
		flags & CC_Z_BIT ? 'Z' : 'z',
		flags & CC_C_BIT ? 'C' : 'c', flags & CC_V_BIT ? 'V' : 'v');
	printf ("  IRQs %s  FIQs %s  Mode %s%s\n",
		interrupts_enabled (regs) ? "on" : "off",
		fast_interrupts_enabled (regs) ? "on" : "off",
		processor_modes[processor_mode (regs)],
		thumb_mode (regs) ? " (T)" : "");
	dump_instr(regs);
}

/* fixup PC to point to the instruction leading to the exception */
static inline void fixup_pc(struct pt_regs *regs, int offset)
{
	uint32_t pc = instruction_pointer(regs) + offset;
	regs->ARM_pc = pc | (regs->ARM_pc & PCMASK);
}

void do_undefined_instruction (struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf ("undefined instruction\n");
	fixup_pc(pt_regs, -4);
	show_regs (pt_regs);
	show_efi_loaded_images(pt_regs);
	bad_mode ();
}

void do_software_interrupt (struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf ("software interrupt\n");
	fixup_pc(pt_regs, -4);
	show_regs (pt_regs);
	show_efi_loaded_images(pt_regs);
	bad_mode ();
}

void do_prefetch_abort (struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf ("prefetch abort\n");
	fixup_pc(pt_regs, -8);
	show_regs (pt_regs);
	show_efi_loaded_images(pt_regs);
	bad_mode ();
}

void do_data_abort (struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf ("data abort\n");
	fixup_pc(pt_regs, -8);
	show_regs (pt_regs);
	show_efi_loaded_images(pt_regs);
	bad_mode ();
}

void do_not_used (struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf ("not used\n");
	fixup_pc(pt_regs, -8);
	show_regs (pt_regs);
	show_efi_loaded_images(pt_regs);
	bad_mode ();
}

void do_fiq (struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf ("fast interrupt request\n");
	fixup_pc(pt_regs, -8);
	show_regs (pt_regs);
	show_efi_loaded_images(pt_regs);
	bad_mode ();
}

#ifndef CONFIG_USE_IRQ
void do_irq (struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf ("interrupt request\n");
	fixup_pc(pt_regs, -8);
	show_regs (pt_regs);
	show_efi_loaded_images(pt_regs);
	bad_mode ();
}
#endif

u32 gic_ack_int(void)
{
	u32 ack = *((volatile unsigned int *)((void __iomem *)GICC_BASE +
				GIC_CPU_INTACK));
	ack &= GICC_IAR_MASK;

	return ack;
}

static inline void gic_end_int(u32 ack)
{
	*((volatile unsigned int *)((void __iomem *)GICC_BASE +
				GIC_CPU_EOI)) = (ack & GICC_IAR_MASK);
}

void *g_gic_irq_cb[1024] = {NULL};

void gic_irq_register(int irq_num, void (*irq_handle)(int, int))
{
	g_gic_irq_cb[irq_num] = (void *)irq_handle;
}

void gic_mask_irq(unsigned long hw_irq)
{
	u32 mask = 1 << (hw_irq % 32);
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_ENABLE_CLEAR + (hw_irq / 32) * 4)) = mask;
}

void gic_unmask_irq(unsigned long hw_irq)
{
	u32 mask = 1 << (hw_irq % 32);
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_ENABLE_SET + (hw_irq / 32) * 4)) = mask;
}

void gic_active_irq(unsigned long hw_irq)
{
	u32 mask = 1 << (hw_irq % 32);
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_ACTIVE_SET + (hw_irq / 32) * 4)) = mask;
}

u32 gic_get_pending(unsigned long hw_irq)
{
	u32 pending = *((volatile unsigned int *)
			((void __iomem *)GICD_BASE + GIC_DIST_PENDING_SET +
			 (hw_irq / 32) * 4));

	return pending;
}

void gic_set_target(u32 core_mask, unsigned long hw_irq)
{
	u32 val = core_mask << ((hw_irq % 4) * 8);
	u32 mask = 0xff << ((hw_irq % 4) * 8);
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_TARGET + (hw_irq / 4) * 4)) &= ~mask;
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_TARGET + (hw_irq / 4) * 4)) |= val;
}

void gic_set_type(unsigned long hw_irq)
{
	u32 confmask = 0x2 << ((hw_irq % 16) * 2);

	gic_mask_irq(hw_irq);
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_CONFIG + (hw_irq / 16) * 4)) |= confmask;
	gic_unmask_irq(hw_irq);
}

void gic_set_pri_per_cpu(void)
{
	int i;

	for (i = 0; i < 32; i += 4)
		*((volatile unsigned int *)((void __iomem *)GICD_BASE +
					GIC_DIST_PRI + (i / 4) * 4)) = 0x80808080;

	*((volatile unsigned int *)((void __iomem *)GICC_BASE +
				GIC_CPU_PRIMASK)) = 0xf0;
}

void gic_enable_dist(void)
{
	/* set the SGI interrupts for this core to group 1 */
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_IGROUP)) = 0xffffffff;
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_CTRL)) = GICD_ENABLE;
	u32 bypass = *((volatile unsigned int *)
			((void __iomem *)GICC_BASE + GIC_CPU_CTRL));
	bypass &= GICC_DIS_BYPASS_MASK;
	bypass |= GICC_ENABLE;
	*((volatile unsigned int *)((void __iomem *)GICC_BASE +
				GIC_CPU_CTRL)) = bypass;
}

void gic_set_pri_common(void)
{
	int i;

	for (i = 32; i < 256; i += 4)
		*((volatile unsigned int *)((void __iomem *)GICD_BASE +
					GIC_DIST_PRI + (i / 4) * 4)) = 0x70707070;
}

void gic_set_pri_irq(u32 hw_irq, u8 pri)
{
	u32 val = pri << ((hw_irq % 4) * 8);
	u32 mask = 0xff << ((hw_irq % 4) * 8);
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_PRI + (hw_irq / 4) * 4)) &= ~mask;
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_PRI + (hw_irq / 4) * 4)) |= val;
}

void gic_set_sgi(int core_mask, u32 hw_irq)
{
	u32 val;
	if (hw_irq > 16) {
		printf("Interrupt id num: %d is not valid, SGI[0 - 15]\n",
				hw_irq);

		return;
   }

	val = (core_mask << 16) | hw_irq;
	/* group 1 */
	val |= 0x8000;
	*((volatile unsigned int *)((void __iomem *)GICD_BASE +
				GIC_DIST_SOFTINT)) = val;

	return;
}

void do_irq(struct pt_regs *pt_regs)
{
	u32 ack;
	int hw_irq;
	int src_coreid; /* just for SGI, will be 0 for other */
	void (*irq_handle)(int, int);

READ_ACK:
	ack = gic_ack_int();
	hw_irq = ack & GICC_IAR_INT_ID_MASK;
	src_coreid = ack >> 10;

	if (hw_irq  >= 1021)
		return;

	irq_handle = (void (*)(int, int))g_gic_irq_cb[hw_irq];
	if (irq_handle)
		irq_handle(hw_irq, src_coreid);


	gic_end_int(ack);

	goto READ_ACK;
}
