# SPDX-License-Identifier: GPL-2.0+

head-y := arch/loongarch/cpu/start.o

ifeq ($(CONFIG_SPL_BUILD),y)
ifneq ($(CONFIG_SPL_START_S_PATH),)
head-y := $(CONFIG_SPL_START_S_PATH:"%"=%)/start.o
endif
endif

libs-y += arch/loongarch/cpu/
libs-y += arch/loongarch/lib/


machine-$(CONFIG_ARCH_LA_LOONGSON) += loongson

machdirs := $(patsubst %,arch/loongarch/mach-%/,$(machine-y))
libs-y += $(machdirs)

PLATFORM_CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))

# Optimize for Loongarch architectures
arch-$(CONFIG_CPU_LOONGARCH64) += -march=loongarch64 

# Allow extra optimization for specific CPUs/SoCs
tune-$(CONFIG_CPU_LOONGARCH64) = -mtune=loongarch64

# Include default header files
cflags-y += -I$(srctree)/arch/loongarch/include/asm/mach-generic

PLATFORM_CPPFLAGS += $(arch-y) $(tune-y) $(cflags-y)
