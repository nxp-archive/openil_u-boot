/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#include "hal.h"
#include "pfe/pfe.h"

void *cbus_base_addr;
void *ddr_base_addr;
unsigned long ddr_phys_base_addr;
#if 0
#define dprintf(fmt, arg...) printf(fmt, ##arg)
#else
#define dprintf(fmt, arg...)
#endif
static struct pe_info pe[MAX_PE];

/** Initializes the PFE library.
 * Must be called before using any of the library functions.
 *
 * @param[in] cbus_base		CBUS virtual base address (as mapped in
 * the host CPU address space)
 * @param[in] ddr_base		DDR virtual base address (as mapped in
 * the host CPU address space)
 * @param[in] ddr_phys_base	DDR physical base address (as mapped in
 * platform)
 */
void pfe_lib_init(void *cbus_base, void *ddr_base, unsigned long ddr_phys_base)
{
	cbus_base_addr = cbus_base;
	ddr_base_addr = ddr_base;
	ddr_phys_base_addr = ddr_phys_base;

	pe[CLASS0_ID].dmem_base_addr = (u32)CLASS_DMEM_BASE_ADDR(0);
	pe[CLASS0_ID].pmem_base_addr = (u32)CLASS_IMEM_BASE_ADDR(0);
	pe[CLASS0_ID].pmem_size = (u32)CLASS_IMEM_SIZE;
	pe[CLASS0_ID].mem_access_wdata = (void *)CLASS_MEM_ACCESS_WDATA;
	pe[CLASS0_ID].mem_access_addr = (void *)CLASS_MEM_ACCESS_ADDR;
	pe[CLASS0_ID].mem_access_rdata = (void *)CLASS_MEM_ACCESS_RDATA;

	pe[CLASS1_ID].dmem_base_addr = (u32)CLASS_DMEM_BASE_ADDR(1);
	pe[CLASS1_ID].pmem_base_addr = (u32)CLASS_IMEM_BASE_ADDR(1);
	pe[CLASS1_ID].pmem_size = (u32)CLASS_IMEM_SIZE;
	pe[CLASS1_ID].mem_access_wdata = (void *)CLASS_MEM_ACCESS_WDATA;
	pe[CLASS1_ID].mem_access_addr = (void *)CLASS_MEM_ACCESS_ADDR;
	pe[CLASS1_ID].mem_access_rdata = (void *)CLASS_MEM_ACCESS_RDATA;

	pe[CLASS2_ID].dmem_base_addr = (u32)CLASS_DMEM_BASE_ADDR(2);
	pe[CLASS2_ID].pmem_base_addr = (u32)CLASS_IMEM_BASE_ADDR(2);
	pe[CLASS2_ID].pmem_size = (u32)CLASS_IMEM_SIZE;
	pe[CLASS2_ID].mem_access_wdata = (void *)CLASS_MEM_ACCESS_WDATA;
	pe[CLASS2_ID].mem_access_addr = (void *)CLASS_MEM_ACCESS_ADDR;
	pe[CLASS2_ID].mem_access_rdata = (void *)CLASS_MEM_ACCESS_RDATA;

	pe[CLASS3_ID].dmem_base_addr = (u32)CLASS_DMEM_BASE_ADDR(3);
	pe[CLASS3_ID].pmem_base_addr = (u32)CLASS_IMEM_BASE_ADDR(3);
	pe[CLASS3_ID].pmem_size = (u32)CLASS_IMEM_SIZE;
	pe[CLASS3_ID].mem_access_wdata = (void *)CLASS_MEM_ACCESS_WDATA;
	pe[CLASS3_ID].mem_access_addr = (void *)CLASS_MEM_ACCESS_ADDR;
	pe[CLASS3_ID].mem_access_rdata = (void *)CLASS_MEM_ACCESS_RDATA;

#if !defined(CONFIG_PLATFORM_PCI)
	pe[CLASS4_ID].dmem_base_addr = (u32)CLASS_DMEM_BASE_ADDR(4);
	pe[CLASS4_ID].pmem_base_addr = (u32)CLASS_IMEM_BASE_ADDR(4);
	pe[CLASS4_ID].pmem_size = (u32)CLASS_IMEM_SIZE;
	pe[CLASS4_ID].mem_access_wdata = (void *)CLASS_MEM_ACCESS_WDATA;
	pe[CLASS4_ID].mem_access_addr = (void *)CLASS_MEM_ACCESS_ADDR;
	pe[CLASS4_ID].mem_access_rdata = (void *)CLASS_MEM_ACCESS_RDATA;

	pe[CLASS5_ID].dmem_base_addr = (u32)CLASS_DMEM_BASE_ADDR(5);
	pe[CLASS5_ID].pmem_base_addr = (u32)CLASS_IMEM_BASE_ADDR(5);
	pe[CLASS5_ID].pmem_size = (u32)CLASS_IMEM_SIZE;
	pe[CLASS5_ID].mem_access_wdata = (void *)CLASS_MEM_ACCESS_WDATA;
	pe[CLASS5_ID].mem_access_addr = (void *)CLASS_MEM_ACCESS_ADDR;
	pe[CLASS5_ID].mem_access_rdata = (void *)CLASS_MEM_ACCESS_RDATA;
#endif
	pe[TMU0_ID].dmem_base_addr = (u32)TMU_DMEM_BASE_ADDR(0);
	pe[TMU0_ID].pmem_base_addr = (u32)TMU_IMEM_BASE_ADDR(0);
	pe[TMU0_ID].pmem_size = (u32)TMU_IMEM_SIZE;
	pe[TMU0_ID].mem_access_wdata = (void *)TMU_MEM_ACCESS_WDATA;
	pe[TMU0_ID].mem_access_addr = (void *)TMU_MEM_ACCESS_ADDR;
	pe[TMU0_ID].mem_access_rdata = (void *)TMU_MEM_ACCESS_RDATA;

#if !defined(CONFIG_TMU_DUMMY)
	pe[TMU1_ID].dmem_base_addr = (u32)TMU_DMEM_BASE_ADDR(1);
	pe[TMU1_ID].pmem_base_addr = (u32)TMU_IMEM_BASE_ADDR(1);
	pe[TMU1_ID].pmem_size = (u32)TMU_IMEM_SIZE;
	pe[TMU1_ID].mem_access_wdata = (void *)TMU_MEM_ACCESS_WDATA;
	pe[TMU1_ID].mem_access_addr = (void *)TMU_MEM_ACCESS_ADDR;
	pe[TMU1_ID].mem_access_rdata = (void *)TMU_MEM_ACCESS_RDATA;

#if !defined(CONFIG_LS1012A)
	pe[TMU2_ID].dmem_base_addr = (u32)TMU_DMEM_BASE_ADDR(2);
	pe[TMU2_ID].pmem_base_addr = (u32)TMU_IMEM_BASE_ADDR(2);
	pe[TMU2_ID].pmem_size = (u32)TMU_IMEM_SIZE;
	pe[TMU2_ID].mem_access_wdata = (void *)TMU_MEM_ACCESS_WDATA;
	pe[TMU2_ID].mem_access_addr = (void *)TMU_MEM_ACCESS_ADDR;
	pe[TMU2_ID].mem_access_rdata = (void *)TMU_MEM_ACCESS_RDATA;
#endif

	pe[TMU3_ID].dmem_base_addr = (u32)TMU_DMEM_BASE_ADDR(3);
	pe[TMU3_ID].pmem_base_addr = (u32)TMU_IMEM_BASE_ADDR(3);
	pe[TMU3_ID].pmem_size = (u32)TMU_IMEM_SIZE;
	pe[TMU3_ID].mem_access_wdata = (void *)TMU_MEM_ACCESS_WDATA;
	pe[TMU3_ID].mem_access_addr = (void *)TMU_MEM_ACCESS_ADDR;
	pe[TMU3_ID].mem_access_rdata = (void *)TMU_MEM_ACCESS_RDATA;
#endif

#if !defined(CONFIG_UTIL_PE_DISABLED)
	pe[UTIL_ID].dmem_base_addr = (u32)UTIL_DMEM_BASE_ADDR;
	pe[UTIL_ID].mem_access_wdata = (void *)UTIL_MEM_ACCESS_WDATA;
	pe[UTIL_ID].mem_access_addr = (void *)UTIL_MEM_ACCESS_ADDR;
	pe[UTIL_ID].mem_access_rdata = (void *)UTIL_MEM_ACCESS_RDATA;
#endif
}


