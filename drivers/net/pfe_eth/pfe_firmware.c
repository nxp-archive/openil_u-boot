/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */

/** @file
 *  Contains all the functions to handle parsing and loading of PE firmware
 * files.
 */

#include "hal.h"
#include "pfe_firmware.h"
#include "pfe/pfe.h"


/* CLASS-PE ELF file content */
unsigned char class_fw_data[] __attribute__((aligned(sizeof(int)))) = {
	#include CLASS_FIRMWARE_FILENAME
};

/* TMU-PE ELF file content */
unsigned char tmu_fw_data[] __attribute__((aligned(sizeof(int)))) = {
	#include TMU_FIRMWARE_FILENAME
};

#if !defined(CONFIG_UTIL_PE_DISABLED)
unsigned char util_fw_data[] = {
	#include UTIL_FIRMWARE_FILENAME
};
#endif

/** PFE elf firmware loader.
 * Loads an elf firmware image into a list of PE's (specified using a bitmask)
 *
 * @param pe_mask	Mask of PE id's to load firmware to
 * @param fw		Pointer to the firmware image
 *
 * @return		0 on sucess, a negative value on error
 *
 */
int pfe_load_elf(int pe_mask, const struct firmware *fw)
{
	Elf32_Ehdr *elf_hdr = (Elf32_Ehdr *)fw->data;
	Elf32_Half sections = be16_to_cpu(elf_hdr->e_shnum);
	Elf32_Shdr *shdr = (Elf32_Shdr *) (fw->data +
						be32_to_cpu(elf_hdr->e_shoff));
	int id, section;
	int rc;

	printf("%s: no of sections: %d\n", __func__, sections);

	/* Some sanity checks */
	if (strncmp((char *)&elf_hdr->e_ident[EI_MAG0], ELFMAG, SELFMAG)) {
		printf("%s: incorrect elf magic number\n", __func__);
		return -1;
	}

	if (elf_hdr->e_ident[EI_CLASS] != ELFCLASS32) {
		printf("%s: incorrect elf class(%x)\n", __func__,
			elf_hdr->e_ident[EI_CLASS]);
		return -1;
	}

	if (elf_hdr->e_ident[EI_DATA] != ELFDATA2MSB) {
		printf("%s: incorrect elf data(%x)\n", __func__,
			elf_hdr->e_ident[EI_DATA]);
		return -1;
	}

	if (be16_to_cpu(elf_hdr->e_type) != ET_EXEC) {
		printf("%s: incorrect elf file type(%x)\n", __func__,
			be16_to_cpu(elf_hdr->e_type));
		return -1;
	}

	for (section = 0; section < sections; section++, shdr++) {
		if (!(be32_to_cpu(shdr->sh_flags) & (SHF_WRITE | SHF_ALLOC |
			SHF_EXECINSTR)))
			continue;
		for (id = 0; id < MAX_PE; id++)
			if (pe_mask & (1 << id)) {
				rc = pe_load_elf_section(id, fw->data, shdr);
				if (rc < 0)
					goto err;
			}
	}

	return 0;

err:
	return rc;
}

/** PFE firmware initialization.
 * Loads different firmware files from filesystem.
 * Initializes PE IMEM/DMEM and UTIL-PE DDR
 * Initializes control path symbol addresses (by looking them up in the elf
 * firmware files
 * Takes PE's out of reset
 *
 * @return	0 on sucess, a negative value on error
 *
 */
int pfe_firmware_init(u8 *class_fw_loc, u8 *tmu_fw_loc, u8 *util_fw_loc)
{
	struct firmware class_fw, tmu_fw;
#if !defined(CONFIG_UTIL_PE_DISABLED)
	struct firmware util_fw;
#endif
	int rc = 0;

	printf("%s\n", __func__);
#if 0
	/* This testing purpose only */
	printf("Copying default fw\n");
	memcpy(class_fw_loc, class_fw_data, sizeof(class_fw_data));
	memcpy(tmu_fw_loc, tmu_fw_data, sizeof(tmu_fw_data));
	memcpy(util_fw_loc, util_fw_data, sizeof(util_fw_data));
#endif

	if (class_fw_loc)
		class_fw.data = class_fw_loc;
	else
		class_fw.data = class_fw_data;

	if (tmu_fw_loc)
		tmu_fw.data = tmu_fw_loc;
	else
		tmu_fw.data = tmu_fw_data;

#if !defined(CONFIG_UTIL_PE_DISABLED)
	if (util_fw_loc)
		util_fw.data = util_fw_loc;
	else
		util_fw.data = util_fw_data;
#endif

	rc = pfe_load_elf(CLASS_MASK, &class_fw);
	if (rc < 0) {
		printf("%s: class firmware load failed\n", __func__);
		goto err3;
	}

	printf("%s: class firmware loaded\n", __func__);

	rc = pfe_load_elf(TMU_MASK, &tmu_fw);
	if (rc < 0) {
		printf("%s: tmu firmware load failed\n", __func__);
		goto err3;
	}

	printf("%s: tmu firmware loaded\n", __func__);

#if !defined(CONFIG_UTIL_PE_DISABLED)
	rc = pfe_load_elf(UTIL_MASK, &util_fw);
	if (rc < 0) {
		printf("%s: util firmware load failed\n", __func__);
		goto err3;
	}

	printf("%s: util firmware loaded\n", __func__);

	util_enable();
#endif

#if defined(CONFIG_LS1012A)
	tmu_enable(0xb);
#else
	tmu_enable(0xf);
#endif
	class_enable();

	gpi_enable(HGPI_BASE_ADDR);


err3:
	return rc;
}

/** PFE firmware cleanup
 * Puts PE's in reset
 *
 *
 */
void pfe_firmware_exit(void)
{
	printf("%s\n", __func__);

	class_disable();
	tmu_disable(0xf);
#if !defined(CONFIG_UTIL_PE_DISABLED)
	util_disable();
#endif
	hif_tx_disable();
	hif_rx_disable();
}
