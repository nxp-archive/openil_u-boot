#
# Copyright 2015 Freescale Semiconductor
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += ddr.o
obj-y += ls1043ardb.o
ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_NET) += eth.o
ifndef CONFIG_TARGET_LS1028ARDB
obj-y += cpld.o
endif
endif
