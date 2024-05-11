/* SPDX-License-Identifier: GPL-2.0+ */

OUTPUT_ARCH("loongarch")
ENTRY(_start)

SECTIONS
{
	. = ALIGN(4);
	.text : {
		__text_start = .;
		__image_copy_start = .;
		arch/loongarch/cpu/start.o	(.text)
		*(.text*)
	}

	. = ALIGN(4);
	.rodata : {
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
	}

	. = ALIGN(4);
	.data : {
		*(.data*)
	}

	. = ALIGN(4);
	.sdata : {
		*(.sdata*)
	}

	. = ALIGN(4);
	.got : {
		__got_start = .;
		*(.got.plt) *(.got)
		__got_end = .;
	}

	. = ALIGN(4);
	.u_boot_list : {
		KEEP(*(SORT(__u_boot_list*)));
	}

	. = ALIGN(4);
	.binman_sym_table : {
		__binman_sym_start = .;
		KEEP(*(SORT(.binman_sym*)));
		__binman_sym_end = .;
	}

	. = ALIGN(4);
	.plt : { *(.plt*) }

	. = ALIGN(4);
	.rela.dyn : {
		__rel_dyn_start = .;
		*(.rela*)
		__rel_dyn_end = .;
	}

	. = ALIGN(4);
	.dynsym : {
		__dyn_sym_start = .;
		*(.dynsym)
		__dyn_sym_end = .;
	}

	. = ALIGN(4);
	__image_copy_end = .;
	_image_binary_end = .;
	_end = .;

	.bss : {
		__bss_start = .;
		*(.sbss.*)
		*(.dynbss*)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	}
}

#if defined(IMAGE_MAX_SIZE)
ASSERT(__image_copy_end - __text_start <= (IMAGE_MAX_SIZE), \
	"SPL image too big");
#endif

#if defined(CONFIG_SPL_BSS_MAX_SIZE)
ASSERT(__bss_end - __bss_start <= (CONFIG_SPL_BSS_MAX_SIZE), \
	"SPL image BSS too big");
#endif
