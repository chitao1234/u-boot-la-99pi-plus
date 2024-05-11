/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_LA_IO_H
#define __ASM_LA_IO_H

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/byteorder.h>
#include <asm/barrier.h>
#include <asm/addrspace.h>
#include <mach/mapmem.h>
#include <asm/config.h>

static inline void sync(void)
{
	__sync();
}

#ifdef CONFIG_ARCH_MAP_SYSMEM

static __always_inline void *map_sysmem(phys_addr_t paddr, unsigned long len)
{
	if (VA_TO_PHYS(paddr) >= (unsigned long)BOOT_SPACE_BASE &&
			VA_TO_PHYS(paddr) < (unsigned long)BOOT_SPACE_BASE + (unsigned long)BOOT_SPACE_SIZE)
		return (void *)PHYS_TO_CACHED((unsigned long)paddr);
	return map_sysmem_mach(paddr, len);
}

static inline void unmap_sysmem(const void *vaddr)
{

}

static inline phys_addr_t map_to_sysmem(const void *ptr)
{
	return (phys_addr_t)map_sysmem((unsigned long)ptr, 0);
}
#endif

#ifdef CONFIG_HAVE_ARCH_IOREMAP
static inline void __iomem *ioremap(resource_size_t offset,
				    resource_size_t size)
{
	return (void __iomem *)PHYS_TO_UNCACHED((unsigned long long)offset);
}

static inline void iounmap(void __iomem *addr)
{
}
#endif

#define MAP_NOCACHE		0
#define MAP_WRCOMBINE	1
#define MAP_WRBACK		2
#define MAP_WRTHROUGH	3
static inline void *map_physmem(phys_addr_t paddr, unsigned long len,
				unsigned long flags)
{
	if (flags == MAP_NOCACHE)
		return (void *)PHYS_TO_UNCACHED(paddr);
	else
		return (void *)PHYS_TO_CACHED(paddr);
}
#define map_physmem 	map_physmem

static inline phys_addr_t virt_to_dma(volatile void *address)
{
	return (phys_addr_t)VA_TO_PHYS(address);
}

static inline void *dma_to_virt(unsigned long address)
{
	return (void *)PHYS_TO_CACHED(address);
}

/*
 * Generic virtual read/write.  Note that we don't support half-word
 * read/writes.  We define __arch_*[bl] here, and leave __arch_*w
 * to the architecture specific code.
 */
#define __arch_getb(a)			(*(volatile unsigned char *)(a))
#define __arch_getw(a)			(*(volatile unsigned short *)(a))
#define __arch_getl(a)			(*(volatile unsigned int *)(a))
#define __arch_getq(a)			(*(volatile unsigned long long *)(a))

#define __arch_putb(v,a)		(*(volatile unsigned char *)(a) = (v))
#define __arch_putw(v,a)		(*(volatile unsigned short *)(a) = (v))
#define __arch_putl(v,a)		(*(volatile unsigned int *)(a) = (v))
#define __arch_putq(v,a)		(*(volatile unsigned long long *)(a) = (v))

static inline void __raw_writesb(unsigned long addr, const void *data,
				 int bytelen)
{
	uint8_t *buf = (uint8_t *)data;
	while(bytelen--)
		__arch_putb(*buf++, addr);
}

static inline void __raw_writesw(unsigned long addr, const void *data,
				 int wordlen)
{
	uint16_t *buf = (uint16_t *)data;
	while(wordlen--)
		__arch_putw(*buf++, addr);
}

static inline void __raw_writesl(unsigned long addr, const void *data,
				 int longlen)
{
	uint32_t *buf = (uint32_t *)data;
	while(longlen--)
		__arch_putl(*buf++, addr);
}

static inline void __raw_readsb(unsigned long addr, void *data, int bytelen)
{
	uint8_t *buf = (uint8_t *)data;
	while(bytelen--)
		*buf++ = __arch_getb(addr);
}

static inline void __raw_readsw(unsigned long addr, void *data, int wordlen)
{
	uint16_t *buf = (uint16_t *)data;
	while(wordlen--)
		*buf++ = __arch_getw(addr);
}

static inline void __raw_readsl(unsigned long addr, void *data, int longlen)
{
	uint32_t *buf = (uint32_t *)data;
	while(longlen--)
		*buf++ = __arch_getl(addr);
}

#define __raw_writeb(v,a)	__arch_putb(v,a)
#define __raw_writew(v,a)	__arch_putw(v,a)
#define __raw_writel(v,a)	__arch_putl(v,a)
#define __raw_writeq(v,a)	__arch_putq(v,a)

#define __raw_readb(a)		__arch_getb(a)
#define __raw_readw(a)		__arch_getw(a)
#define __raw_readl(a)		__arch_getl(a)
#define __raw_readq(a)		__arch_getq(a)


