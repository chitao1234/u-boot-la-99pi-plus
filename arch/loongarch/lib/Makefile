# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2003-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.



obj-y += time.o
obj-y += cache.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-y += handoff.o
else
obj-y += interrupts.o
obj-y += reloc.o
obj-$(CONFIG_CMD_BOOTM) += bootm.o
endif
obj-y += mtdpart_simple.o

