/* SPDX-License-Identifier: GPL-2.0+ */

OUTPUT_ARCH("loongarch")
ENTRY(_start)

SECTIONS
{
	. = ALIGN(4);
	.text : {
		__image_copy_start = .;
		__text_start = .;
		arch/loongarch/cpu/start.o	(.text*)
		*(.text*)
		__text_end = .;
	}

	. = ALIGN(4);
	.rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }

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
		*(.got*)
		__got_end = .;
	}

	. = ALIGN(4);
	.u_boot_list : {
		KEEP(*(SORT(__u_boot_list*)));
	}

	. = ALIGN(4);
	.plt : { *(.plt*) }

	. = ALIGN(4);
	__image_copy_end = .;
	__init_end = .;

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
	_image_binary_end = .;
	_end = .;

	.bss __rel_dyn_start (OVERLAY) : {
		__bss_start = .;
		*(.sbss.*)
		*(.dynbss*)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	}

	/DISCARD/ : { *(.debug*) }
	/DISCARD/ : { *(.dynamic*) }
	/DISCARD/ : { *(.interp*) }
}
