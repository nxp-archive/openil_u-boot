// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2020 NXP
 *
 */

#include <common.h>
#include <asm-generic/gpio.h>

int test_set_gpio(int value)
{
	int ngpio = 135;  /* GPIO5_7 is the pin 8 of J1003 */
	char label[] = "output135";

	gpio_request(ngpio, label);
	gpio_direction_output(ngpio, value);
	gpio_free(ngpio);

	return 0;
}

int test_get_gpio(void)
{
	int ngpio = 136;  /* GPIO5_8 is the pin 7 of J1003 */
	char label[] = "input136";
	int value;

	gpio_request(ngpio, label);
	gpio_direction_input(ngpio);
	value = gpio_get_value(ngpio);
	gpio_free(ngpio);

	return value;
}

/* before run this func, need connect J1003.7 and J1003.8 */
int test_gpio(void)
{
	int value;
	int ret = 0;

	test_set_gpio(1);
	udelay(1000000);
	value = test_get_gpio();
	if (value == 1) {
		test_set_gpio(0);
		udelay(1000000);
		value = test_get_gpio();
		if (value == 0)
			ret = 1;
	}
	if (ret == 0)
		printf("[error]GPIO test failed\n");
	else
		printf("[ok]GPIO test ok\n");

	return ret;
}
