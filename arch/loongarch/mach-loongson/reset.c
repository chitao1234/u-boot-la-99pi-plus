/*
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <mach/loongson.h>

void _machine_restart(void)
{
	writel(0x1, LS_RST_CNT_REG);
	while (1) /* NOP*/ ;
}

int do_poweroff(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
#ifndef NOT_USE_ACPI
	writel(0xffffffff, LS_PM1_STS_REG);
	writel(0x1c00, LS_PM1_CNT_REG);
	writel(0x3c00, LS_PM1_CNT_REG);
#endif
	while (1) /* NOP*/ ;
	return 0;
}

void _machine_poweroff(void)
{
	do_poweroff(NULL, 0, 0, NULL);
}
