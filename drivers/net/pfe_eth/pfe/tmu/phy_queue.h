/* Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * SPDX-License-Identifier:GPL-2.0+
 */
#ifndef _PHY_QUEUE_H_
#define _PHY_QUEUE_H_

/**< [28:19] same as SHAPER_STATUS, [18:3] same as QUEUE_STATUS,
 * [2:0] must be zero before a new packet may be dequeued
 */
#define PHY_QUEUE_SHAPER_STATUS	(PHY_QUEUE_BASE_ADDR + 0x00)
/**< [15:0] bit mask of input queues with pending packets */
#define QUEUE_STATUS		(PHY_QUEUE_BASE_ADDR + 0x04)

#define QUEUE0_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x08)
#define QUEUE1_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x0c)
#define QUEUE2_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x10)
#define QUEUE3_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x14)
#define QUEUE4_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x18)
#define QUEUE5_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x1c)
#define QUEUE6_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x20)
#define QUEUE7_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x24)
#define QUEUE8_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x28)
#define QUEUE9_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x2c)
#define QUEUE10_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x30)
#define QUEUE11_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x34)
#define QUEUE12_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x38)
#define QUEUE13_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x3c)
#define QUEUE14_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x40)
#define QUEUE15_PKT_LEN		(PHY_QUEUE_BASE_ADDR + 0x44)
/**< [7] set to one to indicate output PHY (TMU0->PHY0, TMU1->PHY1,
 * TMU2->PHY2, TMU3->PHY3), [6:0] winner input queue number
 */
#define QUEUE_RESULT0		(PHY_QUEUE_BASE_ADDR + 0x48)
/**< [7] set to one to indicate output PHY (TMU0->PHY0, TMU1->PHY1,
 * TMU2->PHY2, TMU3->PHY4), [6:0] winner input queue number
 */
#define QUEUE_RESULT1		(PHY_QUEUE_BASE_ADDR + 0x4c)
/**< [7] set to one to indicate output PHY (TMU0->PHY0, TMU1->PHY1,
 * TMU2->PHY2, TMU3->PHY5), [6:0] winner input queue number
 */
#define QUEUE_RESULT2		(PHY_QUEUE_BASE_ADDR + 0x50)
#define QUEUE_GBL_PKTLEN	(PHY_QUEUE_BASE_ADDR + 0x5c)
#define QUEUE_GBL_PKTLEN_MASK	(PHY_QUEUE_BASE_ADDR + 0x60)



#endif /* _PHY_QUEUE_H_ */
