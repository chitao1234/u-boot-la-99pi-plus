// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2011-2013 Pali Rohár <pali.rohar@gmail.com>
 */

#include <common.h>
#include <command.h>
#include <ansi.h>
#include <env.h>
#include <menu.h>
#include <watchdog.h>
#include <malloc.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "loongson_update.h"

#ifdef CONFIG_LOONGSON_COMPAT
#include "board_kernel_product.h"
#endif

extern const char *update_typename_str[UPDATE_TYPE_COUNT];
int updatemenu_type;
int updatemenu_return = UPDATEMENU_RETURN_CONSOLE;

static char *updatemenu_kernel[] = {
#ifdef CONFIG_AHCI
	"Update kernel (uImage) to sata disk (by usb)=loongson_update usb kernel sata\0",
	#ifdef CONFIG_MMC
	"Update kernel (uImage) to sata disk (by mmc)=loongson_update mmc kernel sata\0",
	#endif
	"Update kernel (uImage) to sata disk (by tftp)=loongson_update tftp kernel sata\0",
#endif
#ifdef CONFIG_MTD_RAW_NAND
	"Update kernel (uImage) to nand flash (by usb)=loongson_update usb kernel nand\0",
	#ifdef CONFIG_MMC
	"Update kernel (uImage) to nand flash (by mmc)=loongson_update mmc kernel nand\0",
	#endif
	"Update kernel (uImage) to nand flash (by tftp)=loongson_update tftp kernel nand\0",
#endif
#ifdef CONFIG_LOONGSON_GENERAL_LOAD
	#ifdef CONFIG_MMC
	#ifdef CONFIG_USB_STORAGE
	"Update kernel (uImage) to mmc (by usb)=general_load --if usb0:0 --fmt fat --sym /update/uImage --of mmc0 --fmt ext4 --sym /boot/uImage\0",
	#endif
	"Update kernel (uImage) to mmc (by tftp)=general_load --if net --fmt tftp --sym /uImage --of mmc0 --fmt ext4 --sym /boot/uImage\0",
	#endif
#endif
	NULL
};

static char *updatemenu_rootfs[] = {
#ifdef CONFIG_MTD_RAW_NAND
	"Update rootfs (rootfs-ubifs-ze.img) to nand flash (by usb)=loongson_update usb rootfs nand\0",
	#ifdef CONFIG_MMC
	"Update rootfs (rootfs-ubifs-ze.img) to nand flash (by mmc)=loongson_update mmc rootfs nand\0",
	#endif
	"Update rootfs (rootfs-ubifs-ze.img) to nand flash (by tftp)=loongson_update tftp rootfs nand\0",
#endif
	NULL
};

static char *updatemenu_uboot[] = {
	"Update u-boot to spi flash (by tftp)=loongson_update tftp uboot\0",
	"Update u-boot to spi flash (by usb)=loongson_update usb uboot\0",
	#ifdef CONFIG_MMC
	"Update u-boot to spi flash (by mmc)=loongson_update mmc uboot\0",
	"Update u-boot to mmc       (by tftp)=general_load --if net --fmt tftp --sym /u-boot-with-spl.bin --of mmc0 --fmt ext4 --sym /boot/u-boot-with-spl.bin\0",
	#endif
	#ifdef CONFIG_USB_STORAGE
	"Update u-boot to usb       (by tftp)=general_load --if net --fmt tftp --sym /u-boot-with-spl.bin --of usb0:0 --fmt fat --sym /u-boot-with-spl.bin\0",
	#endif
	NULL
};

static char *updatemenu_dtb[] = {
	"Update DTB (dtb.bin) to spi flash (by usb)=loongson_update usb dtb\0",
	#ifdef CONFIG_MMC
	"Update DTB (dtb.bin) to spi flash (by mmc)=loongson_update mmc dtb\0",
	#endif
	"Update DTB (dtb.bin) to spi flash (by tftp)=loongson_update tftp dtb\0",
	"Clean  DTB parts=sf probe;sf erase dtb 0x10000\0",
	NULL
};