#define writeb(v,a)		__arch_putb(v,a)
#define writew(v,a)		__arch_putw(v,a)
#define writel(v,a)		__arch_putl(v,a)
#define writeq(v,a)		__arch_putq(v,a)

#define readb(a)	__arch_getb(a)
#define readw(a)	__arch_getw(a)
#define readl(a)	__arch_getl(a)
#define readq(a)	__arch_getq(a)


#define smp_processor_id()	0

/*
 * Relaxed I/O memory access primitives. These follow the Device memory
 * ordering rules but do not guarantee any ordering relative to Normal memory
 * accesses.
 */
#define readb_relaxed(c)	({ u8  __r = __raw_readb(c); __r; })
#define readw_relaxed(c)	({ u16 __r = le16_to_cpu((__force __le16) \
						__raw_readw(c)); __r; })
#define readl_relaxed(c)	({ u32 __r = le32_to_cpu((__force __le32) \
						__raw_readl(c)); __r; })
#define readq_relaxed(c)	({ u64 __r = le64_to_cpu((__force __le64) \
						__raw_readq(c)); __r; })

#define writeb_relaxed(v, c)	((void)__raw_writeb((v), (c)))
#define writew_relaxed(v, c)	((void)__raw_writew((__force u16) \
						    cpu_to_le16(v), (c)))
#define writel_relaxed(v, c)	((void)__raw_writel((__force u32) \
						    cpu_to_le32(v), (c)))
#define writeq_relaxed(v, c)	((void)__raw_writeq((__force u64) \
						    cpu_to_le64(v), (c)))

/*
 * The compiler seems to be incapable of optimising constants
 * properly.  Spell it out to the compiler in some cases.
 * These are only valid for small values of "off" (< 1<<12)
 */
#define __raw_base_writeb(val,base,off)	__arch_base_putb(val,base,off)
#define __raw_base_writew(val,base,off)	__arch_base_putw(val,base,off)
#define __raw_base_writel(val,base,off)	__arch_base_putl(val,base,off)

#define __raw_base_readb(base,off)	__arch_base_getb(base,off)
#define __raw_base_readw(base,off)	__arch_base_getw(base,off)
#define __raw_base_readl(base,off)	__arch_base_getl(base,off)

/*
 * Clear and set bits in one shot. These macros can be used to clear and
 * set multiple bits in a register using a single call. These macros can
 * also be used to set a multiple-bit bit pattern using a mask, by
 * specifying the mask in the 'clear' parameter and the new bit pattern
 * in the 'set' parameter.
 */

#define out_arch(type,endian,a,v)	__raw_write##type(cpu_to_##endian(v),a)
#define in_arch(type,endian,a)		endian##_to_cpu(__raw_read##type(a))

#define out_le64(a,v)	out_arch(q,le64,a,v)
#define out_le32(a,v)	out_arch(l,le32,a,v)
#define out_le16(a,v)	out_arch(w,le16,a,v)

#define in_le64(a)	in_arch(q,le64,a)
#define in_le32(a)	in_arch(l,le32,a)
#define in_le16(a)	in_arch(w,le16,a)

#define out_be64(a,v)	out_arch(l,be64,a,v)
#define out_be32(a,v)	out_arch(l,be32,a,v)
#define out_be16(a,v)	out_arch(w,be16,a,v)

#define in_be64(a)	in_arch(l,be64,a)
#define in_be32(a)	in_arch(l,be32,a)
#define in_be16(a)	in_arch(w,be16,a)

#define out_64(a,v)	__raw_writeq(v,a)
#define out_32(a,v)	__raw_writel(v,a)
#define out_16(a,v)	__raw_writew(v,a)
#define out_8(a,v)	__raw_writeb(v,a)

#define in_64(a)	__raw_readq(a)
#define in_32(a)	__raw_readl(a)
#define in_16(a)	__raw_readw(a)
#define in_8(a)		__raw_readb(a)

#define clrbits(type, addr, clear) \
	out_##type((addr), in_##type(addr) & ~(clear))

#define setbits(type, addr, set) \
	out_##type((addr), in_##type(addr) | (set))

#define clrsetbits(type, addr, clear, set) \
	out_##type((addr), (in_##type(addr) & ~(clear)) | (set))

#define clrbits_be32(addr, clear) clrbits(be32, addr, clear)
#define setbits_be32(addr, set) setbits(be32, addr, set)
#define clrsetbits_be32(addr, clear, set) clrsetbits(be32, addr, clear, set)

#define clrbits_le32(addr, clear) clrbits(le32, addr, clear)
#define setbits_le32(addr, set) setbits(le32, addr, set)
#define clrsetbits_le32(addr, clear, set) clrsetbits(le32, addr, clear, set)

#define clrbits_32(addr, clear) clrbits(32, addr, clear)
#define setbits_32(addr, set) setbits(32, addr, set)
#define clrsetbits_32(addr, clear, set) clrsetbits(32, addr, clear, set)

