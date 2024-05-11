/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __ASM_LA_REGDEF_H
#define __ASM_LA_REGDEF_H

#define zero    $r0     /* wired zero */
#define ra      $r1     /* return address */
#define tp      $r2     /* TLS */
#define sp      $r3     /* stack pointer */
#define v0      $r4     /* return value - caller saved */
#define v1      $r5
#define a0      $r4     /* argument registers */
#define a1      $r5
#define a2      $r6
#define a3      $r7
#define a4      $r8     /* arg reg 64 bit; caller saved in 32 bit */
#define a5      $r9
#define a6      $r10
#define a7      $r11
#define t0      $r12    /* caller saved */
#define t1      $r13
#define t2      $r14
#define t3      $r15
#define t4      $r16    /* callee saved */
#define t5      $r17
#define t6      $r18
#define t7      $r19
#define t8      $r20    /* caller saved */
#define gp      $r21    /* global pointer */
#define fp      $r22    /* frame pointer */
#define s0      $r23    /* callee saved */
#define s1      $r24
#define s2      $r25
#define s3      $r26
#define s4      $r27
#define s5      $r28
#define s6      $r29
#define s7      $r30
#define s8      $r31    /* callee saved */


#define STAND_ARG_SIZE		16
#define STAND_FRAME_SIZE	24
#define STAND_RA_OFFSET		20  

/*
 * Location of the saved registers relative to ZERO.
 * Usage is p->p_regs[XX].
 */
#define ZERO   0
#define RA     1
#define GP     2
#define SP     3
#define A0     4
#define A1     5
#define A2     6
#define A3     7
#define A4     8
#define A5     9
#define A6     10
#define A7     11
#define T0     12
#define T1     13
#define T2     14
#define T3     15
#define T4     16
#define T5     17
#define T6     18
#define T7     19
#define T8     20
#define TP     21
#define FP     22
#define S0     23
#define S1     24
#define S2     25
#define S3     26
#define S4     27
#define S5     28
#define S6     29
#define S7     30
#define S8     31

#define BASE_NUM 32
#define LA_CSR_CRMD  0
#define LA_CSR_PRMD  1
#define LA_CSR_ECTL  4
#define LA_CSR_ESTAT 5
#define LA_CSR_EPC   6
#define LA_CSR_BADV  7
#define LA_CSR_BADI  8


#define	NUMSAVEREGS 38		/* Number of registers saved in trap */

#define FPBASE	(NUMSAVEREGS)
#define F0	(FPBASE+0)
#define F1	(FPBASE+1)
#define F2	(FPBASE+2)
#define F3	(FPBASE+3)
#define F4	(FPBASE+4)
#define F5	(FPBASE+5)
#define F6	(FPBASE+6)
#define F7	(FPBASE+7)
#define F8	(FPBASE+8)
#define F9	(FPBASE+9)
#define F10	(FPBASE+10)
#define F11	(FPBASE+11)
#define F12	(FPBASE+12)
#define F13	(FPBASE+13)
#define F14	(FPBASE+14)
#define F15	(FPBASE+15)
#define F16	(FPBASE+16)
#define F17	(FPBASE+17)
#define F18	(FPBASE+18)
#define F19	(FPBASE+19)
#define F20	(FPBASE+20)
#define F21	(FPBASE+21)
#define F22	(FPBASE+22)
#define F23	(FPBASE+23)
#define F24	(FPBASE+24)
#define F25	(FPBASE+25)
#define F26	(FPBASE+26)
#define F27	(FPBASE+27)
#define F28	(FPBASE+28)
#define F29	(FPBASE+29)
#define F30	(FPBASE+30)
#define F31	(FPBASE+31)
#define	FSR	(FPBASE+32)
#define NUMFPREGS 33

#endif /* __ASM_LA_REGDEF_H */