/** Writes a buffer to PE internal memory from the host
 * through indirect access registers.
 *
 * @param[in] id	       PE identification (CLASS0_ID, ..., TMU0_ID,
 * ..., UTIL_ID)
 * @param[in] src		Buffer source address
 * @param[in] mem_access_addr	DMEM destination address (must be 32bit
 * aligned)
 * @param[in] len		Number of bytes to copy
 */
void pe_mem_memcpy_to32(int id, u32 mem_access_addr, const void *src, unsigned
				int len)
{
	u32 offset = 0, val, addr;
	unsigned int len32 = len >> 2;
	int i;

	addr = mem_access_addr | PE_MEM_ACCESS_WRITE |
		PE_MEM_ACCESS_BYTE_ENABLE(0, 4);

	for (i = 0; i < len32; i++, offset += 4, src += 4) {
		val = *(u32 *)src;
		writel(cpu_to_be32(val), pe[id].mem_access_wdata);
		writel(addr + offset, pe[id].mem_access_addr);
	}

	if ((len = (len & 0x3))) {
		val = 0;

		addr = (mem_access_addr | PE_MEM_ACCESS_WRITE |
			PE_MEM_ACCESS_BYTE_ENABLE(0, len)) + offset;

		for (i = 0; i < len; i++, src++)
			val |= (*(u8 *)src) << (8 * i);

		writel(cpu_to_be32(val), pe[id].mem_access_wdata);
		writel(addr, pe[id].mem_access_addr);
	}
}

/** Writes a buffer to PE internal data memory (DMEM) from the host
 * through indirect access registers.
 * @param[in] id		PE identification (CLASS0_ID, ..., TMU0_ID,
 * ..., UTIL_ID)
 * @param[in] src		Buffer source address
 * @param[in] dst		DMEM destination address (must be 32bit
 * aligned)
 * @param[in] len		Number of bytes to copy
 */
void pe_dmem_memcpy_to32(int id, u32 dst, const void *src, unsigned int len)
{
	pe_mem_memcpy_to32(id, pe[id].dmem_base_addr | dst |
				PE_MEM_ACCESS_DMEM, src, len);
}


/** Writes a buffer to PE internal program memory (PMEM) from the host
 * through indirect access registers.
 * @param[in] id		PE identification (CLASS0_ID, ..., TMU0_ID,
 * ..., TMU3_ID)
 * @param[in] src		Buffer source address
 * @param[in] dst		PMEM destination address (must be 32bit
 * aligned)
 * @param[in] len		Number of bytes to copy
 */
void pe_pmem_memcpy_to32(int id, u32 dst, const void *src, unsigned int len)
{
	pe_mem_memcpy_to32(id, pe[id].pmem_base_addr | (dst & (pe[id].pmem_size
				- 1)) | PE_MEM_ACCESS_IMEM, src, len);
}


/** Reads PE internal program memory (IMEM) from the host
 * through indirect access registers.
 * @param[in] id		PE identification (CLASS0_ID, ..., TMU0_ID,
 * ..., TMU3_ID)
 * @param[in] addr		PMEM read address (must be aligned on size)
 * @param[in] size		Number of bytes to read (maximum 4, must not
 * cross 32bit boundaries)
 * @return			the data read (in PE endianess, i.e BE).
 */
u32 pe_pmem_read(int id, u32 addr, u8 size)
{
	u32 offset = addr & 0x3;
	u32 mask = 0xffffffff >> ((4 - size) << 3);
	u32 val;

	addr = pe[id].pmem_base_addr | ((addr & ~0x3) & (pe[id].pmem_size - 1))
		| PE_MEM_ACCESS_READ | PE_MEM_ACCESS_IMEM |
		PE_MEM_ACCESS_BYTE_ENABLE(offset, size);

	writel(addr, pe[id].mem_access_addr);
	val = be32_to_cpu(readl(pe[id].mem_access_rdata));

	return (val >> (offset << 3)) & mask;
}


/** Writes PE internal data memory (DMEM) from the host
 * through indirect access registers.
 * @param[in] id		PE identification (CLASS0_ID, ..., TMU0_ID,
 * ..., UTIL_ID)
 * @param[in] addr		DMEM write address (must be aligned on size)
 * @param[in] val		Value to write (in PE endianess, i.e BE)
 * @param[in] size		Number of bytes to write (maximum 4, must not
 * cross 32bit boundaries)
 */
void pe_dmem_write(int id, u32 val, u32 addr, u8 size)
{
	u32 offset = addr & 0x3;

	addr = pe[id].dmem_base_addr | (addr & ~0x3) | PE_MEM_ACCESS_WRITE |
		PE_MEM_ACCESS_DMEM | PE_MEM_ACCESS_BYTE_ENABLE(offset, size);

	/* Indirect access interface is byte swapping data being written */
	writel(cpu_to_be32(val << (offset << 3)), pe[id].mem_access_wdata);
	writel(addr, pe[id].mem_access_addr);
}


/** Reads PE internal data memory (DMEM) from the host
 * through indirect access registers.
 * @param[in] id		PE identification (CLASS0_ID, ..., TMU0_ID,
 * ..., UTIL_ID)
 * @param[in] addr		DMEM read address (must be aligned on size)
 * @param[in] size		Number of bytes to read (maximum 4, must not
 * cross 32bit boundaries)
 * @return			the data read (in PE endianess, i.e BE).
 */
u32 pe_dmem_read(int id, u32 addr, u8 size)
{
	u32 offset = addr & 0x3;
	u32 mask = 0xffffffff >> ((4 - size) << 3);
	u32 val;

	addr = pe[id].dmem_base_addr | (addr & ~0x3) | PE_MEM_ACCESS_READ |
		PE_MEM_ACCESS_DMEM | PE_MEM_ACCESS_BYTE_ENABLE(offset, size);

	writel(addr, pe[id].mem_access_addr);

	/* Indirect access interface is byte swapping data being read */
	val = be32_to_cpu(readl(pe[id].mem_access_rdata));

	return (val >> (offset << 3)) & mask;
}

/** This function is used to write to CLASS internal bus peripherals (ccu,
 * pe-lem) from the host
 * through indirect access registers.
 * @param[in]	val	value to write
 * @param[in]	addr	Address to write to (must be aligned on size)
 * @param[in]	size	Number of bytes to write (1, 2 or 4)
 *
 */
void class_bus_write(u32 val, u32 addr, u8 size)
{
	u32 offset = addr & 0x3;

	writel((addr & CLASS_BUS_ACCESS_BASE_MASK), CLASS_BUS_ACCESS_BASE);

	addr = (addr & ~CLASS_BUS_ACCESS_BASE_MASK) | PE_MEM_ACCESS_WRITE |
		(size << 24);

	writel(cpu_to_be32(val << (offset << 3)), CLASS_BUS_ACCESS_WDATA);
	writel(addr, CLASS_BUS_ACCESS_ADDR);
}


/** Reads from CLASS internal bus peripherals (ccu, pe-lem) from the host
 * through indirect access registers.
 * @param[in] addr	Address to read from (must be aligned on size)
 * @param[in] size	Number of bytes to read (1, 2 or 4)
 * @return		the read data
 *
 */
u32 class_bus_read(u32 addr, u8 size)
{
	u32 offset = addr & 0x3;
	u32 mask = 0xffffffff >> ((4 - size) << 3);
	u32 val;

	writel((addr & CLASS_BUS_ACCESS_BASE_MASK), CLASS_BUS_ACCESS_BASE);

	addr = (addr & ~CLASS_BUS_ACCESS_BASE_MASK) | (size << 24);

	writel(addr, CLASS_BUS_ACCESS_ADDR);
	val = be32_to_cpu(readl(CLASS_BUS_ACCESS_RDATA));

	return (val >> (offset << 3)) & mask;
}

/** Writes data to the cluster memory (PE_LMEM)
 * @param[in] dst	PE LMEM destination address (must be 32bit aligned)
 * @param[in] src	Buffer source address
 * @param[in] len	Number of bytes to copy
 */
void class_pe_lmem_memcpy_to32(u32 dst, const void *src, unsigned int len)
{
	u32 len32 = len >> 2;
	int i;

	for (i = 0; i < len32; i++, src += 4, dst += 4)
		class_bus_write(*(u32 *)src, dst, 4);

	if (len & 0x2) {
		class_bus_write(*(u16 *)src, dst, 2);
		src += 2;
		dst += 2;
	}

	if (len & 0x1) {
		class_bus_write(*(u8 *)src, dst, 1);
		src++;
		dst++;
	}
}

