/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 1994, 1995, 1996, 1999 by Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#ifndef _ASM_TYPES_H
#define _ASM_TYPES_H

#include <asm-generic/int-ll64.h>

#ifndef __ASSEMBLY__

typedef unsigned short umode_t;

#endif /* __ASSEMBLY__ */

/*
 * These aren't exported outside the kernel to avoid name space clashes
 */
#ifdef __KERNEL__

#ifdef CONFIG_64BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif

#ifndef __ASSEMBLY__

#if defined(CONFIG_64BIT)
typedef u64 phys_addr_t;
typedef u64 phys_size_t;
#else
typedef u32 phys_addr_t;
typedef u32 phys_size_t;
#endif

#ifdef CONFIG_DMA_ADDR_T_64BIT
typedef u64 dma_addr_t;
#else
typedef u32 dma_addr_t;
#endif

typedef u64 dma64_addr_t;

/*
 * Don't use phys_t.  You've been warned.
 */
#ifdef CONFIG_64BIT_PHYS_ADDR
typedef unsigned long long phys_t;
#else
typedef unsigned long phys_t;
#endif

#endif /* __ASSEMBLY__ */

#endif /* __KERNEL__ */

#endif /* _ASM_TYPES_H */
