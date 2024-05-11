// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, <wd@denx.de>
 */

#include <common.h>
#include <cpu_func.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/loongarch.h>
#include <asm/system.h>
#include <linux/bug.h>

DECLARE_GLOBAL_DATA_PTR;

static unsigned long icache_size;
static unsigned long dcache_size;
static unsigned long vcache_size;
static unsigned long scache_size;

static char *way_string[] = { NULL, "direct mapped", "2-way",
	"3-way", "4-way", "5-way", "6-way", "7-way", "8-way",
	"9-way", "10-way", "11-way", "12-way",
	"13-way", "14-way", "15-way", "16-way",
};

static void probe_pcache(void)
{
	volatile struct cache_desc *icache = &gd->arch.icache;
	volatile struct cache_desc *dcache = &gd->arch.dcache;
	unsigned int lsize, sets, ways;
	unsigned int config;

	config = read_cpucfg(LOONGARCH_CPUCFG17);
	lsize = 1 << ((config & CPUCFG17_L1I_SIZE_M) >> CPUCFG17_L1I_SIZE);
	sets  = 1 << ((config & CPUCFG17_L1I_SETS_M) >> CPUCFG17_L1I_SETS);
	ways  = ((config & CPUCFG17_L1I_WAYS_M) >> CPUCFG17_L1I_WAYS) + 1;

	icache->linesz = lsize;
	icache->sets = sets;
	icache->ways = ways;
	icache_size = sets * ways * lsize;
	icache->waysize = icache_size / icache->ways;

	config = read_cpucfg(LOONGARCH_CPUCFG18);
	lsize = 1 << ((config & CPUCFG18_L1D_SIZE_M) >> CPUCFG18_L1D_SIZE);
	sets  = 1 << ((config & CPUCFG18_L1D_SETS_M) >> CPUCFG18_L1D_SETS);
	ways  = ((config & CPUCFG18_L1D_WAYS_M) >> CPUCFG18_L1D_WAYS) + 1;

	dcache->linesz = lsize;
	dcache->sets = sets;
	dcache->ways = ways;
	dcache_size = sets * ways * lsize;
	dcache->waysize = dcache_size / dcache->ways;

	debug("Primary instruction cache %ldkB, %s, %s, linesize %d bytes.\n",
		icache_size >> 10, way_string[icache->ways], "VIPT", icache->linesz);

	debug("Primary data cache %ldkB, %s, %s, %s, linesize %d bytes\n",
		dcache_size >> 10, way_string[dcache->ways], "VIPT", "no aliases", dcache->linesz);
}

static void probe_vcache(void)
{
	volatile struct cache_desc *vcache = &gd->arch.vcache;
	unsigned int lsize, sets, ways;
	unsigned int config;

	config = read_cpucfg(LOONGARCH_CPUCFG19);
	lsize = 1 << ((config & CPUCFG19_L2_SIZE_M) >> CPUCFG19_L2_SIZE);
	sets  = 1 << ((config & CPUCFG19_L2_SETS_M) >> CPUCFG19_L2_SETS);
	ways  = ((config & CPUCFG19_L2_WAYS_M) >> CPUCFG19_L2_WAYS) + 1;

	vcache->linesz = lsize;
	vcache->sets = sets;
	vcache->ways = ways;
	vcache_size = lsize * sets * ways;
	vcache->waysize = vcache_size / vcache->ways;

	debug("Unified victim cache %ldkB %s, linesize %d bytes.\n",
		vcache_size >> 10, way_string[vcache->ways], vcache->linesz);
}

static void probe_scache(void)
{
	volatile struct cache_desc *scache = &gd->arch.scache;
	unsigned int lsize, sets, ways;
	unsigned int config;

	config = read_cpucfg(LOONGARCH_CPUCFG20);
	lsize = 1 << ((config & CPUCFG20_L3_SIZE_M) >> CPUCFG20_L3_SIZE);
	sets  = 1 << ((config & CPUCFG20_L3_SETS_M) >> CPUCFG20_L3_SETS);
	ways  = ((config & CPUCFG20_L3_WAYS_M) >> CPUCFG20_L3_WAYS) + 1;

	scache->linesz = lsize;
	scache->sets = sets;
	scache->ways = ways;
	/* scaches are shared */
	scache_size = lsize * sets * ways;
	scache->waysize = scache_size / scache->ways;

	debug("Unified secondary cache %ldkB %s, linesize %d bytes.\n",
		scache_size >> 10, way_string[scache->ways], scache->linesz);
}

void loongarch_cache_probe(void)
{
	probe_pcache();
	probe_vcache();
	probe_scache();
}

/* Cache operations. */
void local_flush_icache_range(unsigned long start, unsigned long end)
{
	asm volatile ("\tibar 0\n"::);
}

inline unsigned long icache_line_size(void)
{
#ifdef CONFIG_SYS_CACHE_SIZE_AUTO
	return gd->arch.icache.linesz;
#else
	return CONFIG_SYS_ICACHE_LINE_SIZE;
#endif
}

inline unsigned long dcache_line_size(void)
{
#ifdef CONFIG_SYS_CACHE_SIZE_AUTO
	return gd->arch.dcache.linesz;
#else
	return CONFIG_SYS_DCACHE_LINE_SIZE;
#endif
}

inline unsigned long vcache_line_size(void)
{
#ifdef CONFIG_SYS_CACHE_SIZE_AUTO
	return gd->arch.vcache.linesz;
#else
	return CONFIG_SYS_VCACHE_LINE_SIZE;
#endif
}

inline unsigned long scache_line_size(void)
{
#ifdef CONFIG_SYS_CACHE_SIZE_AUTO
	return gd->arch.scache.linesz;
#else
	return CONFIG_SYS_SCACHE_LINE_SIZE;
#endif
}

void flush_cache(unsigned long addr, unsigned long size)
{

}

void flush_dcache_range(unsigned long addr, unsigned long size)
{
	asm volatile ("\tdbar 0\n"::);
}

void invalidate_dcache_range(unsigned long addr, unsigned long size)
{

}

int dcache_status(void)
{
	return 0;
}

void dcache_enable(void)
{
}

void dcache_disable(void)
{
}