/** Writes value to the cluster memory (PE_LMEM)
 * @param[in] dst	PE LMEM destination address (must be 32bit aligned)
 * @param[in] val	Value to write
 * @param[in] len	Number of bytes to write
 */
void class_pe_lmem_memset(u32 dst, int val, unsigned int len)
{
	u32 len32 = len >> 2;
	int i;

	val = val | (val << 8) | (val << 16) | (val << 24);

	for (i = 0; i < len32; i++, dst += 4)
		class_bus_write(val, dst, 4);

	if (len & 0x2) {
		class_bus_write(val, dst, 2);
		dst += 2;
	}

	if (len & 0x1) {
		class_bus_write(val, dst, 1);
		dst++;
	}
}

/** Reads data from the cluster memory (PE_LMEM)
 * @param[out] dst		pointer to the source buffer data are copied
 *					to
 * @param[in] len		length in bytes of the amount of data to read
 *					from cluster memory
 * @param[in] offset	offset in bytes in the cluster memory where data are
 *				read from
 */
void pe_lmem_read(u32 *dst, u32 len, u32 offset)
{
	u32 len32 = len >> 2;
	int i = 0;

	for (i = 0; i < len32; dst++, i++, offset += 4)
		*dst = class_bus_read(PE_LMEM_BASE_ADDR + offset, 4);

	/* FIXME we may have an out of bounds access on dst */
	if (len & 0x03)
		*dst = class_bus_read(PE_LMEM_BASE_ADDR + offset, (len &
					0x03));
}

/** Writes data to the cluster memory (PE_LMEM)
 * @param[in] src	pointer to the source buffer data are copied from
 * @param[in] len	length in bytes of the amount of data to write to the
 *				cluster memory
 * @param[in] offset	offset in bytes in the cluster memory where data are
 *				written to
 */
void pe_lmem_write(u32 *src, u32 len, u32 offset)
{
	u32 len32 = len >> 2;
	int i = 0;

	for (i = 0; i < len32; src++, i++, offset += 4)
		class_bus_write(*src, PE_LMEM_BASE_ADDR + offset, 4);

	/* FIXME we may have an out of bounds access on src */
	if (len & 0x03)
		class_bus_write(*src, PE_LMEM_BASE_ADDR + offset, (len &
					0x03));
}

#if !defined(CONFIG_UTIL_PE_DISABLED)
/** Writes UTIL program memory (DDR) from the host.
 *
 * @param[in] addr	Address to write (virtual, must be aligned on size)
 * @param[in] val		Value to write (in PE endianess, i.e BE)
 * @param[in] size		Number of bytes to write (2 or 4)
 */
static void util_pmem_write(u32 val, void *addr, u8 size)
{
	void *addr64 = (void *)((unsigned long)addr & ~0x7);
	unsigned long off = 8 - ((unsigned long)addr & 0x7) - size;

	/* IMEM should  be loaded as a 64bit swapped value in a 64bit aligned
	 * location
	 */
	if (size == 4)
		writel(be32_to_cpu(val), addr64 + off);
	else
		writew(be16_to_cpu((u16)val), addr64 + off);
}


/** Writes a buffer to UTIL program memory (DDR) from the host.
 *
 * @param[in] dst	Address to write (virtual, must be at least 16bit
 *					aligned)
 * @param[in] src	Buffer to write (in PE endianess, i.e BE, must have
 *				same alignment as dst)
 * @param[in] len	Number of bytes to write (must be at least 16bit
 *						aligned)
 */
static void util_pmem_memcpy(void *dst, const void *src, unsigned int len)
{
	unsigned int len32;
	int i;

	if ((unsigned long)src & 0x2) {
		util_pmem_write(*(u16 *)src, dst, 2);
		src += 2;
		dst += 2;
		len -= 2;
	}

	len32 = len >> 2;

	for (i = 0; i < len32; i++, dst += 4, src += 4)
		util_pmem_write(*(u32 *)src, dst, 4);

	if (len & 0x2)
		util_pmem_write(*(u16 *)src, dst, len & 0x2);
}
#endif

/** Loads an elf section into pmem
 * Code needs to be at least 16bit aligned and only PROGBITS sections are
 * supported
 *
 * @param[in] id	PE identification (CLASS0_ID, ..., TMU0_ID, ...,
 *					TMU3_ID)
 * @param[in] data	pointer to the elf firmware
 * @param[in] shdr	pointer to the elf section header
 *
 */
static int pe_load_pmem_section(int id, const void *data, Elf32_Shdr *shdr)
{
	u32 offset = be32_to_cpu(shdr->sh_offset);
	u32 addr = be32_to_cpu(shdr->sh_addr);
	u32 size = be32_to_cpu(shdr->sh_size);
	u32 type = be32_to_cpu(shdr->sh_type);

#if !defined(CONFIG_UTIL_PE_DISABLED)
	if (id == UTIL_ID) {
		printf("%s: unsuported pmem section for UTIL\n", __func__);
		return -1;
	}
#endif

	if (((unsigned long)(data + offset) & 0x3) != (addr & 0x3)) {
		printf(
			"%s: load address(%x) and elf file address(%lx) don't have the same alignment\n",
			__func__, addr, (unsigned long) data + offset);

		return -1;
	}

	if (addr & 0x1) {
		printf("%s: load address(%x) is not 16bit aligned\n",
			__func__, addr);
		return -1;
	}

	if (size & 0x1) {
		printf("%s: load size(%x) is not 16bit aligned\n", __func__,
				size);
		return -1;
	}

		dprintf("pmem pe%d @%x len %d\n", id, addr, size);
	switch (type) {
	case SHT_PROGBITS:
		pe_pmem_memcpy_to32(id, addr, data + offset, size);
		break;

	default:
		printf("%s: unsuported section type(%x)\n", __func__, type);
		return -1;
	}

	return 0;
}


/** Loads an elf section into dmem
 * Data needs to be at least 32bit aligned, NOBITS sections are correctly
 * initialized to 0
 *
 * @param[in] id		PE identification (CLASS0_ID, ..., TMU0_ID,
 * ..., UTIL_ID)
 * @param[in] data		pointer to the elf firmware
 * @param[in] shdr		pointer to the elf section header
 *
 */
static int pe_load_dmem_section(int id, const void *data, Elf32_Shdr *shdr)
{
	u32 offset = be32_to_cpu(shdr->sh_offset);
	u32 addr = be32_to_cpu(shdr->sh_addr);
	u32 size = be32_to_cpu(shdr->sh_size);
	u32 type = be32_to_cpu(shdr->sh_type);
	u32 size32 = size >> 2;
	int i;

	if (((unsigned long)(data + offset) & 0x3) != (addr & 0x3)) {
		printf(
			"%s: load address(%x) and elf file address(%lx) don't have the same alignment\n",
			__func__, addr, (unsigned long)data + offset);

		return -1;
	}

	if (addr & 0x3) {
		printf("%s: load address(%x) is not 32bit aligned\n",
			__func__, addr);
		return -1;
	}

	switch (type) {
	case SHT_PROGBITS:
		dprintf("dmem pe%d @%x len %d\n", id, addr, size);
		pe_dmem_memcpy_to32(id, addr, data + offset, size);
		break;

	case SHT_NOBITS:
		dprintf("dmem zero pe%d @%x len %d\n", id, addr, size);
		for (i = 0; i < size32; i++, addr += 4)
			pe_dmem_write(id, 0, addr, 4);

		if (size & 0x3)
			pe_dmem_write(id, 0, addr, size & 0x3);

		break;

	default:
		printf("%s: unsuported section type(%x)\n", __func__, type);
		return -1;
	}

	return 0;
}


/** Loads an elf section into DDR
 * Data needs to be at least 32bit aligned, NOBITS sections are correctly
 *		initialized to 0
 *
 * @param[in] id		PE identification (CLASS0_ID, ..., TMU0_ID,
 *				..., UTIL_ID)
 * @param[in] data		pointer to the elf firmware
 * @param[in] shdr		pointer to the elf section header
 *
 */
