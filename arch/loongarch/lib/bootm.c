// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <bootm.h>
#include <bootstage.h>
#include <env.h>
#include <image.h>
#include <fdt_support.h>
#include <lmb.h>
#include <log.h>
#include <asm/addrspace.h>
#include <asm/global_data.h>
#include <asm/io.h>

#if defined(CONFIG_LOONGSON_BOOT_FIXUP)
extern void *build_boot_param(void);
#endif

DECLARE_GLOBAL_DATA_PTR;

#define	LINUX_MAX_ENVS		256
#define	LINUX_MAX_ARGS		256

static int linux_argc;
static char **linux_argv;
static char *linux_argp;

static char **linux_env;
static char *linux_env_p;
static int linux_env_idx;

static ulong arch_get_sp(void)
{
	ulong ret;

	__asm__ __volatile__("move %0, $sp" : "=r"(ret) : );

	return ret;
}

void arch_lmb_reserve(struct lmb *lmb)
{
	ulong sp;

	sp = arch_get_sp();
	debug("## Current stack ends at 0x%08lx\n", sp);

	/* adjust sp by 4K to be safe */
	sp -= 4096;
	lmb_reserve(lmb, sp, gd->ram_top - sp);
}

static int boot_reloc_fdt(struct bootm_headers *images)
{
	/*
	 * In case of legacy uImage's, relocation of FDT is already done
	 * by do_bootm_states() and should not repeated in 'bootm prep'.
	 */
	if (images->state & BOOTM_STATE_FDT) {
		debug("## FDT already relocated\n");
		return 0;
	}

#if CONFIG_IS_ENABLED(OF_LIBFDT)
	boot_fdt_add_mem_rsv_regions(&images->lmb, images->ft_addr);
	return boot_relocate_fdt(&images->lmb, &images->ft_addr,
		&images->ft_len);
#else
	return 0;
#endif
}

static int boot_setup_fdt(struct bootm_headers *images)
{
	images->initrd_start = virt_to_phys((void *)images->initrd_start);
	images->initrd_end = virt_to_phys((void *)images->initrd_end);
	//return image_setup_libfdt(images, images->ft_addr, images->ft_len,
	//	&images->lmb);
	return image_setup_libfdt(images, images->ft_addr, &images->lmb);
}
static char *linux_command_line;
static void boot_prep_linux(struct bootm_headers *images)
{
	char *bootargs = env_get("bootargs");
	linux_command_line = bootargs;
	if (images->ft_len) {
		boot_reloc_fdt(images);
		boot_setup_fdt(images);
	} else {
		long rd_start, rd_size;
		if (images->initrd_start) {
			rd_start = images->initrd_start;
			rd_size = images->initrd_end - images->initrd_start;
			linux_command_line = malloc((bootargs ? strlen(bootargs) : 0) + 64);
			if (bootargs)
				sprintf(linux_command_line, "%s rd_start=0x%016llX rd_size=0x%lX", bootargs, (long long)(long)rd_start, rd_size);
			else
				sprintf(linux_command_line, "rd_start=0x%016llX rd_size=0x%lX", (long long)(long)rd_start, rd_size);
		}
	}
}

static void boot_jump_linux(struct bootm_headers *images)
{
	typedef void __noreturn (*kernel_entry_t)(int, ulong, ulong, ulong);
	kernel_entry_t kernel = (kernel_entry_t)map_to_sysmem((void*)images->ep);
#if defined(CONFIG_LOONGSON_BOOT_FIXUP)
	void *fw_arg2 = NULL, *fw_arg3 = NULL;
	void *bootparam = NULL;
#endif

	debug("## Transferring control to Linux (at address %p) ...\n", kernel);

	bootstage_mark(BOOTSTAGE_ID_RUN_OS);

#if CONFIG_IS_ENABLED(BOOTSTAGE_FDT)
	bootstage_fdt_add_report();
#endif
#if CONFIG_IS_ENABLED(BOOTSTAGE_REPORT)
	bootstage_report();
#endif

#if defined(CONFIG_LOONGSON_BOOT_FIXUP)

	bootparam = build_boot_param();
	fw_arg2 = *(unsigned long long *)(bootparam + 8);
	fw_arg3 = 0;

	kernel(0, (ulong)linux_command_line, (ulong)fw_arg2,
			(ulong)fw_arg3);
#else
	if (images->ft_len) {
		kernel(-2, (ulong)images->ft_addr, 0, 0);
	} else {
		kernel(linux_argc, (ulong)linux_argv, (ulong)linux_env,
			linux_extra);
	}
#endif
}

int do_bootm_linux(int flag, struct bootm_info *bmi)
{
	struct bootm_headers *images = bmi->images;
	
    if (flag & BOOTM_STATE_OS_BD_T)
		return -1;

	/*
	 * Cmdline init has been moved to 'bootm prep' because it has to be
	 * done after relocation of ramdisk to always pass correct values
	 * for rd_start and rd_size to Linux kernel.
	 */
	if (flag & BOOTM_STATE_OS_CMDLINE)
		return 0;

	if (flag & BOOTM_STATE_OS_PREP) {
		boot_prep_linux(images);
		sync();
		return 0;
	}

	if (flag & (BOOTM_STATE_OS_GO | BOOTM_STATE_OS_FAKE_GO)) {
		boot_jump_linux(images);
		return 0;
	}

	/* does not return */
	return 1;
}
