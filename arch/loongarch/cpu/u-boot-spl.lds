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

	/* These mark the ABI of U-Boot for debuggers. */
	.mdebug.abi32 : {
		KEEP(*(.mdebug.abi32))
	}
	.mdebug.abi64 : {
		KEEP(*(.mdebug.abi64))
	}

	/* Stabs debugging sections.  */
	.stab 0 : { *(.stab) }
	.stabstr 0 : { *(.stabstr) }
	.stab.excl 0 : { *(.stab.excl) }
	.stab.exclstr 0 : { *(.stab.exclstr) }
	.stab.index 0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment 0 : { *(.comment) }

	/*
	 * DWARF debug sections.
	 * Symbols in the DWARF debugging sections are relative to
	 * the beginning of the section so we begin them at 0.
	 */
	/* DWARF 1 */
	.debug 0 : { *(.debug) }
	.line 0 : { *(.line) }
	/* GNU DWARF 1 extensions */
	.debug_srcinfo 0 : { *(.debug_srcinfo) }
	.debug_sfnames 0 : { *(.debug_sfnames) }
	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges 0 : { *(.debug_aranges) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	/* DWARF 2 */
	.debug_info 0 : {
		*(.debug_info
		.gnu.linkonce.wi.*)
	}
	.debug_abbrev 0 : { *(.debug_abbrev) }
	.debug_line 0 : { *(.debug_line) }
	.debug_frame 0 : { *(.debug_frame) }
	.debug_str 0 : { *(.debug_str) }
	.debug_loc 0 : { *(.debug_loc) }
	.debug_macinfo 0 : { *(.debug_macinfo) }
	.debug_pubtypes 0 : { *(.debug_pubtypes) }
	/* DWARF 3 */
	.debug_ranges 0 : { *(.debug_ranges) }
	/* SGI/MIPS DWARF 2 extensions */
	.debug_weaknames 0 : { *(.debug_weaknames) }
	.debug_funcnames 0 : { *(.debug_funcnames) }
	.debug_typenames 0 : { *(.debug_typenames) }
	.debug_varnames 0 : { *(.debug_varnames) }
	/* GNU DWARF 2 extensions */
	.debug_gnu_pubnames 0 : { *(.debug_gnu_pubnames) }
	.debug_gnu_pubtypes 0 : { *(.debug_gnu_pubtypes) }
	/* DWARF 4 */
	.debug_types 0 : { *(.debug_types) }
	/* DWARF 5 */
	.debug_macro 0 : { *(.debug_macro) }
	.debug_addr 0 : { *(.debug_addr) }

	/DISCARD/ : {
		/* ABI crap starts here */
		*(.LOONGARCH.options)
		*(.options)
		*(.eh_frame)
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
