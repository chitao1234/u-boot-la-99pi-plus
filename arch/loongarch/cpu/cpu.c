// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, <wd@denx.de>
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <linux/compiler.h>
#include <asm/cache.h>
#include <asm/loongarch.h>
#include <dm.h>
#include <clk.h>
#include <clk-uclass.h>

DECLARE_GLOBAL_DATA_PTR;

#if !CONFIG_IS_ENABLED(SYSRESET)
void __weak _machine_restart(void)
{
	fprintf(stderr, "*** reset failed ***\n");

	while (1)
		/* NOP */;
}

void reset_cpu(void)
{
	_machine_restart();
}

int do_reset(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	_machine_restart();

	return 0;
}
#endif

u32 get_cpuprid(void)
{
	return read_cpucfg(LOONGARCH_CPUCFG0);
}

int arch_cpu_init(void)
{
	u32 freq, mul, div, val;

	loongarch_cache_probe();
	gd->arch.cpuprid = get_cpuprid();
	// get stable counter freq.
	val = read_cpucfg(LOONGARCH_CPUCFG2);
	if (val % (1 << 14)) {
		freq = read_cpucfg(LOONGARCH_CPUCFG4);
		val = read_cpucfg(LOONGARCH_CPUCFG5);
		mul = val & 0xFFFF;
		div = (val >> 16) & 0xFFFF;
		gd->arch.timer_freq = freq * mul / div;
	} else {
		gd->arch.timer_freq = CONFIG_SYS_HZ;
	}

	debug("timer freq: %ld\n", gd->arch.timer_freq);
	return 0;
}

/* arch specific CPU init after DM */
int arch_cpu_init_dm(void)
{
	int ret;
	struct udevice *dev;

	ret = uclass_get_device(UCLASS_CLK, 0, &dev);
	if (ret) {
		printf("clk-uclass not found\n");
		return 0;
	}

	return 0;
}
