
/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>

#ifdef CONFIG_TARGET_LS1021AIOT
#define I2C_AUDIO_ADDR	0x2a
#define I2C_AUDIO_OFFSET0_DATA	0xa0
#elif defined(CONFIG_TARGET_LS1043ARDB) || defined(CONFIG_TARGET_LS1046ARDB)
#define I2C_AUDIO_ADDR  0x40
#define I2C_AUDIO_OFFSET0_DATA  0x39
#endif


void test_i2c(void)
{
	/* I2C_AUDIO_ADDR is address of Audio codec. */
	int chip = I2C_AUDIO_ADDR;
	int devaddr = 0;
	int alen = 1;
	unsigned char   linebuf[10];
	int length = 1;
	int bus_no;
	int ret;

	bus_no = 0;
	ret = i2c_set_bus_num(bus_no);
	if (ret != 0) {
	    printf("[error]i2c test error, set bus error\n");
	    return;
	}
	ret = i2c_read(chip, devaddr, alen, linebuf, length);
	if (ret != 0) {
	    printf("[error]i2c test error, read error\n");
	    return;
	}
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