#define clrbits_be16(addr, clear) clrbits(be16, addr, clear)
#define setbits_be16(addr, set) setbits(be16, addr, set)
#define clrsetbits_be16(addr, clear, set) clrsetbits(be16, addr, clear, set)

#define clrbits_le16(addr, clear) clrbits(le16, addr, clear)
#define setbits_le16(addr, set) setbits(le16, addr, set)
#define clrsetbits_le16(addr, clear, set) clrsetbits(le16, addr, clear, set)

#define clrbits_16(addr, clear) clrbits(16, addr, clear)
#define setbits_16(addr, set) setbits(16, addr, set)
#define clrsetbits_16(addr, clear, set) clrsetbits(16, addr, clear, set)

#define clrbits_8(addr, clear) clrbits(8, addr, clear)
#define setbits_8(addr, set) setbits(8, addr, set)
#define clrsetbits_8(addr, clear, set) clrsetbits(8, addr, clear, set)

#define clrbits_be64(addr, clear) clrbits(be64, addr, clear)
#define setbits_be64(addr, set) setbits(be64, addr, set)
#define clrsetbits_be64(addr, clear, set) clrsetbits(be64, addr, clear, set)

#define clrbits_le64(addr, clear) clrbits(le64, addr, clear)
#define setbits_le64(addr, set) setbits(le64, addr, set)
#define clrsetbits_le64(addr, clear, set) clrsetbits(le64, addr, clear, set)

#define clrbits_64(addr, clear) clrbits(64, addr, clear)
#define setbits_64(addr, set) setbits(64, addr, set)
#define clrsetbits_64(addr, clear, set) clrsetbits(64, addr, clear, set)



#define outb(v,p)			__raw_writeb(v,p)
#define outw(v,p)			__raw_writew(cpu_to_le16(v),p)
#define outl(v,p)			__raw_writel(cpu_to_le32(v),p)

#define inb(p)	({ unsigned int __v = __raw_readb(p); __v; })
#define inw(p)	({ unsigned int __v = le16_to_cpu(__raw_readw(p)); __v; })
#define inl(p)	({ unsigned int __v = le32_to_cpu(__raw_readl(p)); __v; })

#define outsb(p,d,l)			__raw_writesb(p,d,l)
#define outsw(p,d,l)			__raw_writesw(p,d,l)
#define outsl(p,d,l)			__raw_writesl(p,d,l)

#define insb(p,d,l)			__raw_readsb(p,d,l)
#define insw(p,d,l)			__raw_readsw(p,d,l)
#define insl(p,d,l)			__raw_readsl(p,d,l)

#define outb_p(val,port)		outb((val),(port))
#define outw_p(val,port)		outw((val),(port))
#define outl_p(val,port)		outl((val),(port))
#define inb_p(port)			inb((port))
#define inw_p(port)			inw((port))
#define inl_p(port)			inl((port))

#define outsb_p(port,from,len)		outsb(port,from,len)
#define outsw_p(port,from,len)		outsw(port,from,len)
#define outsl_p(port,from,len)		outsl(port,from,len)
#define insb_p(port,to,len)		insb(port,to,len)
#define insw_p(port,to,len)		insw(port,to,len)
#define insl_p(port,to,len)		insl(port,to,len)

#define writesl(a, d, s)	__raw_writesl((unsigned long)a, d, s)
#define readsl(a, d, s)		__raw_readsl((unsigned long)a, d, s)
#define writesw(a, d, s)	__raw_writesw((unsigned long)a, d, s)
#define readsw(a, d, s)		__raw_readsw((unsigned long)a, d, s)
#define writesb(a, d, s)	__raw_writesb((unsigned long)a, d, s)
#define readsb(a, d, s)		__raw_readsb((unsigned long)a, d, s)

/*
 * DMA-consistent mapping functions.  These allocate/free a region of
 * uncached, unwrite-buffered mapped memory space for use with DMA
 * devices.  This is the "generic" version.  The PCI specific version
 * is in pci.h
 */
extern void *consistent_alloc(int gfp, size_t size, dma_addr_t *handle);
extern void consistent_free(void *vaddr, size_t size, dma_addr_t handle);
extern void consistent_sync(void *vaddr, size_t size, int rw);

/*
 * String version of IO memory access ops:
 */
extern void _memcpy_fromio(void *, unsigned long, size_t);
extern void _memcpy_toio(unsigned long, const void *, size_t);
extern void _memset_io(unsigned long, int, size_t);

extern void __readwrite_bug(const char *fn);

/* Optimized copy functions to read from/write to IO sapce */
#include <cpu_func.h>
/*
 * Copy data from IO memory space to "real" memory space.
 */
