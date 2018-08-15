/*
 * This file is part of CanFestival, a library implementing CanOpen Stack. 
 *
 * Copyright 2018 NXP
 *
 * Author: Jianchao Wang <jianchao.wang@nxp.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */


#ifndef __CAN_CANFESTIVAL__
#define __CAN_CANFESTIVAL__

#include "applicfg.h"
#include "data.h"

/* ---------  to be called by user app --------- */
UNS8 canSend(CAN_PORT notused, Message *m);
void timerForCan(void);/* defined in ls1021aiot_canfestival.c */

#endif