static char *updatemenu_all[] = {
#ifdef CONFIG_MTD_RAW_NAND
	"Update all (kernel rootfs uboot) to nand flash (by usb)=loongson_update usb all nand\0",
	#ifdef CONFIG_MMC
	"Update all (kernel rootfs uboot) to nand flash (by mmc)=loongson_update mmc all nand\0",
	#endif
	"Update all (kernel rootfs uboot) to nand flash (by tftp)=loongson_update tftp all nand\0",
#endif
	NULL
};

static char *updatemenu_system[] = {
#ifdef CONFIG_AHCI
	"System install to sata disk (by usb)=recover_cmd usb\0",
	"System install to sata disk (by usb iso)=bootcfg usb\0",
	"System install to sata disk (by tftp)=recover_cmd tftp\0",
	#ifdef CONFIG_MMC
	"System install to sata disk (by mmc)=recover_cmd mmc\0",
	#endif
	"System recover from sata disk=recover_cmd sata\0",
	#ifdef LS_DOUBLE_SYSTEM
	"System recover last time boot=recover_cmd last\0",
	#endif
#endif
#ifdef CONFIG_LOONGSON_GENERAL_LOAD
	#ifdef CONFIG_MMC
	#ifdef CONFIG_USB_STORAGE
	"System(rootfs.ext4.gz) install to mmc(by usb)=general_load --if usb --fmt fat --sym /install/rootfs.ext4.gz --of mmc0 --decompress\0",
	#endif
	"System(rootfs.ext4.gz) install to mmc(by tftp)=general_load --if net --fmt tftp --sym rootfs.ext4.gz --of mmc0 --decompress\0",
	#endif
#endif
	NULL
};

#ifndef BOOT_SATA_DEFAULT
#define BOOT_SATA_DEFAULT
#endif

#ifndef BOOT_NAND_DEFAULT
#define BOOT_NAND_DEFAULT
#endif

static char *updatemenu_bootselect[] = {
#ifdef CONFIG_AHCI
	"System Boot from sata disk=loongson_boot ssd\0",
	"System Boot from sata disk(boof.cfg)=bootcfg scsi\0",
#endif
#ifdef CONFIG_MTD_RAW_NAND
	"System Boot from nand flash=loongson_boot nand\0",
#endif
#ifdef CONFIG_MMC
	"System Boot from mmc=loongson_boot mmc\0",
#endif
//	"System Boot from usb=" BOOT_USB_DEFAULT "\0",
	NULL
};

#if !defined(CONFIG_VIDEO)

__weak char *resolution_menu[] = {
	"Video dev not defined=""\0",
	NULL
};

__weak void update_resolution(char *command, int num)
{
	run_command(command, 0);
}

__weak char* get_resolution_option(int n)
{
	return resolution_menu[n];
}

//////////////////////////////

__weak char *rotation_menu[] = {
	"Video dev not defined=""\0",
	NULL
};

__weak void update_rotation(char *command, int num)
{
	run_command(command, 0);
}
#endif

__weak int set_board_kernel_product_id(char* command)
{
	printf("cur uboot dont support select board product\n");
	return 0;
}

__weak char* product_null_set[] = {
	NULL
};

__weak char* get_board_kernel_product_menu_option(int n)
{
	return product_null_set[n];
}

/* maximum updatemenu entries */
#define MAX_COUNT	99

/* maximal size of updatemenu env
 *  9 = strlen("updatemenu_")
 *  2 = strlen(MAX_COUNT)
 *  1 = NULL term
 */
#define MAX_ENV_SIZE	(9 + 2 + 1)

struct updatemenu_entry {
	unsigned short int num;		/* unique number 0 .. MAX_COUNT */
	char key[3];			/* key identifier of number */
	char *title;			/* title of entry */
	char *command;			/* hush command of entry */
	struct updatemenu_data *menu;	/* this updatemenu */
	struct updatemenu_entry *next;	/* next menu entry (num+1) */
};