static inline
void __memcpy_fromio(void *to, const volatile void __iomem *from, size_t count)
{
	while (count && !IS_ALIGNED((unsigned long)from, 8)) {
		*(u8 *)to = __raw_readb(from);
		from++;
		to++;
		count--;
	}

	while (count >= 8) {
		*(u64 *)to = __raw_readq(from);
		from += 8;
		to += 8;
		count -= 8;
	}

	while (count) {
		*(u8 *)to = __raw_readb(from);
		from++;
		to++;
		count--;
	}
}

/*
 * Copy data from "real" memory space to IO memory space.
 */
static inline
void __memcpy_toio(volatile void __iomem *to, const void *from, size_t count)
{
	while (count && !IS_ALIGNED((unsigned long)to, 8)) {
		__raw_writeb(*(u8 *)from, to);
		from++;
		to++;
		count--;
	}

	while (count >= 8) {
		__raw_writeq(*(u64 *)from, to);
		from += 8;
		to += 8;
		count -= 8;
	}

	while (count) {
		__raw_writeb(*(u8 *)from, to);
		from++;
		to++;
		count--;
	}
}

/*
 * "memset" on IO memory space.
 */
static inline
void __memset_io(volatile void __iomem *dst, int c, size_t count)
{
	u64 qc = (u8)c;

	qc |= qc << 8;
	qc |= qc << 16;
	qc |= qc << 32;

	while (count && !IS_ALIGNED((unsigned long)dst, 8)) {
		__raw_writeb(c, dst);
		dst++;
		count--;
	}

	while (count >= 8) {
		__raw_writeq(qc, dst);
		dst += 8;
		count -= 8;
	}

	while (count) {
		__raw_writeb(c, dst);
		dst++;
		count--;
	}
}


#define memset_io(a, b, c)			__memset_io((a), (b), (c))
#define memcpy_fromio(a, b, c)		__memcpy_fromio((a), (b), (c))
#define memcpy_toio(a, b, c)		__memcpy_toio((a), (b), (c))

// #define memset_io(a, b, c)			memset((void *)(a), (b), (c))
// #define memcpy_fromio(a, b, c)		memcpy((a), (void *)(b), (c))
// #define memcpy_toio(a, b, c)		memcpy((void *)(a), (b), (c))


/*
 * If this architecture has ISA IO, then define the isa_read/isa_write
 * macros.
 */
#ifdef __mem_isa
#define isa_readb(addr)			__raw_readb(__mem_isa(addr))
#define isa_readw(addr)			__raw_readw(__mem_isa(addr))
#define isa_readl(addr)			__raw_readl(__mem_isa(addr))
#define isa_writeb(val,addr)		__raw_writeb(val,__mem_isa(addr))
#define isa_writew(val,addr)		__raw_writew(val,__mem_isa(addr))
#define isa_writel(val,addr)		__raw_writel(val,__mem_isa(addr))
#define isa_memset_io(a,b,c)		_memset_io(__mem_isa(a),(b),(c))
#define isa_memcpy_fromio(a,b,c)	_memcpy_fromio((a),__mem_isa(b),(c))
#define isa_memcpy_toio(a,b,c)		_memcpy_toio(__mem_isa((a)),(b),(c))

#define isa_eth_io_copy_and_sum(a,b,c,d) \
				eth_copy_and_sum((a),__mem_isa(b),(c),(d))

static inline int
isa_check_signature(unsigned long io_addr, const unsigned char *signature,
		    int length)
{
	int retval = 0;
	do {
		if (isa_readb(io_addr) != *signature)
			goto out;
		io_addr++;
		signature++;
		length--;
	} while (length);
	retval = 1;
out:
	return retval;
}

#else	/* __mem_isa */

#define isa_readb(addr)			(__readwrite_bug("isa_readb"),0)
#define isa_readw(addr)			(__readwrite_bug("isa_readw"),0)
#define isa_readl(addr)			(__readwrite_bug("isa_readl"),0)
#define isa_writeb(val,addr)		__readwrite_bug("isa_writeb")
#define isa_writew(val,addr)		__readwrite_bug("isa_writew")
#define isa_writel(val,addr)		__readwrite_bug("isa_writel")
#define isa_memset_io(a,b,c)		__readwrite_bug("isa_memset_io")
#define isa_memcpy_fromio(a,b,c)	__readwrite_bug("isa_memcpy_fromio")
#define isa_memcpy_toio(a,b,c)		__readwrite_bug("isa_memcpy_toio")

#define isa_eth_io_copy_and_sum(a,b,c,d) \
				__readwrite_bug("isa_eth_io_copy_and_sum")

#define isa_check_signature(io,sig,len)	(0)

#endif	/* __mem_isa */
#endif	/* __KERNEL__ */

#include <asm-generic/io.h>
#include <iotrace.h>

#endif	/* __ASM_LA_IO_H */
