/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <div64.h>
#include <dm.h>
#include <malloc.h>
#include <mapmem.h>
#include <spi.h>
#include <spi_flash.h>
#include <jffs2/jffs2.h>
#include <linux/mtd/mtd.h>

#include <asm/io.h>
#include <dm/device-internal.h>

static struct spi_flash *flash;

static int test_qspi_flash_probe(void)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
#ifdef CONFIG_DM_SPI_FLASH
	struct udevice *new, *bus_dev;
	int ret;
	/* In DM mode defaults will be taken from DT */
	speed = 0, mode = 0;
#else
	struct spi_flash *new;
#endif


#ifdef CONFIG_DM_SPI_FLASH
	/* Remove the old device, otherwise probe will just be a nop */
	ret = spi_find_bus_and_cs(bus, cs, &bus_dev, &new);
	if (!ret)
		device_remove(new, DM_REMOVE_NORMAL);

	flash = NULL;
	ret = spi_flash_probe_bus_cs(bus, cs, speed, mode, &new);
	if (ret) {
		printf("Failed to initialize SPI flash at %u:%u (error %d)\n",
		       bus, cs, ret);
		return 1;
	}

	flash = dev_get_uclass_priv(new);
#else
	if (flash)
		spi_flash_free(flash);

	new = spi_flash_probe(bus, cs, speed, mode);
	flash = new;

	if (!new) {
		printf("Failed to initialize SPI flash at %u:%u\n", bus, cs);
		return 1;
	}

	flash = new;
#endif

	return 0;
}
/**
 * Run a test on the SPI flash
 *
 * @param flash		SPI flash to use
 * @param buf		Source buffer for data to write
 * @param len		Size of data to read/write
 * @param offset	Offset within flash to check
 * @param vbuf		Verification buffer
 * @return 0 if ok, -1 on error
 */
static int qspi_flash_test(struct spi_flash *flash, uint8_t *buf, ulong len,
			   ulong offset, uint8_t *vbuf)
{
	int i;
	ulong size;

	printf("QSPI flash test:\n");
	size = ROUND(len, flash->sector_size);
	if (spi_flash_erase(flash, offset, size)) {
		printf("Erase failed\n");
		return -1;
	}

	if (spi_flash_read(flash, offset, len, vbuf)) {
		printf("Check read failed\n");
		return -1;
	}
	for (i = 0; i < len; i++) {
		if (vbuf[i] != 0xff) {
			printf("Check failed at %d\n", i);
			print_buffer(i, vbuf + i, 1,
				     min_t(uint, len - i, 0x40), 0);
			return -1;
		}
	}

	if (spi_flash_write(flash, offset, len, buf)) {
		printf("Write failed\n");
		return -1;
	}

	memset(vbuf, '\0', len);
	if (spi_flash_read(flash, offset, len, vbuf)) {
		printf("Read failed\n");
		return -1;
	}

	for (i = 0; i < len; i++) {
		if (buf[i] != vbuf[i]) {
			printf("Verify failed at %d, good data:\n", i);
			print_buffer(i, buf + i, 1,
				     min_t(uint, len - i, 0x40), 0);
			printf("Bad data:\n");
			print_buffer(i, vbuf + i, 1,
				     min_t(uint, len - i, 0x40), 0);
			return -1;
		}
	}
	printf("QSPI test ok\n");

	return 0;
}

static int do_test_qspi_flash(ulong offset, ulong len)
{
	uint8_t *buf, *from;
	char *endp;
	uint8_t *vbuf;
	int ret;

	vbuf = malloc(len);
	if (!vbuf) {
		printf("Cannot allocate memory (%lu bytes)\n", len);
		return 1;
	}
	buf = malloc(len);
	if (!buf) {
		free(vbuf);
		printf("Cannot allocate memory (%lu bytes)\n", len);
		return 1;
	}

	from = map_sysmem(CONFIG_SYS_TEXT_BASE, 0);
	memcpy(buf, from, len);

	ret = qspi_flash_test(flash, buf, len, offset, vbuf);
	free(vbuf);
	free(buf);
	if (ret) {
		printf("QSPI test failed!\n");
		return 1;
	}

	return 0;
}

int test_qspi(void)
{
	int ret;

	ret = test_qspi_flash_probe();

	/* The remaining commands require a selected device */
	if (!flash) {
		puts("No SPI flash selected.\n");
		return 1;
	}
	if (ret == 0)
		do_test_qspi_flash(0x3f00000, 0x40000);

	return 0;
}
