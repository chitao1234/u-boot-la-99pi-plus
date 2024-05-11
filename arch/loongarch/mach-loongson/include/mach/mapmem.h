#ifndef __MACH_LS_ADDRSPACE_MAP_H_
#define __MACH_LS_ADDRSPACE_MAP_H_

#ifdef CONFIG_ARCH_MAP_SYSMEM

#include <mach/addrspace.h>

static __always_inline void *map_sysmem_mach(phys_addr_t paddr, unsigned long len)
{
	if (VA_TO_PHYS(paddr) >= (unsigned long)MEM_WIN_BASE &&
			VA_TO_PHYS(paddr) < (unsigned long)MEM_WIN_BASE + (unsigned long)MEM_WIN_SIZE)
		return (void *)PHYS_TO_CACHED((unsigned long)paddr);

	if (VA_TO_PHYS(paddr) >= (unsigned long)HIGH_MEM_WIN_BASE &&
			VA_TO_PHYS(paddr) < (unsigned long)HIGH_MEM_WIN_BASE + (unsigned long)HIGH_MEM_WIN_SIZE)
		return (void *)PHYS_TO_CACHED((unsigned long)paddr);

	return (void *)PHYS_TO_UNCACHED((unsigned long)paddr);
}

#endif

#endif