static int pe_load_ddr_section(int id, const void *data, Elf32_Shdr *shdr)
{
	u32 offset = be32_to_cpu(shdr->sh_offset);
	u32 addr = be32_to_cpu(shdr->sh_addr);
	u32 size = be32_to_cpu(shdr->sh_size);
	u32 type = be32_to_cpu(shdr->sh_type);
	u32 flags = be32_to_cpu(shdr->sh_flags);

	switch (type) {
	case SHT_PROGBITS:
		dprintf("ddr  pe%d @%x len %d\n", id, addr, size);
		if (flags & SHF_EXECINSTR) {
			if (id <= CLASS_MAX_ID) {
				/* DO the loading only once in DDR */
				if (id == CLASS0_ID) {
					dprintf(
						"%s: load address(%x) and elf file address(%lx) rcvd\n"
						, __func__, addr,
						(unsigned long)data + offset);
					if (((unsigned long)(data + offset)
						&0x3) != (addr & 0x3)) {
						printf(
							"%s: load address(%x) and elf file address(%lx) don't have the same alignment\n",
							__func__, addr,
							(unsigned long)data +
							offset);

						return -1;
					}

					if (addr & 0x1) {
						printf(
							"%s: load address(%x) is not 16bit aligned\n"
							, __func__, addr);
						return -1;
					}

					if (size & 0x1) {
						printf(
							"%s: load length(%x) is not 16bit aligned\n"
							, __func__, size);
						return -1;
					}

					memcpy((void *)DDR_PFE_TO_VIRT(addr),
						data + offset, size);
				}
			}

#if !defined(CONFIG_UTIL_PE_DISABLED)
			else if (id == UTIL_ID) {
				if (((unsigned long)(data + offset) & 0x3)
					!= (addr & 0x3)) {
					printf(
						"%s: load address(%x) and elf file address(%lx) don't have the same alignment\n",
							__func__, addr,
						(unsigned long)data + offset);

					return -1;
				}

				if (addr & 0x1) {
					printf(
						"%s: load address(%x) is not 16bit aligned\n"
						, __func__, addr);
					return -1;
				}

				if (size & 0x1) {
					printf(
						"%s: load length(%x) is not 16bit aligned\n"
						, __func__, size);
					return -1;
				}

				util_pmem_memcpy((void *)DDR_PFE_TO_VIRT(addr),
							data + offset, size);
			}
#endif
			else {
				printf(
					"%s: unsuported ddr section type(%x) for PE(%d)\n"
					, __func__, type, id);
				return -1;
			}

		} else {
			memcpy((void *)DDR_PFE_TO_VIRT(addr), data + offset,
					size);
		}

		break;

	case SHT_NOBITS:
		dprintf("ddr zero pe%d @%x len %d\n", id, addr, size);
		memset((void *)DDR_PFE_TO_VIRT(addr), 0, size);

		break;

	default:
		printf("%s: unsuported section type(%x)\n", __func__, type);
		return -1;
	}

	return 0;
}

/** Loads an elf section into pe lmem
 * Data needs to be at least 32bit aligned, NOBITS sections are correctly
 * initialized to 0
 *
 * @param[in] id		PE identification (CLASS0_ID,..., CLASS5_ID)
 * @param[in] data		pointer to the elf firmware
 * @param[in] shdr		pointer to the elf section header
 *
 */
static int pe_load_pe_lmem_section(int id, const void *data, Elf32_Shdr *shdr)
{
	u32 offset = be32_to_cpu(shdr->sh_offset);
	u32 addr = be32_to_cpu(shdr->sh_addr);
	u32 size = be32_to_cpu(shdr->sh_size);
	u32 type = be32_to_cpu(shdr->sh_type);

	if (id > CLASS_MAX_ID) {
		printf("%s: unsuported pe-lmem section type(%x) for PE(%d)\n",
			__func__, type, id);
		return -1;
	}

	if (((unsigned long)(data + offset) & 0x3) != (addr & 0x3)) {
		printf(
			"%s: load address(%x) and elf file address(%lx) don't have the same alignment\n",
			__func__, addr, (unsigned long)data + offset);

		return -1;
	}

	if (addr & 0x3) {
		printf("%s: load address(%x) is not 32bit aligned\n",
			__func__, addr);
		return -1;
	}
	dprintf("lmem  pe%d @%x len %d\n", id, addr, size);
	switch (type) {
	case SHT_PROGBITS:
		class_pe_lmem_memcpy_to32(addr, data + offset, size);
		break;

	case SHT_NOBITS:
		class_pe_lmem_memset(addr, 0, size);
		break;

	default:
		printf("%s: unsuported section type(%x)\n", __func__, type);
		return -1;
	}

	return 0;
}


/** Loads an elf section into a PE
 * For now only supports loading a section to dmem (all PE's), pmem (class and
 * tmu PE's),
 * DDDR (util PE code)
 *
 * @param[in] id		PE identification (CLASS0_ID, ..., TMU0_ID,
 * ..., UTIL_ID)
 * @param[in] data		pointer to the elf firmware
 * @param[in] shdr		pointer to the elf section header
 *
 */
int pe_load_elf_section(int id, const void *data, Elf32_Shdr *shdr)
{
	u32 addr = be32_to_cpu(shdr->sh_addr);
	u32 size = be32_to_cpu(shdr->sh_size);

	if (IS_DMEM(addr, size))
		return pe_load_dmem_section(id, data, shdr);
	else if (IS_PMEM(addr, size))
		return pe_load_pmem_section(id, data, shdr);
	else if (IS_PFE_LMEM(addr, size))
		return 0; /* FIXME */
	else if (IS_PHYS_DDR(addr, size))
		return pe_load_ddr_section(id, data, shdr);
	else if (IS_PE_LMEM(addr, size)) {
		return pe_load_pe_lmem_section(id, data, shdr);

	} else {
		printf("%s: unsuported memory range(%x)\n", __func__, addr);
		/* FIXME this should be remove after testing UTIL from 0x20000
		 */
		/* return pe_load_ddr_section(id, data, shdr);
		 * return -1;
		 */
	}

	return 0;
}

/** This function is used to write to UTIL internal bus peripherals from the
 * host
 * through indirect access registers.
 * @param[in]	val	32bits value to write
 * @param[in]	addr	Address to write to
 * @param[in]	size	Number of bytes to write
 *
 */
void util_bus_write(u32 val, u32 addr, u8 size)
{
	u32 offset = addr & 0x3;
	u32 access_addr;

	access_addr = ((addr & ~0x3) & CLASS_BUS_ACCESS_ADDR_MASK) |
			PE_MEM_ACCESS_WRITE |
			PE_MEM_ACCESS_BYTE_ENABLE(offset, size);

	/* writel((addr & CLASS_BUS_ACCESS_BASE_MASK), CLASS_BUS_ACCESS_BASE);
	 */

	writel(cpu_to_be32(val << (offset << 3)), UTIL_BUS_ACCESS_WDATA);
	writel(access_addr, UTIL_BUS_ACCESS_ADDR);
}


/** Reads from UTIL internal bus peripherals from the host
 * through indirect access registers.
 * @param[in] addr	Address to read from
 * @param[in] size	Number of bytes to read
 * @return		the read data
 *
 */
u32 util_bus_read(u32 addr, u8 size)
{
	u32 offset = addr & 0x3;
	u32 mask = 0xffffffff >> ((4 - size) << 3);
	u32 access_addr, val;

	access_addr = ((addr & ~0x3) & CLASS_BUS_ACCESS_ADDR_MASK) |
			PE_MEM_ACCESS_READ |
			PE_MEM_ACCESS_BYTE_ENABLE(offset, size);

	/* writel((addr & CLASS_BUS_ACCESS_BASE_MASK), CLASS_BUS_ACCESS_BASE);
	 */

	writel(access_addr, UTIL_BUS_ACCESS_ADDR);
	val = be32_to_cpu(readl(UTIL_BUS_ACCESS_RDATA));

	return (val >> (offset << 3)) & mask;
}



/**************************** BMU ***************************/

/** Initializes a BMU block.
 * @param[in] base	BMU block base address
 * @param[in] cfg	BMU configuration
 */
void bmu_init(void *base, BMU_CFG *cfg)
{

	bmu_disable(base);

	bmu_set_config(base, cfg);

	bmu_reset(base);
}

/** Resets a BMU block.
 * @param[in] base	BMU block base address
 */
void bmu_reset(void *base)
{
	writel(CORE_SW_RESET, base + BMU_CTRL);

	/* Wait for self clear */
	while (readl(base + BMU_CTRL) & CORE_SW_RESET)
		;
}

/** Enabled a BMU block.
 * @param[in] base	BMU block base address
 */
void bmu_enable(void *base)
{
	writel(CORE_ENABLE, base + BMU_CTRL);
}

/** Disables a BMU block.
 * @param[in] base	BMU block base address
 */
void bmu_disable(void *base)
{
	writel(CORE_DISABLE, base + BMU_CTRL);
}

/** Sets the configuration of a BMU block.
 * @param[in] base	BMU block base address
 * @param[in] cfg	BMU configuration
 */
