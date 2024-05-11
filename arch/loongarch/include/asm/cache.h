/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2011 The Chromium OS Authors.
 */

#ifndef __ASM_LA_CACHE_H
#define __ASM_LA_CACHE_H

/*
 * Descriptor for a cache
 */
struct cache_desc {
	unsigned int waysize;	/* Bytes per way */
	unsigned short sets;	/* Number of lines per set */
	unsigned char ways;	/* Number of ways */
	unsigned char linesz;	/* Size of line in bytes */
	unsigned char waybit;	/* Bits to select in a cache set */
	unsigned char flags;	/* Flags describing cache properties */
};

#define L1_CACHE_SHIFT		CONFIG_LOONGARCH_L1_CACHE_SHIFT
#define L1_CACHE_BYTES		(1 << L1_CACHE_SHIFT)

#define ARCH_DMA_MINALIGN	(L1_CACHE_BYTES)

/*
 * CONFIG_SYS_CACHELINE_SIZE is still used in various drivers primarily for
 * DMA buffer alignment. Satisfy those drivers by providing it as a synonym
 * of ARCH_DMA_MINALIGN for now.
 */
#define CONFIG_SYS_CACHELINE_SIZE ARCH_DMA_MINALIGN

void loongarch_cache_probe(void);
unsigned long icache_line_size(void);
unsigned long dcache_line_size(void);
unsigned long vcache_line_size(void);
unsigned long scache_line_size(void);

#endif /* __ASM_LA_CACHE_H */
