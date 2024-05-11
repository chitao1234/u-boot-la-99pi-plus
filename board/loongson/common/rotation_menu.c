// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2021 maoxiaochuan <maoxiaochuan@loongson.cn>
 */

#include <common.h>
#include <command.h>
#include <env.h>
#include <watchdog.h>
#include <malloc.h>
#include <linux/string.h>
#include <linux/fb.h>

#include "loongson_update.h"


char *rotation_menu[] = {
	"normal orientation (0 degree)=0",
	"clockwise orientation (90 degrees)=1",
	"upside down orientation (180 degrees)=2",
	"counterclockwise orientation (270 degrees)=3",
	NULL
};

static inline int set_rotation(char *rotation)
{
	int ret;
	ret = env_set("rotate", rotation);
	if (!ret)
		ret = env_save();
	return ret;
}

void update_rotation(char *command, int num)
{
	int ret;

	if (num < 0 || num >= 4)
		return ;

	ret = set_rotation(command);
	printf("################################################\n");
	printf("### option : %s\n", rotation_menu[num]);
	printf("### result : %s\n", ret ? "FAIL" : "PASS");
	printf("################################################\n");
}

int do_rotation_menu(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	updatemenu_type = UPDATE_TYPE_ROTATION;
	updatemenu_return = UPDATEMENU_RETURN_CONSOLE;
	updatemenu_show();

	return 0;
}

U_BOOT_CMD(
	rotation_menu, 2, 1, do_rotation_menu,
	"ANSI terminal rotation_menu",
	"command to select rotation degree"
);
