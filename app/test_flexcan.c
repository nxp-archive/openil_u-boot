/*
 * Flexcan or CANopen test code.
 *
 * Copyright 2018 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <linux/delay.h>
#include <common.h>
#include <flexcan.h>
#include <flextimer.h>
#include <environment.h>

#if CONFIG_CANFESTIVAL
/* the head file of canfestival */
#include "canfestival.h"
#include "data.h"
#include "TestSlave.h"
#include "callback.h"
#endif

void flexcan_rx_irq(struct can_module *canx)
{
	struct can_frame rx_message;

	flexcan_receive(canx, &rx_message);
#if CONFIG_CANFESTIVAL
	/* the CAN message structure that is defined by canopen */
	Message m;
	u8 i = 0;

	if (TestSlave_Data.nodeState == Unknown_state)
		setState(&TestSlave_Data, Stopped);

	if (rx_message.can_id & CAN_EFF_FLAG) {
		printf("Error: the current canopen "
		       "doesn't support extended frame!\n");
		return;
	}
	m.cob_id = rx_message.can_id & CAN_SFF_MASK;
	if (rx_message.can_id & CAN_RTR_FLAG)
		m.rtr = 1;
	else if (rx_message.can_id & CAN_ERR_FLAG)
		printf("Warn: this is a error CAN message!\n");
	else
		m.rtr = 0;
	m.len = rx_message.can_dlc;
	for (i = 0; i < rx_message.can_dlc; i++)
		m.data[i] = rx_message.data[i];

	canDispatch(&TestSlave_Data, &m);
#else
	/* test code */
	rx_message.can_id++;
	flexcan_send(canx, &rx_message);
#endif
}

/* entry into the interrupt of flextimer one time per 100us */
void flextimer_overflow_irq(void)
{
#if CONFIG_CANFESTIVAL
	timerForCan();
#else
	/* test code */
	static unsigned int flag;
	static struct can_frame canfram_tx = {
		.can_id = 0x00000003,
		.can_dlc = 6,
		.data = "timer!"
	};
	flag++;
	if (flag >= 100000) {/* 10s */
		flexcan_send(CAN3, &canfram_tx);
		flag = 0;
	}
#endif
}

void test_flexcan(void)
{
	flexcan_rx_handle = flexcan_rx_irq;
	flextimer_overflow_handle = flextimer_overflow_irq;

#if CONFIG_CANFESTIVAL
	u8 node_id = 0x02;

	printf("Note: the CANopen protocol starts to run!\n");
	callback_list_init();
	setNodeId(&TestSlave_Data, node_id);
	/* setState(&TestSlave_Data, Stopped); */
#else
	/* test code */
	struct can_frame canfram_tx = {
		.can_id = 0x00000003,
		.can_dlc = 6,
		.data = "start!"
	};

	flexcan_send(CAN3, &canfram_tx);
#endif
}
