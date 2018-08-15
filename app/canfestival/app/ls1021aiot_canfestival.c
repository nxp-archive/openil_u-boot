/*
 * Canfestival Support
 *
 * Copyright 2018 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include "canfestival.h"
#include "flexcan.h"

unsigned int time_cnt = 0;/* the value of timer */
unsigned int next_time = 0;/* next trigger time */
unsigned int TIMER_MAX_COUNT = TIMEVAL_MAX;/* the max value of timer */
static TIMEVAL last_time_set = TIMEVAL_MAX;/* last trigger timer */

/* Set the next alarm */
void setTimer(TIMEVAL value)
{
	next_time = (time_cnt + value) % TIMER_MAX_COUNT;
}

/* Get the elapsed time since the last occured alarm */
TIMEVAL getElapsedTime(void)
{
	int ret=0;

	ret = time_cnt >= last_time_set ? time_cnt - last_time_set :
				time_cnt + TIMER_MAX_COUNT - last_time_set;

	return ret;
}

/* call this function per 100us */
void timerForCan(void)
{
	time_cnt ++;
	if (time_cnt >= TIMER_MAX_COUNT) {
		time_cnt = 0;
	}
	/* when no alarm has been set, trigger it per 65535ms */
	if (time_cnt == next_time) {
		/* save the current value of timer as last value */
		last_time_set = time_cnt;
		TimeDispatch();
	}
}

static struct can_frame tx_message;
unsigned char canSend(CAN_PORT notused, Message *m)
{
	uint32_t i;

	tx_message.can_id = m->cob_id & CAN_SFF_MASK;
	tx_message.can_id &= ~CAN_EFF_FLAG;

	if (m->rtr)
		tx_message.can_id |= CAN_RTR_FLAG;
	else {
		tx_message.can_id &= ~CAN_RTR_FLAG;
		tx_message.can_id &= ~CAN_ERR_FLAG;
	}
	tx_message.can_dlc = m->len;

	for (i = 0; i < m->len; i++)
		tx_message.data[i] = m->data[i];

	return flexcan_send(CAN3, &tx_message);
}
