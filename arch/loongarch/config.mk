# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2024
# Loongson Ltd.


32bit-emul		:= elf32loongarch
64bit-emul		:= elf64loongarch
32bit-bfd		:= elf32-loongarch
64bit-bfd		:= elf64-loongarch

ifdef CONFIG_64BIT
ifneq ($(CONFIG_GCC_VERSION), $(firstword $(shell echo "140000 $(CONFIG_GCC_VERSION)" | tr ' ' '\n' | sort -n)))
PLATFORM_CPPFLAGS	+= $(call cc-option, -mabi-lp64d)
PLATFORM_CPPFLAGS	+= $(call cc-option, -fpermissive)
PLATFORM_CPPFLAGS	+= $(call cc-option, -mno-relax)
else
PLATFORM_CPPFLAGS	+= $(call cc-option, -mabi-lp64)
endif
KBUILD_LDFLAGS		+= -m $(64bit-emul)
OBJCOPYFLAGS		+= -O $(64bit-bfd)
CONFIG_STANDALONE_LOAD_ADDR	?= 0x9000000080200000
else
PLATFORM_CPPFLAGS	+= -mabi=lp32
KBUILD_LDFLAGS		+= -m $(32bit-emul)
OBJCOPYFLAGS		+= -O $(32bit-bfd)
CONFIG_STANDALONE_LOAD_ADDR	?= 0x80200000
endif

PLATFORM_CPPFLAGS += -D__LOONGARCH__
PLATFORM_ELFFLAGS += -B loongarch $(OBJCOPYFLAGS)

PLATFORM_CPPFLAGS		+= -G 0 -fno-pic -fno-builtin -fno-delayed-branch -fno-plt
PLATFORM_CPPFLAGS		+= -msoft-float -mstrict-align
KBUILD_LDFLAGS			+= -G 0 -static -n -nostdlib
PLATFORM_RELFLAGS		+= -ffunction-sections -fdata-sections
LDFLAGS_FINAL			+= --gc-sections
LDFLAGS_STANDALONE		+= --gc-sections

LDFLAGS_u-boot += --gc-sections -static -pie
