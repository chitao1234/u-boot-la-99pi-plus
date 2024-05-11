#include <common.h>
#include <asm/global_data.h>
#include <spl.h>
#include <init.h>
#include <hang.h>
#include <mapmem.h>

DECLARE_GLOBAL_DATA_PTR;

extern void ddr_init(void);
extern ulong spl_relocate_stack_gd(void);

__weak void spl_mach_init(void)
{
	arch_cpu_init();
}

__weak void spl_mach_init_late(void)
{
}

u32 spl_boot_device(void)
{
#if defined(CONFIG_SPL_SPI)
	return BOOT_DEVICE_SPI;
#elif defined(CONFIG_SPL_MMC)
	return BOOT_DEVICE_MMC1;
#elif defined(CONFIG_SPL_NAND_SUPPORT)
	return BOOT_DEVICE_NAND;
#else
	return BOOT_DEVICE_NONE;
#endif
}

__weak int spl_sdram_init(void)
{
	return dram_init();
}

#if defined(CONFIG_SPL_MULTI_DTB_FIT)
__weak int spl_dtb_select(void)
{
	printf("Note: Please implement %s for special machine/board!\n", __func__);
	return 0;
}
#endif

void __noreturn board_init_f(ulong dummy)
{
	int ret;
	ulong new_sp;

#ifdef DBG_ASM
	printstr("Enter board_init_f...\r\n");
#endif
	spl_mach_init();
	ret = spl_early_init();
	if (ret) {
		debug("spl_early_init() failed: %d\n", ret);
#ifdef DBG_ASM
		printstr("spl_early_init() failed.\r\n");
#endif
		hang();
	}

	preloader_console_init();

#if defined(CONFIG_SPL_MULTI_DTB_FIT)
	if (spl_dtb_select()) {
		printf("switch dtb failed\n");
		hang();
	}
#endif

	printf("spl sdram init ...\n");
	if (spl_sdram_init()) {
		printf("spl sdram init failed\n");
		hang();
	}
    readq(PHYS_TO_UNCACHED(0x16002108)) |= 0x80000000;

	// change the sp, gd and malloc to sdram space
	new_sp = spl_relocate_stack_gd();
	if (new_sp > 0) {
		// set new sp addr.
		asm volatile (
			"ori	$sp, %0, 0\n"
			: /* no outputs */
			: "r"(new_sp));
	}

	spl_mach_init_late();
	board_init_r(NULL, 0);
}
