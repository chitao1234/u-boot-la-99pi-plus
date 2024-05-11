#
# (C) Copyright 2019
# <tang-haifeng@foxmail.com> <pengren.mcu@qq.com>
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-$(CONFIG_MACH_LOONGSON)		+= platform.o
obj-$(CONFIG_MACH_LOONGSON)		+= bdinfo/
obj-$(CONFIG_LOONGSON_COMPAT)		+= multi_boards_compat.o
obj-$(CONFIG_MACH_LOONGSON)			+= board_kernel_product.o loongson_board_info.o

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_MACH_LOONGSON)		+= loongson_update.o boot_cfg.o loongson_storage_read_file.o
obj-$(CONFIG_LOONGSON_RECOVER)		+= recover.o
obj-$(CONFIG_LOONGSON_BOOT_FIXUP)	+= bootparam.o loongson_efi_systab.o
obj-y += loongson_boot.o
obj-y += loongson_env_trigger.o
obj-y += loongson_init_env.o
obj-y += loongson_stdout_operation.o
obj-${CONFIG_LOONGSON_GENERAL_LOAD} += general_load.o
obj-${CONFIG_LOONGSON_GENERAL_LOAD} += cmd_general_load.o

ifeq ($(CONFIG_MENU), y)
obj-$(CONFIG_MACH_LOONGSON) += loongson_update_menu.o
endif

ifneq ($(findstring $(CONFIG_$(SPL_TPL_)VIDEO), y),)
obj-y += rotation_menu.o resolution_menu.o
endif

endif