void bmu_set_config(void *base, BMU_CFG *cfg)
{
	writel(cfg->baseaddr, base + BMU_UCAST_BASE_ADDR);
	writel(cfg->count & 0xffff, base + BMU_UCAST_CONFIG);
	writel(cfg->size & 0xffff, base + BMU_BUF_SIZE);
	/*	writel(BMU1_THRES_CNT, base + BMU_THRES); */

	/* Interrupts are never used */
	/*	writel(0x0, base + BMU_INT_SRC); */
	writel(0x0, base + BMU_INT_ENABLE);
}


#if 0 /* These are LS1012A functions */
/**************************** GEMAC ***************************/

/** GEMAC block initialization.
 * @param[in] base	GEMAC base address (GEMAC0, GEMAC1, GEMAC2)
 * @param[in] cfg	GEMAC configuration
 */
void gemac_init(void *base, void *cfg)
{
	gemac_set_config(base, cfg);
	gemac_set_bus_width(base, 64);
}

/** GEMAC set speed.
 * @param[in] base	GEMAC base address
 * @param[in] speed	GEMAC speed (10, 100 or 1000 Mbps)
 */
void gemac_set_speed(void *base, MAC_SPEED gem_speed)
{
	u32 val = readl(base + EMAC_NETWORK_CONFIG);

	val = val & ~EMAC_SPEED_MASK;

	switch (gem_speed) {
	case SPEED_10M:
			val &= (~EMAC_PCS_ENABLE);
			break;

	case SPEED_100M:
			val = val | EMAC_SPEED_100;
			val &= (~EMAC_PCS_ENABLE);
			break;

	case SPEED_1000M:
			val = val | EMAC_SPEED_1000;
			val &= (~EMAC_PCS_ENABLE);
			break;

	case SPEED_1000M_PCS:
			val = val | EMAC_SPEED_1000;
			val |= EMAC_PCS_ENABLE;
			break;

	default:
			val = val | EMAC_SPEED_100;
			val &= (~EMAC_PCS_ENABLE);
		break;
	}

	writel(val, base + EMAC_NETWORK_CONFIG);
}

/** GEMAC set duplex.
 * @param[in] base	GEMAC base address
 * @param[in] duplex	GEMAC duplex mode (Full, Half)
 */
void gemac_set_duplex(void *base, int duplex)
{
	u32 val = readl(base + EMAC_NETWORK_CONFIG);

	if (duplex == DUPLEX_HALF)
		val = (val & ~EMAC_DUPLEX_MASK) | EMAC_HALF_DUP;
	else
		val = (val & ~EMAC_DUPLEX_MASK) | EMAC_FULL_DUP;

	writel(val, base + EMAC_NETWORK_CONFIG);
}

/** GEMAC set mode.
 * @param[in] base	GEMAC base address
 * @param[in] mode	GEMAC operation mode (MII, RMII, RGMII, SGMII)
 */
void gemac_set_mode(void *base, int mode)
{
	switch (mode) {
	case GMII:
		writel((readl(base + EMAC_CONTROL) & ~EMAC_MODE_MASK) |
			EMAC_GMII_MODE_ENABLE, base + EMAC_CONTROL);
		writel(readl(base + EMAC_NETWORK_CONFIG) &
			(~EMAC_SGMII_MODE_ENABLE), base +
			EMAC_NETWORK_CONFIG);
		break;

	case RGMII:
		writel((readl(base + EMAC_CONTROL) & ~EMAC_MODE_MASK) |
			EMAC_RGMII_MODE_ENABLE, base + EMAC_CONTROL);
		writel(readl(base + EMAC_NETWORK_CONFIG) &
			(~EMAC_SGMII_MODE_ENABLE), base +
			EMAC_NETWORK_CONFIG);
		break;

	case RMII:
		writel((readl(base + EMAC_CONTROL) & ~EMAC_MODE_MASK) |
			EMAC_RMII_MODE_ENABLE, base + EMAC_CONTROL);
		writel(readl(base + EMAC_NETWORK_CONFIG) &
			(~EMAC_SGMII_MODE_ENABLE), base +
			EMAC_NETWORK_CONFIG);
		break;

	case MII:
		writel((readl(base + EMAC_CONTROL) & ~EMAC_MODE_MASK) |
			EMAC_MII_MODE_ENABLE, base + EMAC_CONTROL);
		writel(readl(base + EMAC_NETWORK_CONFIG) &
			(~EMAC_SGMII_MODE_ENABLE), base +
			EMAC_NETWORK_CONFIG);
		break;

	case SGMII:
		writel((readl(base + EMAC_CONTROL) & ~EMAC_MODE_MASK) |
			(EMAC_RMII_MODE_DISABLE |
			EMAC_RGMII_MODE_DISABLE), base + EMAC_CONTROL);
		writel(readl(base + EMAC_NETWORK_CONFIG) |
			EMAC_SGMII_MODE_ENABLE, base + EMAC_NETWORK_CONFIG);
		break;

	default:
		writel((readl(base + EMAC_CONTROL) & ~EMAC_MODE_MASK) |
			EMAC_MII_MODE_ENABLE, base + EMAC_CONTROL);
		writel(readl(base + EMAC_NETWORK_CONFIG) &
			(~EMAC_SGMII_MODE_ENABLE), base +
			EMAC_NETWORK_CONFIG);
		break;
	}
}

/** GEMAC Enable MDIO: Activate the Management interface.  This is required to
 * program the PHY
 * @param[in] base       GEMAC base address
 */
void gemac_enable_mdio(void *base)
{
	u32 data;

	data = readl(base + EMAC_NETWORK_CONTROL);
	data |= EMAC_MDIO_EN;
	writel(data, base + EMAC_NETWORK_CONTROL);
}

/** GEMAC Disable MDIO: Disable the Management interface.
 * @param[in] base       GEMAC base address
 */
void gemac_disable_mdio(void *base)
{
	u32 data;

	data = readl(base + EMAC_NETWORK_CONTROL);
	data &= ~EMAC_MDIO_EN;
	writel(data, base + EMAC_NETWORK_CONTROL);
}

/** GEMAC Set MDC clock division
 * @param[in] base       GEMAC base address
 * @param[in] base       MDC divider value
 */
void gemac_set_mdc_div(void *base, MAC_MDC_DIV gem_mdcdiv)
{
	u32 data;

	data = readl(base + EMAC_NETWORK_CONFIG);
	data &= ~(MDC_DIV_MASK << MDC_DIV_SHIFT);
	data |= (gem_mdcdiv & MDC_DIV_MASK) << MDC_DIV_SHIFT;
	writel(data, base + EMAC_NETWORK_CONFIG);
}

/** GEMAC reset function.
 * @param[in] base	GEMAC base address
 */
void gemac_reset(void *base)
{
}

/** GEMAC enable function.
 * @param[in] base	GEMAC base address
 */
void gemac_enable(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONTROL) | EMAC_TX_ENABLE |
		EMAC_RX_ENABLE, base + EMAC_NETWORK_CONTROL);
}

/** GEMAC disable function.
 * @param[in] base	GEMAC base address
 */
void gemac_disable(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONTROL) & ~(EMAC_TX_ENABLE |
		EMAC_RX_ENABLE), base + EMAC_NETWORK_CONTROL);
}

/** GEMAC set mac address configuration.
 * @param[in] base	GEMAC base address
 * @param[in] addr	MAC address to be configured
 */
void gemac_set_address(void *base, SPEC_ADDR *addr)
{
	writel(addr->one.bottom,	base + EMAC_SPEC1_ADD_BOT);
	writel(addr->one.top,		base + EMAC_SPEC1_ADD_TOP);
	writel(addr->two.bottom,	base + EMAC_SPEC2_ADD_BOT);
	writel(addr->two.top,		base + EMAC_SPEC2_ADD_TOP);
	writel(addr->three.bottom,	base + EMAC_SPEC3_ADD_BOT);
	writel(addr->three.top,		base + EMAC_SPEC3_ADD_TOP);
	writel(addr->four.bottom,	base + EMAC_SPEC4_ADD_BOT);
	writel(addr->four.top,		base + EMAC_SPEC4_ADD_TOP);
}

/** GEMAC get mac address configuration.
 * @param[in] base	GEMAC base address
 *
 * @return		MAC addresses configured
 */
