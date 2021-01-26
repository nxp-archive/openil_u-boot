/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2017-2019 NXP
 */

#ifndef __IMX8M_REGS_H_
#define __IMX8M_REGS_H_

#define ARCH_MXC

#ifdef CONFIG_IMX8MQ
#include <asm/arch/imx-regs-imx8mq.h>
#elif defined(CONFIG_IMX8MM) || defined(CONFIG_IMX8MN) || defined(CONFIG_IMX8MP)
#include <asm/arch/imx-regs-imx8mm.h>

/* A53 Reset Control Register (SRC_A53RCR0) */
#define SRC_A53RCR0_CORE_1_RESET_OFFSET     5
#define SRC_A53RCR0_CORE_1_RESET_MASK       (1<<SRC_A53RCR0_CORE_1_RESET_OFFSET)
#define SRC_A53RCR0_CORE_2_RESET_OFFSET     6
#define SRC_A53RCR0_CORE_2_RESET_MASK       (1<<SRC_A53RCR0_CORE_2_RESET_OFFSET)
#define SRC_A53RCR0_CORE_3_RESET_OFFSET     7
#define SRC_A53RCR0_CORE_3_RESET_MASK       (1<<SRC_A53RCR0_CORE_3_RESET_OFFSET)
/* A53 Reset Control Register (SRC_A53RCR1) */
#define SRC_A53RCR1_CORE_1_ENABLE_OFFSET    1
#define SRC_A53RCR1_CORE_1_ENABLE_MASK      (1<<SRC_A53RCR1_CORE_1_ENABLE_OFFSET)
#define SRC_A53RCR1_CORE_2_ENABLE_OFFSET    2
#define SRC_A53RCR1_CORE_2_ENABLE_MASK      (1<<SRC_A53RCR1_CORE_2_ENABLE_OFFSET)
#define SRC_A53RCR1_CORE_3_ENABLE_OFFSET    3
#define SRC_A53RCR1_CORE_3_ENABLE_MASK      (1<<SRC_A53RCR1_CORE_3_ENABLE_OFFSET)
#else
#error "Error no imx-regs.h"
#endif

#endif
