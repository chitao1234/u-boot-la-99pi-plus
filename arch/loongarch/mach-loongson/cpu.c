/*
 * Copyright (C) 2022
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */
#include <common.h>
#include <dm.h>
#include <clk.h>
#include <clk-uclass.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/addrspace.h>

#define PRID_SERIES_MASK	0xf000

#define PRID_SERIES_LA132	0x8000  /* Loongson 32bit */
#define PRID_SERIES_LA264	0xa000  /* Loongson 64bit, 2-issue */
#define PRID_SERIES_LA364	0xb000  /* Loongson 64bit, 3-issue */
#define PRID_SERIES_LA464	0xc000  /* Loongson 64bit, 4-issue */
#define PRID_SERIES_LA664	0xd000  /* Loongson 64bit, 6-issue */

DECLARE_GLOBAL_DATA_PTR;

extern void get_clocks(void);


int mach_cpu_init(void)
{
	get_clocks();
	return 0;
}


const char *get_core_name(void)
{
	u32 prid;
	const char *str;

	prid = gd->arch.cpuprid;
	switch (prid & PRID_SERIES_MASK) {
	case PRID_SERIES_LA132:
		str = "LA132";
		break;
	case PRID_SERIES_LA264:
		str = "LA264";
		break;
	case PRID_SERIES_LA364:
		str = "LA364";
		break;
	case PRID_SERIES_LA464:
		str = "LA464";
		break;
	case PRID_SERIES_LA664:
		str = "LA664";
		break;
	default:
		str = "Unknown";
	}

	return str;
}

int print_cpuinfo(void)
{
	printf("CPU:   %s\n", get_core_name());
	printf("Speed: Cpu @ %ld MHz/ Mem @ %ld MHz/ Bus @ %ld MHz\n",
			gd->cpu_clk/1000000, gd->mem_clk/1000000, gd->bus_clk/1000000);
	return 0;
}