struct updatemenu_data {
	int delay;			/* delay for autoboot */
	int active;			/* active menu entry */
	int count;			/* total count of menu entries */
	struct updatemenu_entry *first;	/* first menu entry */
};

enum updatemenu_key {
	KEY_NONE = 0,
	KEY_UP,
	KEY_DOWN,
	KEY_SELECT,
};

static char *updatemenu_getoption(unsigned short int n)
{
	if (n > MAX_COUNT)
		return NULL;

	switch (updatemenu_type) {
	case UPDATE_TYPE_KERNEL:
		return updatemenu_kernel[n];
		break;
	case UPDATE_TYPE_ROOTFS:
		return updatemenu_rootfs[n];
		break;
	case UPDATE_TYPE_UBOOT:
		return updatemenu_uboot[n];
		break;
	case UPDATE_TYPE_DTB:
		return updatemenu_dtb[n];
		break;
	case UPDATE_TYPE_ALL:
		return updatemenu_all[n];
		break;
	case UPDATE_TYPE_SYSTEM:
		return updatemenu_system[n];
		break;
	case UPDATE_TYPE_BOOTSELECT:
		return updatemenu_bootselect[n];
		break;
	case UPDATE_TYPE_RESOLUTION:
		return get_resolution_option(n);
		break;
	case UPDATE_TYPE_ROTATION:
		//return rotation_menu[n];
		break;
	case UPDTAE_TYPE_PRODUCT:
		return get_board_kernel_product_menu_option(n);
		break;
	}
	return NULL;
}

static void updatemenu_display_statusline(struct menu *m)
{
	struct updatemenu_data *menu;
	struct updatemenu_entry *entry;

	if (menu_default_choice(m, (void *)&entry) < 0)
		return;

	menu = entry->menu;

	printf(ANSI_CURSOR_POSITION, 1, 1);
	puts(ANSI_CLEAR_LINE);
	printf(ANSI_CURSOR_POSITION, 2, 1);
	puts("  *** U-Boot Boot Menu ***");
	puts(ANSI_CLEAR_LINE_TO_END);
	printf(ANSI_CURSOR_POSITION, 3, 1);
	puts(ANSI_CLEAR_LINE);

	/* First 3 lines are bootmenu header + 2 empty lines between entries */
	printf(ANSI_CURSOR_POSITION, menu->count + 5, 1);
	puts(ANSI_CLEAR_LINE);
	printf(ANSI_CURSOR_POSITION, menu->count + 6, 1);
	puts("  Press UP/DOWN to move, ENTER to select");
	puts(ANSI_CLEAR_LINE_TO_END);
	printf(ANSI_CURSOR_POSITION, menu->count + 7, 1);
	puts(ANSI_CLEAR_LINE);
}

static void updatemenu_print_entry(void *data)
{
	struct updatemenu_entry *entry = data;
	int reverse = (entry->menu->active == entry->num);

	/*
	 * Move cursor to line where the entry will be drown (entry->num)
	 * First 3 lines contain updatemenu header + 1 empty line
	 */
	printf(ANSI_CURSOR_POSITION, entry->num + 4, 1);

	puts("     ");

	if (reverse)
		puts(ANSI_COLOR_REVERSE);

#ifdef CONFIG_BOOTMENU_HAVE_NUM
	{
	char buf[8];
	char num = entry->num + 1;
	num = num > 9 ? (num - 10 + 0x61) : (num + 0x30);
	sprintf(buf, "[%c] ", num);
	puts(buf);
	}
#endif

	puts(entry->title);

	if (reverse)
		puts(ANSI_COLOR_RESET);
}

static void updatemenu_loop(struct updatemenu_data *menu,
		enum updatemenu_key *key, int *esc)
{
	int c;