SPEC_ADDR gemac_get_address(void *base)
{
	SPEC_ADDR addr;

	addr.one.bottom =	readl(base + EMAC_SPEC1_ADD_BOT);
	addr.one.top =		readl(base + EMAC_SPEC1_ADD_TOP);
	addr.two.bottom =	readl(base + EMAC_SPEC2_ADD_BOT);
	addr.two.top =		readl(base + EMAC_SPEC2_ADD_TOP);
	addr.three.bottom =	readl(base + EMAC_SPEC3_ADD_BOT);
	addr.three.top =	readl(base + EMAC_SPEC3_ADD_TOP);
	addr.four.bottom =	readl(base + EMAC_SPEC4_ADD_BOT);
	addr.four.top =		readl(base + EMAC_SPEC4_ADD_TOP);

	return addr;
}

/** GEMAC set specific local addresses of the MAC.
 * Rather than setting up all four specific addresses, this function sets them up
 * individually.
 *
 * @param[in] base	GEMAC base address
 * @param[in] addr	MAC address to be configured
 */
void gemac_set_laddr1(void *base, MAC_ADDR *address)
{
	writel(address->bottom,		base + EMAC_SPEC1_ADD_BOT);
	writel(address->top,		base + EMAC_SPEC1_ADD_TOP);
}


void gemac_set_laddr2(void *base, MAC_ADDR *address)
{
	writel(address->bottom,		base + EMAC_SPEC2_ADD_BOT);
	writel(address->top,		base + EMAC_SPEC2_ADD_TOP);
}


void gemac_set_laddr3(void *base, MAC_ADDR *address)
{
	writel(address->bottom,		base + EMAC_SPEC3_ADD_BOT);
	writel(address->top,		base + EMAC_SPEC3_ADD_TOP);
}


void gemac_set_laddr4(void *base, MAC_ADDR *address)
{
	writel(address->bottom,		base + EMAC_SPEC4_ADD_BOT);
	writel(address->top,		base + EMAC_SPEC4_ADD_TOP);
}

void gemac_set_laddrN(void *base, MAC_ADDR *address, unsigned int entry_index)
{
	if (entry_index < 5) {
		writel(address->bottom,		base + (entry_index *
			8) + EMAC_SPEC1_ADD_BOT);
		writel(address->top,		base + (entry_index * 8)
			+ EMAC_SPEC1_ADD_TOP);
	} else {
		writel(address->bottom,		base + ((entry_index
			- 5) * 8) + EMAC_SPEC5_ADD_BOT);
		writel(address->top,		base + ((entry_index -
			5) * 8) + EMAC_SPEC5_ADD_TOP);
	}
}

/** Get specific local addresses of the MAC.
 * This allows returning of a single specific address stored in the MAC.
 * @param[in] base	GEMAC base address
 *
 * @return		Specific MAC address 1
 *
 */
MAC_ADDR gem_get_laddr1(void *base)
{
	MAC_ADDR addr;

	addr.bottom = readl(base + EMAC_SPEC1_ADD_BOT);
	addr.top = readl(base + EMAC_SPEC1_ADD_TOP);
	return addr;
}


MAC_ADDR gem_get_laddr2(void *base)
{
	MAC_ADDR addr;

	addr.bottom = readl(base + EMAC_SPEC2_ADD_BOT);
	addr.top = readl(base + EMAC_SPEC2_ADD_TOP);
	return addr;
}


MAC_ADDR gem_get_laddr3(void *base)
{
	MAC_ADDR addr;

	addr.bottom = readl(base + EMAC_SPEC3_ADD_BOT);
	addr.top = readl(base + EMAC_SPEC3_ADD_TOP);
	return addr;
}


MAC_ADDR gem_get_laddr4(void *base)
{
	MAC_ADDR addr;

	addr.bottom = readl(base + EMAC_SPEC4_ADD_BOT);
	addr.top = readl(base + EMAC_SPEC4_ADD_TOP);
	return addr;
}


MAC_ADDR gem_get_laddrN(void *base, unsigned int entry_index)
{
	MAC_ADDR addr;

	if (entry_index < 5) {
		addr.bottom = readl(base + (entry_index * 8) +
					EMAC_SPEC1_ADD_BOT);
		addr.top = readl(base + (entry_index * 8) +
					EMAC_SPEC1_ADD_TOP);
	} else {
		addr.bottom = readl(base + ((entry_index - 5) * 8) +
					EMAC_SPEC5_ADD_BOT);
		addr.top = readl(base + ((entry_index - 5) * 8) +
					EMAC_SPEC5_ADD_TOP);
	}

	return addr;
}

/** GEMAC allow frames
 * @param[in] base	GEMAC base address
 */
void gemac_enable_copy_all(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) & EMAC_ENABLE_COPY_ALL, base +
		EMAC_NETWORK_CONFIG);
}

/** GEMAC do not allow frames
 * @param[in] base	GEMAC base address
 */
void gemac_disable_copy_all(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) & ~EMAC_ENABLE_COPY_ALL, base
			+ EMAC_NETWORK_CONFIG);
}



/** GEMAC allow broadcast function.
 * @param[in] base	GEMAC base address
 */
void gemac_allow_broadcast(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) & ~EMAC_NO_BROADCAST, base +
			EMAC_NETWORK_CONFIG);
}

/** GEMAC no broadcast function.
 * @param[in] base	GEMAC base address
 */
void gemac_no_broadcast(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) | EMAC_NO_BROADCAST, base +
			EMAC_NETWORK_CONFIG);
}

/** GEMAC enable unicast function.
 * @param[in] base	GEMAC base address
 */
void gemac_enable_unicast(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) | EMAC_ENABLE_UNICAST, base +
			EMAC_NETWORK_CONFIG);
}

/** GEMAC disable unicast function.
 * @param[in] base	GEMAC base address
 */
void gemac_disable_unicast(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) & ~EMAC_ENABLE_UNICAST, base +
			EMAC_NETWORK_CONFIG);
}

/** GEMAC enable multicast function.
 * @param[in] base	GEMAC base address
 */
void gemac_enable_multicast(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) | EMAC_ENABLE_MULTICAST, base
			+ EMAC_NETWORK_CONFIG);
}

/** GEMAC disable multicast function.
 * @param[in]	base	GEMAC base address
 */
void gemac_disable_multicast(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) & ~EMAC_ENABLE_MULTICAST, base
			+ EMAC_NETWORK_CONFIG);
}

/** GEMAC enable fcs rx function.
 * @param[in]	base	GEMAC base address
 */
void gemac_enable_fcs_rx(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) | EMAC_ENABLE_FCS_RX, base +
			EMAC_NETWORK_CONFIG);
}

/** GEMAC disable fcs rx function.
 * @param[in]	base	GEMAC base address
 */
void gemac_disable_fcs_rx(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) & ~EMAC_ENABLE_FCS_RX, base +
			EMAC_NETWORK_CONFIG);
}

/** GEMAC enable 1536 rx function.
 * @param[in]	base	GEMAC base address
 */
void gemac_enable_1536_rx(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) | EMAC_ENABLE_1536_RX, base +
			EMAC_NETWORK_CONFIG);
}

/** GEMAC disable 1536 rx function.
 * @param[in]	base	GEMAC base address
 */
void gemac_disable_1536_rx(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) & ~EMAC_ENABLE_1536_RX, base +
			EMAC_NETWORK_CONFIG);
}

/** GEMAC enable pause rx function.
 * @param[in] base	GEMAC base address
 */
void gemac_enable_pause_rx(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) | EMAC_ENABLE_PAUSE_RX, base +
			EMAC_NETWORK_CONFIG);
}

/** GEMAC disable pause rx function.
 * @param[in] base	GEMAC base address
 */
void gemac_disable_pause_rx(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) & ~EMAC_ENABLE_PAUSE_RX, base
			+ EMAC_NETWORK_CONFIG);
}

/** GEMAC enable rx checksum offload function.
 * @param[in] base	GEMAC base address
 */
void gemac_enable_rx_checksum_offload(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) | EMAC_ENABLE_CHKSUM_RX, base
			+ EMAC_NETWORK_CONFIG);
	writel(readl(CLASS_L4_CHKSUM_ADDR) | IPV4_CHKSUM_DROP,
			CLASS_L4_CHKSUM_ADDR);
}

/** GEMAC disable rx checksum offload function.
 * @param[in] base	GEMAC base address
 */
void gemac_disable_rx_checksum_offload(void *base)
{
	writel(readl(base + EMAC_NETWORK_CONFIG) & ~EMAC_ENABLE_CHKSUM_RX, base
			+ EMAC_NETWORK_CONFIG);
	writel(readl(CLASS_L4_CHKSUM_ADDR) & ~IPV4_CHKSUM_DROP,
			CLASS_L4_CHKSUM_ADDR);
}

