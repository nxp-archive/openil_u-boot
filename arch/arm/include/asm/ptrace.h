#ifndef __ASM_ARM_PTRACE_H
#define __ASM_ARM_PTRACE_H

#define PTRACE_GETREGS		12
#define PTRACE_SETREGS		13
#define PTRACE_GETFPREGS	14
#define PTRACE_SETFPREGS	15

#define PTRACE_SETOPTIONS	21

/* options set using PTRACE_SETOPTIONS */
#define PTRACE_O_TRACESYSGOOD	0x00000001

#define DEFINE(sym, val) \
        asm volatile("\n->" #sym " %0 " #val : : "i" (val))

#define S_FRAME_SIZE (36 * 8)
#define S_LR (30 * 8)

#define S_SP (32 * 8)

#define S_PC (33 * 8)

#include <asm/proc-armv/ptrace.h>

#ifndef __ASSEMBLY__
#define pc_pointer(v) \
	((v) & ~PCMASK)

#define instruction_pointer(regs) \
	(pc_pointer((regs)->ARM_pc))

#ifdef __KERNEL__
extern void show_regs(struct pt_regs *);

#define predicate(x)	(x & 0xf0000000)
#define PREDICATE_ALWAYS	0xe0000000

#endif

#endif /* __ASSEMBLY__ */

#endif
