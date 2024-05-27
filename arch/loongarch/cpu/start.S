/*
 * Copyright (c) 2021 Loongson Technology Corporation Limited (www.loongson.cn)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Opsycon AB, Sweden.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _KERNEL
#define _KERNEL
#endif

#include <asm-offsets.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/loongarch.h>
#include <asm/addrspace.h>
#include <config.h>
#include <mach/loongson.h>

#define PRINT_EXCEPTION_PROMPT	\
		li.w	a0, 'e';	\
		bl	printchar;	\
		li.w	a0, 'x';	\
		bl	printchar;	\
		li.w	a0, 'c';	\
		bl	printchar;	\
		li.w	a0, 'e';	\
		bl	printchar;	\
		li.w	a0, 'p';	\
		bl	printchar;	\
		li.w	a0, 't';	\
		bl	printchar;	\
		li.w	a0, 'i';	\
		bl	printchar;	\
		li.w	a0, 'o';	\
		bl	printchar;	\
		li.w	a0, 'n';	\
		bl	printchar;	\
		li.w	a0, '!';	\
		bl	printchar;	\
		li.w	a0, '\r';	\
		bl	printchar;	\
		li.w	a0, '\n';	\
		bl	printchar;

#define PRINT_CPU_EXCEPTION_PROMPT	\
		li.w	a0, 'C';	\
		bl	printchar;	\
		li.w	a0, 'P';	\
		bl	printchar;	\
		li.w	a0, 'U';	\
		bl	printchar;	\
		csrrd   a0, LOONGARCH_CSR_CPUNUM;	\
		andi    a0, a0, 0x1ff;	\
		addi.w	a0, a0, '0';	\
		bl	printchar;	\
		li.w	a0, ' ';	\
		bl	printchar;	\
		PRINT_EXCEPTION_PROMPT


#define PRINT_TLB_EXCEPTION_PROMPT	\
		li.w	a0, 'T';	\
		bl	printchar;	\
		li.w	a0, 'L';	\
		bl	printchar;	\
		li.w	a0, 'B';	\
		bl	printchar;	\
		li.w	a0, ' ';	\
		bl	printchar;	\
		PRINT_EXCEPTION_PROMPT


#define PRINT_CSR(offset)	\
		li.w	a0, '\r';	\
		bl	printchar;	\
		li.w	a0, '\n';	\
		bl	printchar;	\
		li.w	a0, 'c';	\
		bl	printchar;	\
		li.w	a0, 's';	\
		bl	printchar;	\
		li.w	a0, 'r';	\
		bl	printchar;	\
		li.w	a0, ' ';	\
		bl	printchar;	\
		li.w	a0, offset;	\
		bl	printhex;	\
		li.w	a0, ' ';	\
		bl	printchar;	\
		li.w	a0, '-';	\
		bl	printchar;	\
		li.w	a0, '>';	\
		bl	printchar;	\
		li.w	a0, ' ';	\
		bl	printchar;	\
		csrrd	a0, offset;	\
		bl	printhex64;	\
		li.w	a0, '\r';	\
		bl	printchar;	\
		li.w	a0, '\n';	\
		bl	printchar;	\

/*
 * Register usage:
 *
 * tp($r2)		link versus load offset, used to relocate absolute adresses.
 * gp($r21) 	global data pointer
 */
 #define relc_reg	tp

	.globl	_start
	.globl	start
/*
* scripts/Makefile.spl
* KBUILD_CPPFLAGS += -DCONFIG_SPL_BUILD
*/
_start:
start:
#if !defined(CONFIG_SPL) || defined(CONFIG_SPL_BUILD)
	li.d	t0, (1 << (24 + 32))	//set CPUCFG 0 bit24
	csrxchg	t0, t0, LOONGARCH_CSR_PRID

	li.d	t0, (0xa << (16 + 32))	//set CPUCFG 0x13 bit16-23
	li.d	t1, (0xff << (16 + 32))
	csrxchg	t0, t1, LOONGARCH_CSR_MCSR9

	/* enable perf counter as cp0 counter */
	li.w	t0, (0x1 << 16)
	csrxchg t0, t0, LOONGARCH_CSR_PERFCTRL0

	/* open auto flush SFB */
	li.w	t0, (0x1 << 9)
	csrxchg t0, t0, LOONGARCH_CSR_IMPCTL1

	/* fast_ldq_dis close */
	li.w	t0, (0x1 << 12)
	csrxchg zero, t0, LOONGARCH_CSR_IMPCTL1

	/* low power setting */
	li.w	t0, 0xe
	csrxchg t0, t0, LOONGARCH_CSR_MCSR24

	/* pll setting */
	li.d	t0, 0x0001000107270e00
	csrwr	t0, LOONGARCH_CSR_MCSR2