/** Sets Gemac bus width to 64bit
 * @param[in] base       GEMAC base address
 * @param[in] width	gemac bus width to be set possible values are
 * 32/64/128
 *
 */
void gemac_set_bus_width(void *base, int width)
{
	u32 val = readl(base + EMAC_NETWORK_CONFIG);

	switch (width)  {
	case 32:
		val = (val & ~EMAC_DATA_BUS_WIDTH_MASK) |
			EMAC_DATA_BUS_WIDTH_32;
	case 128:
		val = (val & ~EMAC_DATA_BUS_WIDTH_MASK) |
			EMAC_DATA_BUS_WIDTH_128;
	case 64:
	default:
		val = (val & ~EMAC_DATA_BUS_WIDTH_MASK) |
			EMAC_DATA_BUS_WIDTH_64;

	}
	writel(val, base + EMAC_NETWORK_CONFIG);
}

/** Sets Gemac configuration.
 * @param[in] base	GEMAC base address
 * @param[in] cfg	GEMAC configuration
 */
void gemac_set_config(void *base, GEMAC_CFG *cfg)
{
	gemac_set_mode(base, cfg->mode);

	gemac_set_speed(base, cfg->speed);

	gemac_set_duplex(base, cfg->duplex);
}
#endif

/**************************** GPI ***************************/

/** Initializes a GPI block.
 * @param[in] base	GPI base address
 * @param[in] cfg	GPI configuration
 */
void gpi_init(void *base, GPI_CFG *cfg)
{
	gpi_reset(base);

	gpi_disable(base);

	gpi_set_config(base, cfg);
}

/** Resets a GPI block.
 * @param[in] base	GPI base address
 */
void gpi_reset(void *base)
{
	writel(CORE_SW_RESET, base + GPI_CTRL);
}

/** Enables a GPI block.
 * @param[in] base	GPI base address
 */
void gpi_enable(void *base)
{
	writel(CORE_ENABLE, base + GPI_CTRL);
}

/** Disables a GPI block.
 * @param[in] base	GPI base address
 */
void gpi_disable(void *base)
{
	writel(CORE_DISABLE, base + GPI_CTRL);
}


/** Sets the configuration of a GPI block.
 * @param[in] base	GPI base address
 * @param[in] cfg	GPI configuration
 */
void gpi_set_config(void *base, GPI_CFG *cfg)
{
	writel(CBUS_VIRT_TO_PFE(BMU1_BASE_ADDR + BMU_ALLOC_CTRL),	base
		+ GPI_LMEM_ALLOC_ADDR);
	writel(CBUS_VIRT_TO_PFE(BMU1_BASE_ADDR + BMU_FREE_CTRL),	base +
		GPI_LMEM_FREE_ADDR);
	writel(CBUS_VIRT_TO_PFE(BMU2_BASE_ADDR + BMU_ALLOC_CTRL),	base
		+ GPI_DDR_ALLOC_ADDR);
	writel(CBUS_VIRT_TO_PFE(BMU2_BASE_ADDR + BMU_FREE_CTRL),	base +
		GPI_DDR_FREE_ADDR);
	writel(CBUS_VIRT_TO_PFE(CLASS_INQ_PKTPTR),	base + GPI_CLASS_ADDR);
	writel(DDR_HDR_SIZE,		base + GPI_DDR_DATA_OFFSET);
	writel(LMEM_HDR_SIZE,		base + GPI_LMEM_DATA_OFFSET);
	writel(0,		base + GPI_LMEM_SEC_BUF_DATA_OFFSET);
	writel(0,		base + GPI_DDR_SEC_BUF_DATA_OFFSET);
	writel((DDR_HDR_SIZE << 16) |
		LMEM_HDR_SIZE,			base + GPI_HDR_SIZE);
	writel((DDR_BUF_SIZE << 16) |
		LMEM_BUF_SIZE,			base + GPI_BUF_SIZE);

	writel(((cfg->lmem_rtry_cnt << 16) | (GPI_DDR_BUF_EN << 1) |
		GPI_LMEM_BUF_EN),	base + GPI_RX_CONFIG);
	writel(cfg->tmlf_txthres,	base + GPI_TMLF_TX);
	writel(cfg->aseq_len,		base + GPI_DTX_ASEQ);

	/*Make GPI AXI transactions non-bufferable */
	writel(0x1,		base + GPI_AXI_CTRL);
}

/**************************** CLASSIFIER ***************************/

/** Initializes CLASSIFIER block.
 * @param[in] cfg	CLASSIFIER configuration
 */
void class_init(CLASS_CFG *cfg)
{
	class_reset();

	class_disable();

	class_set_config(cfg);
}

/** Resets CLASSIFIER block.
 *
 */
void class_reset(void)
{
	writel(CORE_SW_RESET, CLASS_TX_CTRL);
}

/** Enables all CLASS-PE's cores.
 *
 */
void class_enable(void)
{
	writel(CORE_ENABLE, CLASS_TX_CTRL);
}

/** Disables all CLASS-PE's cores.
 *
 */
void class_disable(void)
{
	writel(CORE_DISABLE, CLASS_TX_CTRL);
}

/** Sets the configuration of the CLASSIFIER block.
 * @param[in] cfg	CLASSIFIER configuration
 */
void class_set_config(CLASS_CFG *cfg)
{
	if (PLL_CLK_EN == 0) {
		/* Clock ratio: for 1:1 the value is 0 */
		writel(0x0,     CLASS_PE_SYS_CLK_RATIO);
	} else {
		/* Clock ratio: for 1:2 the value is 1 */
		writel(0x1,     CLASS_PE_SYS_CLK_RATIO);
	}
	writel((DDR_HDR_SIZE << 16) | LMEM_HDR_SIZE,	CLASS_HDR_SIZE);
	writel(LMEM_BUF_SIZE,				CLASS_LMEM_BUF_SIZE);
	writel(CLASS_ROUTE_ENTRY_SIZE(CLASS_ROUTE_SIZE) |
		CLASS_ROUTE_HASH_SIZE(cfg->route_table_hash_bits),
		CLASS_ROUTE_HASH_ENTRY_SIZE);
	writel(HASH_CRC_PORT_IP | QB2BUS_LE, CLASS_ROUTE_MULTI);

	writel(cfg->route_table_baseaddr,		CLASS_ROUTE_TABLE_BASE);
	memset((void *)DDR_PFE_TO_VIRT(cfg->route_table_baseaddr), 0,
		ROUTE_TABLE_SIZE);

	writel(CLASS_PE0_RO_DM_ADDR0_VAL,		CLASS_PE0_RO_DM_ADDR0);
	writel(CLASS_PE0_RO_DM_ADDR1_VAL,		CLASS_PE0_RO_DM_ADDR1);
	writel(CLASS_PE0_QB_DM_ADDR0_VAL,		CLASS_PE0_QB_DM_ADDR0);
	writel(CLASS_PE0_QB_DM_ADDR1_VAL,		CLASS_PE0_QB_DM_ADDR1);
	writel(CBUS_VIRT_TO_PFE(TMU_PHY_INQ_PKTPTR),	CLASS_TM_INQ_ADDR);

	writel(23, CLASS_AFULL_THRES);
	writel(23, CLASS_TSQ_FIFO_THRES);

	writel(24, CLASS_MAX_BUF_CNT);
	writel(24, CLASS_TSQ_MAX_CNT);
	/*Make Class AXI transactions non-bufferable */
	writel(0x1, CLASS_AXI_CTRL);
	/* writel(1, CLASS_USE_TMU_INQ); */

	/*Make Util AXI transactions non-bufferable */
	/*Util is disabled in U-boot, do it from here */
	writel(0x1, UTIL_AXI_CTRL);
}

/**************************** TMU ***************************/

void tmu_reset(void)
{
	writel(SW_RESET, TMU_CTRL);
}

/** Initializes TMU block.
 * @param[in] cfg	TMU configuration
 */
