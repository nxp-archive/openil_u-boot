/*
 * Define some functions that are related with 
 * calback function for canfestival resource files.
 *
 * Copyright 2018 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _CALLBACK__H_
#define _CALLBACK__H_

#include "data.h"

typedef void (*callback_func_t)(CO_Data *);

extern callback_func_t initialization_callback;
extern callback_func_t pre_operation_callback;
extern callback_func_t operation_callback;
extern callback_func_t stop_callback;

void callback_list_init(void);

#endif
