/*
 * Canfestival callback Support
 *
 * Copyright 2018 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include "callback.h"

callback_func_t initialization_callback 	= NULL;
callback_func_t pre_operation_callback		= NULL;
callback_func_t operation_callback		= NULL;
callback_func_t stop_callback			= NULL;

void node_initialize_callback(CO_Data *d)
{
	printf("Note: slave node initialization is complete!\n");
}

void node_pre_operation_callback(CO_Data *d)
{
	printf("Note: slave node entry into the preOperation mode!\n");
}

void node_operation_callback(CO_Data *d)
{
	printf("Note: slave node entry into the operation mode!\n");
}

void node_stop_callback(CO_Data *d)
{
	printf("Note: slave node entry into the stop mode!\n");
}

void callback_list_init(void)
{
	initialization_callback = node_initialize_callback;
	pre_operation_callback  = node_pre_operation_callback;
	operation_callback      = node_operation_callback;
	stop_callback		= node_stop_callback;
}