void tmu_init(TMU_CFG *cfg)
{
	int q, phyno;

	/* keep in soft reset */
	writel(SW_RESET,                     TMU_CTRL);

	/*Make Class AXI transactions non-bufferable */
	writel(0x1, TMU_AXI_CTRL);

	/* enable EMAC PHY ports */
	writel(0x3,			TMU_SYS_GENERIC_CONTROL);

	writel(750,				TMU_INQ_WATERMARK);
	writel(CBUS_VIRT_TO_PFE(EGPI1_BASE_ADDR +
		GPI_INQ_PKTPTR),	TMU_PHY0_INQ_ADDR);
	writel(CBUS_VIRT_TO_PFE(EGPI2_BASE_ADDR +
		GPI_INQ_PKTPTR),	TMU_PHY1_INQ_ADDR);
#if !defined(CONFIG_LS1012A)
	writel(CBUS_VIRT_TO_PFE(EGPI3_BASE_ADDR +
		GPI_INQ_PKTPTR),	TMU_PHY2_INQ_ADDR);
#endif
	writel(CBUS_VIRT_TO_PFE(HGPI_BASE_ADDR +
		GPI_INQ_PKTPTR),	TMU_PHY3_INQ_ADDR);
	writel(CBUS_VIRT_TO_PFE(HIF_NOCPY_RX_INQ0_PKTPTR), TMU_PHY4_INQ_ADDR);
	writel(CBUS_VIRT_TO_PFE(UTIL_INQ_PKTPTR),	TMU_PHY5_INQ_ADDR);
	writel(CBUS_VIRT_TO_PFE(BMU2_BASE_ADDR + BMU_FREE_CTRL),
		TMU_BMU_INQ_ADDR);

	/* enabling all 10 schedulers [9:0] of each TDQ  */
	writel(0x3FF,	TMU_TDQ0_SCH_CTRL);
	writel(0x3FF,	TMU_TDQ1_SCH_CTRL);
#if !defined(CONFIG_LS1012A)
	writel(0x3FF,	TMU_TDQ2_SCH_CTRL);
#endif
	writel(0x3FF,	TMU_TDQ3_SCH_CTRL);

	if (PLL_CLK_EN == 0) {
		/* Clock ratio: for 1:1 the value is 0 */
		writel(0x0,	TMU_PE_SYS_CLK_RATIO);
	} else {
		/* Clock ratio: for 1:2 the value is 1 */
		writel(0x1,	TMU_PE_SYS_CLK_RATIO);
	}
	debug("TMU_LLM_BASE_ADDR %x\n", cfg->llm_base_addr);
	/* Extra packet pointers will be stored from this address onwards */
	writel(cfg->llm_base_addr,	TMU_LLM_BASE_ADDR);

	debug("TMU_LLM_QUE_LEN %x\n", cfg->llm_queue_len);
	writel(cfg->llm_queue_len,	TMU_LLM_QUE_LEN);
	writel(5,			TMU_TDQ_IIFG_CFG);
	writel(DDR_BUF_SIZE,		TMU_BMU_BUF_SIZE);

	writel(0x0,			TMU_CTRL);

	/* MEM init */
	writel(MEM_INIT,	TMU_CTRL);

	while (!(readl(TMU_CTRL) & MEM_INIT_DONE))
		;

	/* LLM init */
	writel(LLM_INIT,	TMU_CTRL);

	while (!(readl(TMU_CTRL) & LLM_INIT_DONE))
		;

	/* set up each queue for tail drop */
	for (phyno = 0; phyno < 4; phyno++) {
#if !defined(CONFIG_LS1012A)
		if (phyno = 2)
			continue;
#endif
		for (q = 0; q < 16; q++) {
			u32 qmax;

			writel((phyno << 8) | q, TMU_TEQ_CTRL);
			writel(1 << 22, TMU_TEQ_QCFG);

			if (phyno == 3)
				qmax = DEFAULT_TMU3_QDEPTH;
			else
				qmax = (q == 0) ? DEFAULT_Q0_QDEPTH :
					DEFAULT_MAX_QDEPTH;

			writel(qmax << 18, TMU_TEQ_HW_PROB_CFG2);
			writel(qmax >> 14, TMU_TEQ_HW_PROB_CFG3);
		}
	}
	writel(0x05, TMU_TEQ_DISABLE_DROPCHK);
	writel(0,	TMU_CTRL);

}

/** Enables TMU-PE cores.
 * @param[in] pe_mask	TMU PE mask
 */
void tmu_enable(u32 pe_mask)
{
	writel(readl(TMU_TX_CTRL) | (pe_mask & 0xF), TMU_TX_CTRL);
}

/** Disables TMU cores.
 * @param[in] pe_mask	TMU PE mask
 */
void tmu_disable(u32 pe_mask)
{
	writel(readl(TMU_TX_CTRL) & ~(pe_mask & 0xF), TMU_TX_CTRL);
}

/**************************** UTIL ***************************/

/** Resets UTIL block.
 */
void util_reset(void)
{
	writel(CORE_SW_RESET, UTIL_TX_CTRL);
}

/** Initializes UTIL block.
 * @param[in] cfg	UTIL configuration
 */
void util_init(UTIL_CFG *cfg)
{

	/* writel(0x1, UTIL_MISC_REG); */
	/*Make Util AXI transactions non-bufferable */
	writel(0x1, UTIL_AXI_CTRL);

	if (PLL_CLK_EN == 0) {
		/* Clock ratio: for 1:1 the value is 0 */
		writel(0x0,     UTIL_PE_SYS_CLK_RATIO);
	} else {
		writel(0x1,     UTIL_PE_SYS_CLK_RATIO);
		/* Clock ratio: for 1:2 the value is 1 */
	}

}

/** Enables UTIL-PE core.
 *
 */
void util_enable(void)
{
	writel(CORE_ENABLE, UTIL_TX_CTRL);
}

/** Disables UTIL-PE core.
 *
 */
void util_disable(void)
{
	writel(CORE_DISABLE, UTIL_TX_CTRL);
}

#if 0
/** GEMAC PHY Statistics - This function return address of the first statistics
 * register
 * @param[in]	base	GEMAC base address
 */
unsigned int *gemac_get_stats(void *base)
{
	return (unsigned int *)(base + EMAC_OCT_TX_BOT);
}
#endif

/**************************** HIF ***************************/

/** Initializes HIF no copy block.
 *
 */
void hif_nocpy_init(void)
{
	writel(4,				HIF_NOCPY_TX_PORT_NO);
	writel(CBUS_VIRT_TO_PFE(BMU1_BASE_ADDR +
		BMU_ALLOC_CTRL),		HIF_NOCPY_LMEM_ALLOC_ADDR);
	writel(CBUS_VIRT_TO_PFE(CLASS_INQ_PKTPTR),	HIF_NOCPY_CLASS_ADDR);
	writel(CBUS_VIRT_TO_PFE(TMU_PHY_INQ_PKTPTR), HIF_NOCPY_TMU_PORT0_ADDR);

	/*Make HIF_NOCPY AXI transactions non-bufferable */
	writel(0x1, HIF_NOCPY_AXI_CTRL);
}

/** Initializes HIF copy block.
 *
 */
void hif_init(void)
{
	/* Initialize HIF registers */
	writel(HIF_RX_POLL_CTRL_CYCLE<<16|HIF_TX_POLL_CTRL_CYCLE,
		HIF_POLL_CTRL);
	/*Make HIF AXI transactions non-bufferable */
	writel(0x1, HIF_AXI_CTRL);
}

/** Enable hif tx DMA and interrupt
 *
 */
void hif_tx_enable(void)
{
	/* TODO not sure poll_cntrl_en is required or not */
	writel(HIF_CTRL_DMA_EN, HIF_TX_CTRL);
	/* writel((readl(HIF_INT_ENABLE) | HIF_INT_EN | HIF_TXPKT_INT_EN),
	 * HIF_INT_ENABLE);
	 */
}

/** Disable hif tx DMA and interrupt
 *
 */
void hif_tx_disable(void)
{
	u32	hif_int;

	writel(0, HIF_TX_CTRL);

	hif_int = readl(HIF_INT_ENABLE);
	hif_int &= HIF_TXPKT_INT_EN;
	writel(hif_int, HIF_INT_ENABLE);
}

/** Enable hif rx DMA and interrupt
 *
 */
void hif_rx_enable(void)
{
	/* TODO not sure poll_cntrl_en is required or not */
	writel((HIF_CTRL_DMA_EN | HIF_CTRL_BDP_CH_START_WSTB), HIF_RX_CTRL);
	/* writel((readl(HIF_INT_ENABLE) | HIF_INT_EN | HIF_RXPKT_INT_EN),
	 * HIF_INT_ENABLE);
	 */
}

/** Disable hif rx DMA and interrupt
 *
 */
void hif_rx_disable(void)
{
	u32	hif_int;

	writel(0, HIF_RX_CTRL);

	hif_int = readl(HIF_INT_ENABLE);
	hif_int &= HIF_RXPKT_INT_EN;
	writel(hif_int, HIF_INT_ENABLE);

}
