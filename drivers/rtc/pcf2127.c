/*
 * Copyright 2018-2019 NXP
 * Copyright (C) 2016 by NXP Semiconductors Inc.
 * Date & Time support for PCF2127 RTC
 */

/*	#define	DEBUG	*/

#include <common.h>
#include <command.h>
#include <rtc.h>
#include <i2c.h>

#if defined(CONFIG_CMD_DATE)

#define PCF2127_REG_CTRL1	0x00
#define PCF2127_REG_CTRL2	0x01
#define PCF2127_REG_CTRL3	0x02
#define PCF2127_REG_SC		0x03
#define PCF2127_REG_MN		0x04
#define PCF2127_REG_HR		0x05
#define PCF2127_REG_DM		0x06
#define PCF2127_REG_DW		0x07
#define PCF2127_REG_MO		0x08
#define PCF2127_REG_YR		0x09

int rtc_set(struct rtc_time *tm)
{
	uchar buf[8];
	int i = 0, err;

	/* start register address */
	buf[i++] = PCF2127_REG_SC;

	/* hours, minutes and seconds */
	buf[i++] = bin2bcd(tm->tm_sec);
	buf[i++] = bin2bcd(tm->tm_min);
	buf[i++] = bin2bcd(tm->tm_hour);
	buf[i++] = bin2bcd(tm->tm_mday);
	buf[i++] = tm->tm_wday & 0x07;

	/* month, 1 - 12 */
	buf[i++] = bin2bcd(tm->tm_mon + 1);

	/* year */
	buf[i++] = bin2bcd(tm->tm_year % 100);

	/* write register's data */
	err = i2c_write(CONFIG_SYS_I2C_RTC_ADDR,
			PCF2127_REG_CTRL1, 0, buf, sizeof(buf));

	return err;
}

int rtc_get(struct rtc_time *tmp)
{
	int ret = 0;
	uchar buf[10] = { PCF2127_REG_CTRL1 };

	ret = i2c_write(CONFIG_SYS_I2C_RTC_ADDR,
			PCF2127_REG_CTRL1, 0, buf, 1);
	if (ret < 0)
		return ret;
	ret = i2c_read(CONFIG_SYS_I2C_RTC_ADDR,
		       PCF2127_REG_CTRL1, 0, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	if (buf[PCF2127_REG_CTRL3] & 0x04)
		puts("### Warning: RTC Low Voltage - date/time not reliable\n");

	tmp->tm_sec  = bcd2bin(buf[PCF2127_REG_SC] & 0x7F);
	tmp->tm_min  = bcd2bin(buf[PCF2127_REG_MN] & 0x7F);
	tmp->tm_hour = bcd2bin(buf[PCF2127_REG_HR] & 0x3F);
	tmp->tm_mday = bcd2bin(buf[PCF2127_REG_DM] & 0x3F);
	tmp->tm_mon  = bcd2bin(buf[PCF2127_REG_MO] & 0x1F) - 1;
	tmp->tm_year = bcd2bin(buf[PCF2127_REG_YR]) + 1900;
	if (tmp->tm_year < 1970)
		tmp->tm_year += 100;	/* assume we are in 1970...2069 */
	tmp->tm_wday = buf[PCF2127_REG_DW] & 0x07;
	tmp->tm_yday = 0;
	tmp->tm_isdst = 0;

	debug("Get DATE: %4d-%02d-%02d (wday=%d)  TIME: %2d:%02d:%02d\n",
	      tmp->tm_year, tmp->tm_mon, tmp->tm_mday, tmp->tm_wday,
	      tmp->tm_hour, tmp->tm_min, tmp->tm_sec);

	return ret;
}

void rtc_reset(void)
{
	/*Doing nothing here*/
}
#endif
