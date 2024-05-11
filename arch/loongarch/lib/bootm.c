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

static void linux_cmdline_init(void)
{
	linux_argc = 1;
	linux_argv = (char **)map_sysmem(gd->bd->bi_boot_params, 0);
	linux_argv[0] = 0;
	linux_argp = (char *)(linux_argv + LINUX_MAX_ARGS);
}

static void linux_cmdline_set(const char *value, size_t len)
{
	linux_argv[linux_argc] = linux_argp;
	memcpy(linux_argp, value, len);
	linux_argp[len] = 0;

	linux_argp += len + 1;
	linux_argc++;
}

static void linux_cmdline_dump(void)
{
	int i;

	debug("## cmdline argv at 0x%p, argp at 0x%p\n",
	      linux_argv, linux_argp);

	for (i = 1; i < linux_argc; i++)
		debug("   arg %03d: %s\n", i, linux_argv[i]);
}

static void linux_cmdline_legacy(struct bootm_headers *images)
{
	const char *bootargs, *next, *quote;

	linux_cmdline_init();

	bootargs = env_get("bootargs");
	if (!bootargs)
		return;

	next = bootargs;

	while (bootargs && *bootargs && linux_argc < LINUX_MAX_ARGS) {
		quote = strchr(bootargs, '"');
		next = strchr(bootargs, ' ');

		while (next && quote && quote < next) {
			/*
			 * we found a left quote before the next blank
			 * now we have to find the matching right quote
			 */
			next = strchr(quote + 1, '"');
			if (next) {
				quote = strchr(next + 1, '"');
				next = strchr(next + 1, ' ');
			}
		}

		if (!next)
			next = bootargs + strlen(bootargs);

		linux_cmdline_set(bootargs, next - bootargs);

		if (*next)
			next++;

		bootargs = next;
	}
}

// static void linux_cmdline_append(struct bootm_headers *images)
// {
// 	char buf[24];
// 	ulong mem, rd_start, rd_size;

// 	/* append mem */
// 	mem = gd->ram_size >> 20;
// 	sprintf(buf, "mem=%luM", mem);
// 	linux_cmdline_set(buf, strlen(buf));

// 	/* append rd_start and rd_size */
// 	rd_start = images->initrd_start;
// 	rd_size = images->initrd_end - images->initrd_start;

// 	if (rd_size) {
// 		sprintf(buf, "rd_start=0x%08lX", rd_start);
// 		linux_cmdline_set(buf, strlen(buf));
// 		sprintf(buf, "rd_size=0x%lX", rd_size);
// 		linux_cmdline_set(buf, strlen(buf));
// 	}
// }

static void linux_env_init(void)
{
	linux_env = (char **)(((ulong) linux_argp + 15) & ~15);
	linux_env[0] = 0;
	linux_env_p = (char *)(linux_env + LINUX_MAX_ENVS);
	linux_env_idx = 0;
}

static void linux_env_set(const char *env_name, const char *env_val)
{
	if (linux_env_idx < LINUX_MAX_ENVS - 1) {
		linux_env[linux_env_idx] = linux_env_p;

		strcpy(linux_env_p, env_name);
		linux_env_p += strlen(env_name);
		*linux_env_p++ = '=';

		strcpy(linux_env_p, env_val);
		linux_env_p += strlen(env_val);

		linux_env_p++;
		linux_env[++linux_env_idx] = 0;
	}
}

static void linux_env_legacy(struct bootm_headers *images)
{
	char env_buf[12];
	const char *cp;
	ulong rd_start, rd_size;

	if (CONFIG_IS_ENABLED(MEMSIZE_IN_BYTES)) {
		sprintf(env_buf, "%lu", (ulong)gd->ram_size);
		debug("## Giving linux memsize in bytes, %lu\n",
		      (ulong)gd->ram_size);
	} else {
		sprintf(env_buf, "%lu", (ulong)(gd->ram_size >> 20));
		debug("## Giving linux memsize in MB, %lu\n",
		      (ulong)(gd->ram_size >> 20));
	}

	rd_start = (ulong)map_sysmem(images->initrd_start, 0);
	rd_size = images->initrd_end - images->initrd_start;

	linux_env_init();

	linux_env_set("memsize", env_buf);

	sprintf(env_buf, "0x%08lX", rd_start);
	linux_env_set("initrd_start", env_buf);

	sprintf(env_buf, "0x%lX", rd_size);
	linux_env_set("initrd_size", env_buf);

	sprintf(env_buf, "0x%08X", (uint) (gd->bd->bi_flashstart));
	linux_env_set("flash_start", env_buf);

	sprintf(env_buf, "0x%X", (uint) (gd->bd->bi_flashsize));
	linux_env_set("flash_size", env_buf);

	cp = env_get("ethaddr");
	if (cp)
		linux_env_set("ethaddr", cp);

	cp = env_get("eth1addr");
	if (cp)
		linux_env_set("eth1addr", cp);
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

static void boot_prep_linux(struct bootm_headers *images)
{
	if (images->ft_len) {
		boot_reloc_fdt(images);
		boot_setup_fdt(images);
	} else {
#ifdef CONFIG_LOONGARCH_BOOT_CMDLINE_LEGACY
			linux_cmdline_legacy(images);
			// linux_cmdline_append(images);
			linux_cmdline_dump();
			linux_env_legacy(images);
#endif
	}
}

static void boot_jump_linux(struct bootm_headers *images)
{
	typedef void __noreturn (*kernel_entry_t)(int, ulong, ulong, ulong);
	kernel_entry_t kernel = (kernel_entry_t)map_to_sysmem((void*)images->ep);
#if defined(CONFIG_LOONGSON_BOOT_FIXUP)
	void *fw_arg2 = NULL, *fw_arg3 = NULL;
	void *fdt = NULL, *bootparam = NULL;
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
	fdt = env_get("fdt_addr");
	if (fdt) {
		fdt = (void *)simple_strtoul(fdt, NULL, 16);
		if (fdt_check_header(fdt)) {
			printf("Warning: invalid device tree. Used linux default dtb\n");
			fdt = NULL;
		}
	}

	bootparam = build_boot_param();
	fw_arg2 = bootparam;
	fw_arg3 = fdt;

	kernel(linux_argc, (ulong)linux_argv, (ulong)fw_arg2,
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
