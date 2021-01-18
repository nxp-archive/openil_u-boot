// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2021 NXP
 *
 */

#ifndef _FSL_LAYERSCAPE_TEST_H
#define _FSL_LAYERSCAPE_TEST_H

#ifndef __ASSEMBLY__

int test_gpio(void);
void test_i2c(void);
void test_irq_init(void);
void test_icc_func_init(void);
int test_qspi(void);
int test_net(void);
void test_usb(void);
void test_pcie(void);
void test_flexcan(void);
#endif
#endif /* _FSL_LAYERSCAPE_TEST_H */
