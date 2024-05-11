// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2011-2013 Pali Rohár <pali.rohar@gmail.com>
 */

#include <common.h>
#include <command.h>
#include <env.h>
#include <watchdog.h>
#include <malloc.h>
#include <linux/string.h>
#include <linux/fb.h>

#include "loongson_update.h"

extern char* ls_panel_display_timing_option_generic(int n);
extern char* ls_panel_display_timing_option_desc(int n);

void update_resolution(char *command, int num)
{
	int ret;

	//default is bootmenu value
	if (!command || !strcmp(command, "bootmenu"))
		return;

	ret = env_set("panel", command);
	if (!ret)
		ret = env_save();

	printf("################################################\n");
	printf("### option : %s\n", ls_panel_display_timing_option_desc(num));
	printf("### panel  : %s\n", command);
	printf("### result : %s\n", ret ? "FAIL" : "PASS");
	printf("################################################\n");

	if (!ret)
		printf("display switch after reboot\n");
}

char* get_resolution_option(int n)
{
	return ls_panel_display_timing_option_generic(n);
}

int do_res_menu(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	updatemenu_type = UPDATE_TYPE_RESOLUTION;
	updatemenu_return = UPDATEMENU_RETURN_CONSOLE;
	updatemenu_show();

	return 0;
}

U_BOOT_CMD(
	res_menu, 2, 1, do_res_menu,
	"ANSI terminal res_menu",
	"command to select video resolution"
);