	while (!tstc()) {
		schedule();
		mdelay(10);
	}

	c = getchar();

	switch (*esc) {
	case 0:
		/* First char of ANSI escape sequence '\e' */
		if (c == '\e') {
			*esc = 1;
			*key = KEY_NONE;
		}
		break;
	case 1:
		/* Second char of ANSI '[' */
		if (c == '[') {
			*esc = 2;
			*key = KEY_NONE;
		} else {
			*esc = 0;
		}
		break;
	case 2:
	case 3:
		/* Third char of ANSI (number '1') - optional */
		if (*esc == 2 && c == '1') {
			*esc = 3;
			*key = KEY_NONE;
			break;
		}

		*esc = 0;

		/* ANSI 'A' - key up was pressed */
		if (c == 'A')
			*key = KEY_UP;
		/* ANSI 'B' - key down was pressed */
		else if (c == 'B')
			*key = KEY_DOWN;
		/* other key was pressed */
		else
			*key = KEY_NONE;

		break;
	}

	/* enter key was pressed */
	if (c == '\r')
		*key = KEY_SELECT;
}

static char *updatemenu_choice_entry(void *data)
{
	struct updatemenu_data *menu = data;
	struct updatemenu_entry *iter;
	enum updatemenu_key key = KEY_NONE;
	int esc = 0;
	int i;

	while (1) {
		/* Some key was pressed, so autoboot was stopped */
		updatemenu_loop(menu, &key, &esc);

		switch (key) {
		case KEY_UP:
			if (menu->active > 0)
				--menu->active;
			/* no menu key selected, regenerate menu */
			return NULL;
		case KEY_DOWN:
			if (menu->active < menu->count - 1)
				++menu->active;
			/* no menu key selected, regenerate menu */
			return NULL;
		case KEY_SELECT:
			iter = menu->first;
			for (i = 0; i < menu->active; ++i)
				iter = iter->next;
			return iter->key;
		default:
			break;
		}
	}

	/* never happens */
	debug("updatemenu: this should not happen");
	return NULL;
}

static void updatemenu_destroy(struct updatemenu_data *menu)
{
	struct updatemenu_entry *iter = menu->first;
	struct updatemenu_entry *next;

	while (iter) {
		next = iter->next;
		free(iter->title);
		free(iter->command);
		free(iter);
		iter = next;
	}
	free(menu);
}

static struct updatemenu_data *updatemenu_create(void)
{
	unsigned short int i = 0;
	const char *option;
	struct updatemenu_data *menu;
	struct updatemenu_entry *iter = NULL;

	int len;
	char *sep;
	struct updatemenu_entry *entry;

	menu = malloc(sizeof(struct updatemenu_data));
	if (!menu)
		return NULL;

	menu->delay = 0;
	menu->active = 0;
	menu->first = NULL;

	while ((option = updatemenu_getoption(i))) {
		sep = strchr(option, '=');
		if (!sep) {
			printf("Invalid updatemenu entry: %s\n", option);
			break;
		}

		entry = malloc(sizeof(struct updatemenu_entry));
		if (!entry)
			goto cleanup;

		len = sep-option;
		entry->title = malloc(len + 1);
		if (!entry->title) {
			free(entry);
			goto cleanup;
		}
		memcpy(entry->title, option, len);
		entry->title[len] = 0;

		len = strlen(sep + 1);
		entry->command = malloc(len + 1);
		if (!entry->command) {
			free(entry->title);
			free(entry);
			goto cleanup;
		}
		memcpy(entry->command, sep + 1, len);
		entry->command[len] = 0;

		sprintf(entry->key, "%d", i);

		entry->num = i;
		entry->menu = menu;
		entry->next = NULL;

		if (!iter)
			menu->first = entry;
		else
			iter->next = entry;

		iter = entry;
		++i;

		if (i == MAX_COUNT - 1)
			break;
	}

