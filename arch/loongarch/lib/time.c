// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <common.h>
#include <time.h>
#include <asm/loongarch.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

unsigned long notrace timer_read_counter(void)
{
	return (unsigned long)drdtime();
}

ulong notrace get_tbclk(void)
{
	return gd->arch.timer_freq;
}

uint64_t get_ticks(void)
{
	return drdtime();
}
