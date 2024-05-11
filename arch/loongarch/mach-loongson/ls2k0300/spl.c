#include <common.h>
#include <init.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <mach/loongson.h>

void spl_mach_init(void)
{
	arch_cpu_init();
}

void spl_mach_init_late(void)
{
	unsigned long unlock_base = LOCK_CACHE_BASE;

    // unlock scache
	// the scache locked in lowlevel_init.S is used by stack for early stage.
	// now our stack sp is in sdram, so unlock the scache.
	writeq(0x0, LS_SCACHE_LOCK_WIN0_MASK);
	writeq(0x0, LS_SCACHE_LOCK_WIN0_BASE);
#if 1	
	printf("flush locked scache........\n");
	/* flush scache using hit-invalidate */
	for (int i = 0; i < LOCK_CACHE_SIZE/2; i += 0x40) {
		asm(
			"cacop 0x13, %0, 0\n\t"
			:
			: "r"(unlock_base)
		);
		unlock_base += 0x40;
	}
#endif
}
