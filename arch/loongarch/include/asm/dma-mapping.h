/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __ASM_LA_DMA_MAPPING_H
#define __ASM_LA_DMA_MAPPING_H

#include <common.h>
#include <linux/types.h>
#include <asm/cache.h>
#include <cpu_func.h>
#include <linux/dma-direction.h>
#include <malloc.h>

static inline void *dma_alloc_coherent(size_t len, unsigned long *handle)
{
	*handle = (unsigned long)memalign(ARCH_DMA_MINALIGN, len);
	return (void *)*handle;
}

static inline void dma_free_coherent(void *addr)
{
	free(addr);
}

#endif /* __ASM_LA_DMA_MAPPING_H */
