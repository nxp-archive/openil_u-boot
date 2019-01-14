#
# Copyright 2015 Freescale Semiconductor, Inc.
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += ddr.o
ifndef CONFIG_SPL_BUILD
ifndef CONFIG_TARGET_LS1028AQDS
obj-y += eth.o
else
obj-y += eth_ls1028int.o
endif
endif
obj-y += ls1043aqds.o
