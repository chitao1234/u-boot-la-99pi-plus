/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2002-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef	__ASM_LA_GBL_DATA_H
#define __ASM_LA_GBL_DATA_H

#include <asm/types.h>
#include <asm/cache.h>

/* Architecture-specific global data */
struct arch_global_data {
	u32 cpuprid;
#ifdef CONFIG_SYS_CACHE_SIZE_AUTO
	struct cache_desc	icache; /* Primary I-cache */
	struct cache_desc	dcache; /* Primary D or combined I/D cache */
	struct cache_desc	vcache; /* Victim cache, between pcache and scache */
	struct cache_desc	scache; /* Secondary cache */
	struct cache_desc	tcache; /* Tertiary/split secondary cache */
#endif
	unsigned long timer_freq;
};

#include <asm-generic/global_data.h>

#define DECLARE_GLOBAL_DATA_PTR     register volatile gd_t *gd asm ("$r21")

#endif /* __ASM_LA_GBL_DATA_H */