	/* Add U-Boot console entry at the end */
	if (i <= MAX_COUNT - 1) {
		entry = malloc(sizeof(struct updatemenu_entry));
		if (!entry)
			goto cleanup;
#ifndef UPDATEMENU_END
#define UPDATEMENU_END "Return"
#endif
		entry->title = strdup(UPDATEMENU_END);
		if (!entry->title) {
			free(entry);
			goto cleanup;
		}

		switch (updatemenu_return) {
		case UPDATEMENU_RETURN_CONSOLE:
			entry->command = strdup("");
			break;
		case UPDATEMENU_RETURN_MENU:
			entry->command = strdup("bootmenu");
			break;
		default:
			entry->command = strdup("");
			break;
		}

		if (!entry->command) {
			free(entry->title);
			free(entry);
			goto cleanup;
		}

		sprintf(entry->key, "%d", i);

		entry->num = i;
		entry->menu = menu;
		entry->next = NULL;

		if (!iter)
			menu->first = entry;
		else
			iter->next = entry;

		iter = entry;
		++i;
	}

	menu->count = i;

	if ((menu->active >= menu->count)||(menu->active < 0)) { //ensure active menuitem is inside menu
		printf("active menuitem (%d) is outside menu (0..%d)\n",menu->active,menu->count-1);
		menu->active=0;
	}

	return menu;

cleanup:
	updatemenu_destroy(menu);
	return NULL;
}

void updatemenu_show(void)
{
	int init = 0;
	void *choice = NULL;
	char *title = NULL;
	char *command = NULL;
	int num;
	struct menu *menu;
	struct updatemenu_data *updatemenu;
	struct updatemenu_entry *iter;

	updatemenu = updatemenu_create();
	if (!updatemenu)
		return;

	menu = menu_create(NULL, updatemenu->delay, 1, updatemenu_display_statusline,
				updatemenu_print_entry, updatemenu_choice_entry, updatemenu);
	if (!menu) {
		updatemenu_destroy(updatemenu);
		return;
	}

	for (iter = updatemenu->first; iter; iter = iter->next) {
		if (!menu_item_add(menu, iter->key, iter))
			goto cleanup;
	}

	/* Default menu entry is always first */
	menu_default_set(menu, "0");

	puts(ANSI_CURSOR_HIDE);
	puts(ANSI_CLEAR_CONSOLE);
	printf(ANSI_CURSOR_POSITION, 1, 1);

	init = 1;

	if (menu_get_choice(menu, &choice)) {
		iter = choice;
		title = strdup(iter->title);
		command = strdup(iter->command);
		num = iter->num;  //iter保留了也没用，很快就要摧毁了。
	}

cleanup:
	menu_destroy(menu);
	updatemenu_destroy(updatemenu);

	if (init) {
		puts(ANSI_CURSOR_SHOW);
		puts(ANSI_CLEAR_CONSOLE);
		printf(ANSI_CURSOR_POSITION, 1, 1);
	}

	if (title && command) {
		debug("Starting entry '%s'\n", title);
		free(title);
		if (updatemenu_type == UPDATE_TYPE_RESOLUTION) {
			update_resolution(command, num);
		} else if (updatemenu_type == UPDATE_TYPE_ROTATION) {
			update_rotation(command, num);
		} else if (updatemenu_type == UPDTAE_TYPE_PRODUCT) {
			set_board_kernel_product_id(command);
		} else {
			run_command(command, 0);
		}
		free(command);
	}

#ifdef CONFIG_POSTBOOTMENU
	run_command(CONFIG_POSTBOOTMENU, 0);
#endif
}

