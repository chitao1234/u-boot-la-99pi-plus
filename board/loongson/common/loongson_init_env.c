// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <malloc.h>

#include "loongson_init_env.h"

static int do_loongson_init_env(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;

	if(argc != 1)
		return ret;

	ret = run_command("mtd erase uboot_env", 0);
	if (ret)
		return ret;
	ret = run_command("reset", 0);

	return ret;
}

U_BOOT_CMD(
	loongson_init_env,    1,    0,     do_loongson_init_env,
	"init uboot env after reboot.",
	""
);
