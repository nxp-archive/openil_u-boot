// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2021 NXP
 *
 */

#include <common.h>
#include <i2c.h>

#define I2C_AUDIO_ADDR	0x2a
#define I2C_AUDIO_OFFSET0_DATA	0xa0

void test_i2c(void)
{
	/* I2C_AUDIO_ADDR is address of Audio codec. */
	int chip = I2C_AUDIO_ADDR;
	int devaddr = 0;
	int alen = 1;
	unsigned char   linebuf[10];
	int length = 1;
	int bus_no;

	bus_no = 0;
	i2c_set_bus_num(bus_no);
	i2c_read(chip, devaddr, alen, linebuf, length);
	printf("i2c read: 0x%x\n", linebuf[0]);

	/* TODO: use i2c_write(uint8_t chip_addr,
	 * unsigned int addr, int alen, uint8_t *buffer, int len)
	 * func to write i2c device.
	 */

	/* I2C_AUDIO_OFFSET0_DATA is a fixed data that is stored
	 * in offset 0 of Audio codec device.
	 */
	if (linebuf[0] == I2C_AUDIO_OFFSET0_DATA)
		printf("[ok]i2c test ok\n");
	else
		printf("[error]i2c test error\n");

	return;
}