int do_updatemenu(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	char *return_str = NULL;

	updatemenu_type = UPDATE_TYPE_UNKNOWN;
	updatemenu_return = UPDATEMENU_RETURN_CONSOLE;

	for (updatemenu_type = 0; updatemenu_type < UPDATE_TYPE_COUNT; updatemenu_type++) {
		if (update_typename_str[updatemenu_type] &&
			!strcmp(argv[1], update_typename_str[updatemenu_type]))
			break;
	}
	if ((updatemenu_type == UPDATE_TYPE_UNKNOWN) || (updatemenu_type >= UPDATE_TYPE_COUNT)) {
		printf("updatemenu type unknown\r\n");
		printf("please intput type: kernel/rootfs/uboot/dtb\r\n");
		goto err;
	}

	if (argc >= 3)
		return_str = argv[2];

	if (return_str) {
		updatemenu_return = (int)simple_strtol(return_str, NULL, 10);
		if (updatemenu_return < 0 || updatemenu_return > 1) {
			updatemenu_return = UPDATEMENU_RETURN_CONSOLE;
		}
	}

	updatemenu_show();

	return 0;

err:
	return -1;
}

U_BOOT_CMD(
	updatemenu, 3, 1, do_updatemenu,
	"ANSI terminal updatemenu",
	"<kernel|rootfs|uboot|dtb|system|resolution>"
);

int do_updatemenu_kernel(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	updatemenu_type = UPDATE_TYPE_KERNEL;
	updatemenu_return = UPDATEMENU_RETURN_CONSOLE;
	updatemenu_show();

	return 0;
}

U_BOOT_CMD(
	updatemenu_kernel, 2, 1, do_updatemenu_kernel,
	"ANSI terminal updatemenu_kernel",
	"command to update uImage"
);

int do_updatemenu_rootfs(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	updatemenu_type = UPDATE_TYPE_ROOTFS;
	updatemenu_return = UPDATEMENU_RETURN_CONSOLE;
	updatemenu_show();

	return 0;
}

U_BOOT_CMD(
	updatemenu_rootfs, 2, 1, do_updatemenu_rootfs,
	"ANSI terminal updatemenu_rootfs",
	"command to update rootfs"
);

int do_updatemenu_uboot(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	updatemenu_type = UPDATE_TYPE_UBOOT;
	updatemenu_return = UPDATEMENU_RETURN_CONSOLE;
	updatemenu_show();

	return 0;
}

U_BOOT_CMD(
	updatemenu_uboot, 2, 1, do_updatemenu_uboot,
	"ANSI terminal updatemenu_uboot",
	"command to update u-boot"
);

int do_updatemenu_dtb(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	updatemenu_type = UPDATE_TYPE_DTB;
	updatemenu_return = UPDATEMENU_RETURN_CONSOLE;
	updatemenu_show();

	return 0;
}

U_BOOT_CMD(
	updatemenu_dtb, 2, 1, do_updatemenu_dtb,
	"ANSI terminal updatemenu_dtb",
	"command to update dtb file"
);

int do_updatemenu_all(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	updatemenu_type = UPDATE_TYPE_ALL;
	updatemenu_return = UPDATEMENU_RETURN_CONSOLE;
	updatemenu_show();

	return 0;
}

U_BOOT_CMD(
	updatemenu_all, 2, 1, do_updatemenu_all,
	"ANSI terminal updatemenu_all",
	"command to update kernel roofs u-boot to nand flash"
);

int do_updatemenu_system(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	updatemenu_type = UPDATE_TYPE_SYSTEM;
	updatemenu_return = UPDATEMENU_RETURN_CONSOLE;
	updatemenu_show();

	return 0;
}

U_BOOT_CMD(
	updatemenu_system, 2, 1, do_updatemenu_system,
	"ANSI terminal updatemenu_system",
	"command to install or recover system to ssd disk"
);

int do_board_product_menu(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	updatemenu_type = UPDTAE_TYPE_PRODUCT;
	updatemenu_return = UPDATEMENU_RETURN_CONSOLE;
	updatemenu_show();

	return 0;
}

U_BOOT_CMD(
	board_product_menu, 2, 1, do_board_product_menu,
	"ANSI terminal board_product_menu",
	"command to select board product to change board function"
);
