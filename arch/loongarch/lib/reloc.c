// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <cpu_func.h>
#include <init.h>
#include <asm/sections.h>
#include <linux/bitops.h>
#include <asm/regdef.h>
#include <asm/global_data.h>
#include <elf.h>

#define R_LARCH_NONE		0
#define R_LARCH_32			1
#define R_LARCH_64			2
#define R_LARCH_RELATIVE	3

DECLARE_GLOBAL_DATA_PTR;

int clear_bss(void)
{
	uint8_t *bss_start;
	ulong bss_len;
	long offs;

	offs = gd->reloc_off;

	/* Clear the .bss section */
	bss_start = (uint8_t *)((unsigned long)__bss_start + offs);
	bss_len = (unsigned long)&__bss_end - (unsigned long)__bss_start;
	memset(bss_start, 0, bss_len);

	return 0;
}


/**
 * apply_reloc() - Apply a single relocation
 * @type: the type of reloc (R_LARCH_*)
 * @addr: the address that the reloc should be applied to
 * @off: the relocation offset, ie. number of bytes we're moving U-Boot by
 *
 * Apply a single relocation of type @type at @addr. This function is
 * intentionally simple, and does the bare minimum needed to fixup the
 * relocated U-Boot - in particular, it does not check for overflows.
 */
static void __always_inline apply_reloc(Elf64_Xword type, void *addr, long off, Elf64_Rela *rela)
{
	switch (type) {
	case R_LARCH_32:
		printf("TODO: please implement relocation type: R_LARCH_32\n");
		break;
	case R_LARCH_64:
		printf("TODO: please implement relocation type: R_LARCH_64\n");
		break;
	case R_LARCH_RELATIVE:
		*(uint64_t *)addr += off;
		break;
	default:
		panic("Unhandled reloc type %llu (@ %p)\n", type, rela);
	}
}

/**
 * relocate_code() - Relocate U-Boot, generally from flash to DDR
 * @start_addr_sp: new stack pointer
 * @new_gd: pointer to relocated global data
 * @relocaddr: the address to relocate to
 *
 * Relocate U-Boot from its current location (generally in flash) to a new one
 * (generally in DDR). This function will copy the U-Boot binary & apply
 * relocations as necessary, then jump to board_init_r in the new build of
 * U-Boot. As such, this function does not return.
 */
void relocate_code(ulong start_addr_sp, gd_t *new_gd, ulong relocaddr)
{
	unsigned long offs;
	ulong addr, length;
	Elf64_Xword type;
	Elf64_Rela *rela_dyn;

	offs = new_gd->reloc_off;

	if (offs) {
		/* Copy U-Boot to RAM */
		length = __image_copy_end - __text_start;
		memcpy((void *)relocaddr, __text_start, length);

		/* Now apply relocations to the copy in RAM */
		for (rela_dyn = (Elf64_Rela *)&__rel_dyn_start;
				rela_dyn < (Elf64_Rela *)&__rel_dyn_end; ++rela_dyn) {
			type = rela_dyn->r_info;
			addr = relocaddr + (ulong)rela_dyn->r_offset - (ulong)__text_start;
			apply_reloc(type, (void *)addr, offs, rela_dyn);
		}

		/* Ensure the icache is coherent */
		flush_cache(relocaddr, length);
	}

	/* Jump to the relocated U-Boot */
	asm volatile(
		"ori	$sp, %0, 0\n"
		"ori	$a0, %1, 0\n"
		"ori	$a1, %2, 0\n"
		"ori	$ra, $zero, 0\n"
		"jirl	$zero, %3, 0\n"
		: /* no outputs */
		: "r"(start_addr_sp),
		  "r"(new_gd),
		  "r"(relocaddr),
		  "r"((unsigned long)board_init_r + offs));

	/* Since we jumped to the new U-Boot above, we won't get here */
	unreachable();
}
