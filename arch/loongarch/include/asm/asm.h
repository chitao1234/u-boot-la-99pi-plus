/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 wangjianhui <wangjianhui@loongson.cn>
 */

#ifndef __ASM_LA_ASM_H
#define __ASM_LA_ASM_H

#include <asm/regdef.h>

#ifndef ABICALLS
#define	ABICALLS	.abicalls
#endif

#if defined(ABICALLS) && !defined(_KERNEL)
	ABICALLS
#endif

#define RCSID(x)

/*
 * Code for setting gp reg if abicalls are used.
 */
#define	ABISETUP

/*
 * Define -pg profile entry code.
 * if used -pg to debug should complete this macro
 */
#define	MCOUNT


/*
 * EXPORT - export definition of symbol
 */
#define EXPORT(symbol)					\
		.globl	symbol;				\
symbol:

/*
 * ABS - export absolute symbol
 */
#define ABS(symbol,value)				\
		.globl	symbol;				\
symbol		=	value


/*
 * LEAF(x)
 *
 *	Declare a leaf routine.
 */
#define LEAF(x)			\
	.align	3;		\
	.globl x;		\
x: ;				\
	ABISETUP		\
	MCOUNT

#define	ALEAF(x)		\
	.globl	x;		\
x:

/*
 * NLEAF(x)
 *
 *	Declare a non-profiled leaf routine.
 */
#define NLEAF(x)		\
	.align	3;		\
	.globl x;		\
x: ;				\
	ABISETUP

/*
 * NON_LEAF(x)
 *
 *	Declare a non-leaf routine (a routine that makes other C calls).
 */
#define NON_LEAF(x, fsize, retpc) \
	.align	3;		\
	.globl x;		\
x: ;				\
	ABISETUP		\
	MCOUNT

/*
 * NNON_LEAF(x)
 *
 *	Declare a non-profiled non-leaf routine
 *	(a routine that makes other C calls).
 */
#define NNON_LEAF(x, fsize, retpc) \
	.align	3;		\
	.globl x;		\
x: ;				\
	ABISETUP

/*
 * END(x)
 *
 *	Mark end of a procedure.
 */
#define END(x) \
	//end x

/*
 * Macros to panic and printf from assembly language.
 */

#define	MSG(msg) \
	.section .rodata; \
9:	.asciiz	msg; \
	.text

#define ASMSTR(str) \
	.asciiz str; \
	.align	3

#endif /* __ASM_LA_ASM_H */
