/*
 * This file is part of CanFestival, a library implementing CanOpen Stack.
 *
 * Copyright 2018 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __TIMERSCFG_H__
#define __TIMERSCFG_H__

/* 
 * Whatever your microcontroller, the timer wont work if
 * TIMEVAL is not at least on 32 bits
 */
#define TIMEVAL UNS32

/* The timer of the flextimer counts from 0000 to 0xFFFF in counter mode */
#define TIMEVAL_MAX 0xFFFF

/* The timer is incrementing every 100 us. */
#define MS_TO_TIMEVAL(ms) ((ms) * 10)
#define US_TO_TIMEVAL(us) ((us) / 100)

#endif
