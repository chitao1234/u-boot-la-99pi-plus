/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __ASM_LA_SYSTEM_H
#define __ASM_LA_SYSTEM_H

#include <asm/loongarch.h>

static __inline__ void
__sti(void)
{
	u32 flags = CSR_CRMD_IE;
	__asm__ __volatile__(
		"csrxchg %[val], %[mask], %[reg]\n\t"
		: [val] "+r" (flags)
		: [mask] "r" (CSR_CRMD_IE), [reg] "i" (LOONGARCH_CSR_CRMD)
		: "memory");
}

static __inline__ void
__cli(void)
{
	u32 flags = 0;
	__asm__ __volatile__(
		"csrxchg %[val], %[mask], %[reg]\n\t"
		: [val] "+r" (flags)
		: [mask] "r" (CSR_CRMD_IE), [reg] "i" (LOONGARCH_CSR_CRMD)
		: "memory");
}

#define __save_and_cli(x)						\
do {									\
	u32 flags = 0;							\
	__asm__ __volatile__(						\
		"csrxchg %[val], %[mask], %[reg]\n\t"			\
		: [val] "+r" (flags)					\
		: [mask] "r" (CSR_CRMD_IE), [reg] "i" (LOONGARCH_CSR_CRMD)	\
		: "memory");						\
} while(0)

#define __restore_flags(flags)						\
	__asm__ __volatile__(						\
		"csrxchg %[val], %[mask], %[reg]\n\t"			\
		: [val] "+r" (flags)					\
		: [mask] "r" (CSR_CRMD_IE), [reg] "i" (LOONGARCH_CSR_CRMD)	\
		: "memory");

// nothing for now
/* For spinlocks etc */
#define local_irq_save(x)	__save_and_cli(x);
#define local_irq_restore(x)	__restore_flags(x);
#define local_irq_disable()	__cli();
#define local_irq_enable()	__sti();


#endif	/* __ASM_LA_SYSTEM_H */