/*
* set DMW0(0x180) to uncached
* set DMW1(0x181) to cached
*/
	li.d	t0, UNCACHED_MEMORY_ADDR | 0xf
	csrwr	t0, LOONGARCH_CSR_DMWIN0
	li.d	t0, CACHED_MEMORY_ADDR | 0x1f
	csrwr	t0, LOONGARCH_CSR_DMWIN1


/*
 * should before execution jr shutdown slave core
 * otherwise speculative execution cause error
 */
	/* clear Vint cofigure */
	li.d	t0, (0x7 << 16)
	csrxchg zero, t0, LOONGARCH_CSR_ECFG
	/* set ebase address */
	li.d	t0, PHYS_TO_CACHED(BOOT_SPACE_BASE + 0x1000)
	csrwr	t0, LOONGARCH_CSR_EBASE
	/* set TLB excption address */
	li.d	t0, BOOT_SPACE_BASE + 0x1000
	csrwr	t0, LOONGARCH_CSR_TLBREBASE

	/* enable llexc */
	li.w	t0, (1 << 3)
	csrxchg zero, t0, LOONGARCH_CSR_MCSR1

	/* disable interrupt */
	li.d	t0, (1 << 2)
	csrxchg zero, t0, LOONGARCH_CSR_CRMD

	bl	lowlevel_init
	nop

	/* don`t change this code, jumping to cached address */
	li.d	t1, CACHED_MEMORY_ADDR
	bl	1f
1:
	/*
	* 执行一条指令pc值加4，从addi.d 到 jirl 刚好3条指令, 所以此处加12。
	*/
	addi.d	t0, ra, 12
	or	t0, t1, t0
	jirl	zero, t0, 0
	/* now pc run to 0x90xxxxxxxxxxxxxx */
	/* DA disable for 0x80xxxxxxxxxxxxxx and 0x90xxxxxxxxxxxx address can be used */
	li.w	t0, 0xb0
	csrwr	t0, LOONGARCH_CSR_CRMD

	/* calculate ASM stage print function relc_reg address */
	la	relc_reg, start
	li.d	a0, BOOT_SPACE_BASE_UNCACHED

	/* if change locked cache address may need change the following code */
	sub.d	relc_reg, relc_reg, a0

	bl	init_serial
	nop

#ifdef DBG_ASM
	PRINTSTR("\r\nLoongArch Initializing ...\r\n")
#endif
	dbar 0
	ibar 0
	nop
#else
	// now running in ram from spl.
	/* clear Vint cofigure */
	la	t1, _start
	li.d	t0, (0x7 << 16)
	csrxchg zero, t0, LOONGARCH_CSR_ECFG
	/* set ebase address */
	li.d	t2, 0x1000
	add.d	t0, t1, t2
	csrwr	t0, LOONGARCH_CSR_EBASE
	/* set TLB excption address */
	li.d	t0, TO_PHYS_MASK
	and	t0, t1, t0
	li.d	t2, 0x1000
	add.d	t0, t1, t2
	csrwr	t0, LOONGARCH_CSR_TLBREBASE

	li.w	relc_reg, 0

#ifdef DBG_ASM
	PRINTSTR("\r\nU-boot start ...\r\n");
#endif
	b	locate
#endif

locate:
#if defined(CONFIG_SPL) && !defined(CONFIG_SPL_BUILD)
  #ifdef CONFIG_SYS_INIT_SP_ADDR
	li.d	a0, CONFIG_SYS_INIT_SP_ADDR
  #else
	li.d	a0, PHYS_TO_CACHED(MEM_WIN_BASE + 0x2000000)
  #endif
#else
#ifdef DBG_ASM
	PRINTSTR("\r\nRAM(Cache AS RAM) Initializing ...\r\n")
#endif	
	bl		ram_init
	// now the ram top addr is in reg a0
#endif

	// jump to cache
	la	t0, jump_cache
	jirl	zero, t0, 0
