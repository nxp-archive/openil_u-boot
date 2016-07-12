/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */

/** @file
 *  Contains all the defines to handle parsing and loading of PE firmware files.
 */

#ifndef __PFE_FIRMWARE_H__
#define __PFE_FIRMWARE_H__


#define CLASS_FIRMWARE_FILENAME		"class_sbl_elf.fw"
#define TMU_FIRMWARE_FILENAME		   "tmu_sbl_elf.fw"
#define UTIL_FIRMWARE_FILENAME		"util_sbl_elf.fw"


int pfe_firmware_init(u8 *clasS_fw_loc, u8 *tmu_fw_loc, u8 *util_fw_loc);
void pfe_firmware_exit(void);


#endif
