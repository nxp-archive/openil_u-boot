
/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include <console.h>
#include <environment.h>
#include "test_net.h"

#define ping_ip		"192.168.1.2"
#define ipaddr		"192.168.1.1"


/* before run this func, need connect network */
int test_net(void)
{
	net_ping_ip = string_to_ip(ping_ip);
	net_ip = string_to_ip(ipaddr);

	printf("net test case\n");
	if (net_ping_ip.s_addr == 0)
		return CMD_RET_USAGE;

	if (net_loop(PING) < 0) {
		printf("ping failed; host %s is not alive\n", ipaddr);
		return CMD_RET_FAILURE;
	}

	printf("host %s is alive\n", ipaddr);

	return CMD_RET_SUCCESS;
}