jump_cache:

	/*
	* alloc memory to gd ptr
	*/
	or		sp, a0, zero
	bl		board_init_f_alloc_reserve
	bl		board_init_f_init_reserve

#ifdef CONFIG_DEBUG_UART
	la		t8, debug_uart_init
	jirl	ra, t8, 0
#endif

	// jump to uboot board_init_f.
#ifdef DBG_ASM
	PRINTSTR("\r\nJump to board_init_f...\r\n");
#endif
#if defined(CONFIG_SPL) && !defined(CONFIG_SPL_BUILD)
	li.d  	a0, 0x00800		// GD_FLG_SKIP_RELOC
#else
	li.d  	a0, 0
#endif
	la		t8, board_init_f
	jirl	ra, t8, 0

#ifdef CONFIG_SPL
	// jump to board_init_r
#ifdef DBG_ASM
	PRINTSTR("\r\nJump to board_init_r....\r\n");
#endif
	ld.d	sp, gp, GD_START_ADDR_SP
	ld.d	a0, gp, GD_NEW_GD
	li.d  	a1, 0
	la		t8, board_init_r
	jirl	ra, t8, 0
#endif

	// Never get here!
	PRINTSTR("Fatal error! please reset the board!\r\n")
1:
	b 1b


/* all exception entry */
	.org 0x1000
exception_vector:
	csrrd	t0, LOONGARCH_CSR_TLBRERA
	andi	t0, t0, 0x1
	bnez	t0, 2f

	PRINT_CPU_EXCEPTION_PROMPT
	PRINT_CSR(LOONGARCH_CSR_ECFG);
	PRINT_CSR(LOONGARCH_CSR_ESTAT);
	PRINT_CSR(LOONGARCH_CSR_ERA);
	PRINT_CSR(LOONGARCH_CSR_BADV);
	PRINT_CSR(LOONGARCH_CSR_BADI);
1:
	b	1b
2:
	li.d	t1, CACHED_MEMORY_ADDR
	bl	1f
1:
	addi.d	t0, ra, 12
	or	t0, t1, t0
	jirl	zero, t0, 0

	li.d	t0, 0xb0
	csrwr	t0, LOONGARCH_CSR_CRMD
	PRINT_TLB_EXCEPTION_PROMPT
	PRINT_CSR(LOONGARCH_CSR_TLBRBADV);
	PRINT_CSR(LOONGARCH_CSR_TLBRERA);
1:
	b	1b


/******************************************************
 *used: a0~a4, relc_reg
 *通过'\0'来判断字符串结束
 ******************************************************/
LEAF(printstr)
	or	a4, ra, zero
	sub.d	a3, a0, relc_reg
	ld.bu	a0, a3, 0
1:
	beqz	a0, 2f

	bl		printchar

	addi.d	a3, a3, 1
	ld.bu	a0, a3, 0
	b	1b

2:
	ori	ra, a4, 0
	jirl	zero, ra, 0
END(printstr)

LEAF(printhex)
	ori		a4, ra, 0
	ori		a3, a0, 0
	li.d	a5, 8
	li.w	a6, 10

	// print the '0x' prefix
	li.w	a0, 0x30	// '0'
	bl		printchar
	li.w	a0, 0x78	// 'x'
	bl		printchar

1:
	rotri.w a3, a3, 28
	andi	a0, a3, 0xf

	bge		a0, a6, 2f
	addi.w	a0, a0, 0x30
	b		3f
2:
	sub.w	a0, a0, a6
	addi.w	a0, a0, 0x61
3:
	bl		printchar

	addi.d	a5, a5, -1
	bnez	a5, 1b

	ori		ra, a4, 0
	jirl	zero, ra, 0
END(printhex)

LEAF(printhex64)
	ori		a4, ra, 0
	ori		a3, a0, 0
	li.d	a5, 16
	li.w	a6, 10

	// print the '0x' prefix
	li.w	a0, 0x30	// '0'
	bl		printchar
	li.w	a0, 0x78	// 'x'
	bl		printchar

1:
	rotri.d a3, a3, 60
	andi	a0, a3, 0xf

	bge		a0, a6, 2f
	addi.w	a0, a0, 0x30
	b		3f
2:
	sub.w	a0, a0, a6
	addi.w	a0, a0, 0x61
3:
	bl		printchar

	addi.d	a5, a5, -1
	bnez	a5, 1b

	ori		ra, a4, 0
	jirl	zero, ra, 0
END(printhex64)
