// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <ansi.h>
#include <menu.h>
#include <watchdog.h>
#include <elf.h>
#include <part.h>
#include <linux/string.h>
#include <linux/delay.h>

#define issep(ch) ((ch) == ' ' || (ch) == '\t')

#define MAXARGS 256

#define MAX_TIME_OUT 1000

#define OPTION_LEN	50
#define VALUE_LEN	1024
#define GLOBAL_VALUE_LEN 256

#define MENU_TITLE_BUF_LEN 79

///最大的磁盘数
#define MAX_DEVS	4

///磁盘的最大分区数
#define MAX_PARTITIONS	4

struct menu_option
{
	char option[OPTION_LEN + 1];
	int set_type;				/* 0-unset£¬1-set */
	int use_default;			/* use defualt value, 0-if value don't used, use default, 1- must set */
	char value[GLOBAL_VALUE_LEN + 1];
};

struct bootcfg_menu_item
{
	char title[MENU_TITLE_BUF_LEN + 1];	//Title of menu item, display on screen.
	char *kernel;	//kernel file to load.
	char *args;		//arguments for kernel.
	char *initrd;	//initrd file for kernel, maybe empty.
	char *root;		//ROOT device from args.
};

static struct bootcfg_menu_item menu_items[MAXARGS];//Storage All menu information.

static struct menu_option menu_options[] = {
	{"showmenu", 0, 0, "1"}, //indentfy to show or not show menu for user.
	{"default", 0, 0, "0"}, //default menu to boot.
	{"timeout", 0, 0, "5"},//default timeout in seconds wait for user.
	{"password", 0, 1, ""},
	{"md5_enable", 0, 0, "0"},	/* 1-md5 */
};

static int options_num = sizeof(menu_options) / sizeof(menu_options[0]);
static int menu_cnts = 0; //boot.cfg 中的菜单项总和

/***************************************************
 * set a value of a option.
 **************************************************/
static void set_option_value(const char *option, const char *value)
{
	int i = 0;

	for (i = 0; i < options_num; i++) {
		if (strcasecmp(option, menu_options[i].option) == 0) {
			strncpy(menu_options[i].value, value, GLOBAL_VALUE_LEN);
			menu_options[i].set_type = 1;
			return;
		}
	}
}

/***************************************************
 * Got a value of a option.
 **************************************************/
static char *get_option_value(const char *option)
{
	int i = 0;

	for (i = 0; i < options_num; i++) {
		if (strcasecmp(option, menu_options[i].option) == 0)
			return menu_options[i].value;
	}
	return NULL;
}

/**********************************************************
 * Delete SPACE and TAB from begin and end of the str.
 * Also delete \r\n at the end of the str.
 * Final check is the comment line.
**********************************************************/
static char *trim(char *string)
{
	int len;
	char *str = string;

	len = strlen(str);
	while (len && (str[len-1] == '\n' || str[len-1] == '\r' || issep(str[len-1])))
		str[--len] = '\0';

	while (issep(*str))
		str++;

	if(*str == '#')
		*str = '\0';

	return str;
}

static int exec(char **cfg_file, char *buf, int *buflen)
{
	int len = *buflen, i;

	if (**cfg_file == '\0')
		return 0;
	for (i=0; i<len; i++) {
		buf[i] = **cfg_file;
		if (**cfg_file == '\n' || **cfg_file == '\r') {
			*buflen = i + 1;
			*cfg_file = *cfg_file + 1;
			if (**cfg_file == '\n' || **cfg_file == '\r') {
				*buflen = i + 2;
			}
			break;
		}
		*cfg_file = *cfg_file + 1;
	}

	return *buflen;
}

static int read_line(char **cfg_file, char *buf, int buflen)
{
	int len, n;

	n = 0;
	len = buflen - 1;

	while (1) {
		if (exec(cfg_file, buf, &len) <= 0)//read data from fd.
			break;//end of file or error.
		n++;

		buf[len] = '\0';
		while (len && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
			buf[--len] = '\0';
		}

		if (len && buf[len-1] == '\\') {
			//prepare to read another new data to buf.
			buf += len - 1;
			buflen -= len - 1;
			if (!buflen)        /* No buffer space */
				break;
		} else {
			break;
		}
	}

	return n;
}

/*************************************************************
 * Remove comment information from string.
*************************************************************/
static void remove_comment(char *str)
{
	char *p;
	p = str;

	while (*p) {
		switch (*p) {
		case '"':
		case '\'':
			break;
		case '#':
			*p = '\0';
			return;
		}
		++p;
	}
}

static char *remove_pmon_dev(char *str)
{
	char *p = NULL;

	p = strchr(str, '(');
	if (p) {
		p = strchr(p, ')');
		if (p)
			p += 1;
	} else if (strcasecmp(str, "/dev/") != 0) {
		p = strchr(str, '@');
		if (p) {
			p = strchr(p, '/');
		}
	}

	if (p == NULL)
		p = str;

	return p;
}

/*********************************************************************
 * Split string to two part, string delimit by the SPACE or TAB.
 * char * str , strings need to be splited.
 * char * strings[2], buffer to storage splited strings.
*********************************************************************/
static int split_str(char *str, char *strings[2])
{
	char *p;
	char *p1;
	char *p2;

	//check input data buf.
	if (!str || !strings)
		return -1 ;
	if (!strings[0] || !strings [1])
		return -1;

	//Remove SPACE or TAB from header and tail.
	p = trim(str);
	//if is empty line or comment data.
	if (*p == '\0' || *p == '#')
		return -1;
	//found where is the SPACE or TAB.
	p1 = strchr(str, ' ');
	p2 = strchr(str, '\t');

	//Get the first position.
	if (p1 < p2)
		p = p1 ? p1:p2;
	else
		p = p2 ? p2:p1;

	// if found copy data.
	if (p) {
		strncpy(strings[0], str, p - str);
		strcpy(strings[1], p);
		return 2;
	} else {
		strcpy(strings[0], str);
	}
	return 1;
}

/*********************************************************************
 * Split string to key and value.
 * char * str, string to be splited
 * char * option, buffer to stor key.
 * int option_len, length of the buffer option.
 * char * value, buffer to stor value.
 * int value_len, length of the buffer value.
 * return -1 for failed, 0 for success.
*********************************************************************/
static int get_option(char *str, char *option, int option_len, char *value, int val_len)
{
	char *argv[2], *p;
	char key[MENU_TITLE_BUF_LEN +1] = {0};
	char val[VALUE_LEN +1] = {0};
	int argc;

	argv[0] = key;
	argv[1] = val;

	//split string to two part,key and value ,splited by SPACE or TAB
	argc = split_str(str, argv);

	//if split to two part, return 2.
	if (argc < 2)
		return -1;

	//remove SPACE and TAB from key and val header and tail.
	p = trim(key);
	//copy to buffer.
	strncpy(option, p, option_len - 1);

	p = trim(val);
	//remove comment data from string.
	remove_comment(p);
	strncpy(value, p, val_len -1);

	return 0;
}

/******************************************************************
 * Extract the menu title from string.
 * const char * str, the string include menu title information.
 * if found, return 0, else return -1.
******************************************************************/
static int get_title(char *str, char *title, int title_buf_len)
{
	char *argv[2], *p;
	char key[MENU_TITLE_BUF_LEN +1] = {0};
	char value[VALUE_LEN +1] = {0};
	int argc, len;
	argv[0] = key;
	argv[1] = value;

	//Parse string.
	argc = split_str(str, argv);

	if (argc < 1)
		return -1;

	if (argc == 1) {
		//Support for OLD style lable
		len = strlen(argv[0]);
		if (argv[0][len -1] == ':') {
			argv[0][len -1] = '\0';
			p = trim(argv[0]);
			//remove SPACE and TAB from begin and end of string.
			strncpy(title, p, title_buf_len);
			return 0;
		}
	} else {
		//remove SPACE and TAB from begin and end of string.
		p = trim(key);
		//check key.
		if (strcasecmp(p, "title") == 0) {
			//remove SPACE and TAB from begin and end of string.
			p = trim(value);
			//remove comment data from string.
			remove_comment(p);
			strncpy(title, p, title_buf_len);
			return 0;
		}
	}
	return -1;
}

int menu_list_read(char *cfg_file, int flags)
{
	int j; //index for current menu item.
	char buf[1025];
	char buf_bak[1025];
	int buflen = 1024;
	char *cp = NULL;
	char option[OPTION_LEN] = {0};
	char value[VALUE_LEN] = {0};
	int in_menu = 0;
	char title[MENU_TITLE_BUF_LEN + 1];//Title of menu item, display on screen.

	memset(menu_items, 0, sizeof(struct bootcfg_menu_item) * MAXARGS);

	j = -1; //set to 0;

	while (read_line(&cfg_file, buf, buflen) > 0) { //Read a line.
		memset(title, 0, MENU_TITLE_BUF_LEN + 1);

		strcpy(buf_bak, buf);//Got a copy of buf.
		cp = trim(buf);//Trim space
		//If only a empty line,or comment line, drop it.
		if (*cp == '\0' || *cp == '#') {
			continue;
		}

		//Check data, looking for menu title.
		if (get_title(cp, title, MENU_TITLE_BUF_LEN) == 0) {
			j++;
			strncpy(menu_items[j].title, title, MENU_TITLE_BUF_LEN);//storage it.
			if (menu_items[j].kernel == NULL) {
				menu_items[j].kernel = malloc(VALUE_LEN + 1);
				menu_items[j].args = malloc(VALUE_LEN + 1);
				menu_items[j].initrd = malloc(VALUE_LEN + 1);
				menu_items[j].root = malloc(VALUE_LEN + 1);
			}
			memset(menu_items[j].kernel, 0, VALUE_LEN + 1);
			memset(menu_items[j].args, 0, VALUE_LEN + 1);
			memset(menu_items[j].initrd, 0, VALUE_LEN + 1);
			memset(menu_items[j].root, 0, VALUE_LEN + 1);
			in_menu = 1;
			continue;
		}

		cp = trim(buf_bak);
		if (in_menu) {
			//Read all properties of current menu item.
			if (get_option(cp, option, OPTION_LEN, value, VALUE_LEN) == 0) {
				if (strcasecmp(option, "kernel") == 0) { // got kernel property
					// we only sotr the firs kernel property, drop others.
					if (menu_items[j].kernel != NULL && menu_items[j].kernel[0] == '\0') {
						strncpy(menu_items[j].kernel, value, VALUE_LEN);
					}
				} else if (strcasecmp(option, "args") == 0) { // got kernel arguments property.
					if (menu_items[j].args != NULL && menu_items[j].args[0] == '\0')//same as the kernel property.
						strncpy(menu_items[j].args, value, VALUE_LEN);
				} else if (strcasecmp(option, "initrd") == 0) { // go initrd kernel arguments property, may be null.
					if (menu_items[j].initrd != NULL && menu_items[j].initrd[0] == '\0') { // same as the kernel property.
						strncpy(menu_items[j].initrd, value, VALUE_LEN);
					}
				} else if (strcasecmp(option, "root") == 0) {
					if (menu_items[j].root != NULL && menu_items[j].root[0] == '\0') {
						strncpy(menu_items[j].root, value, VALUE_LEN);
					}
				}
			}
		} else { // out of menu item.
			//Check data, looking for global option.
			if (get_option(cp, option, OPTION_LEN, value, VALUE_LEN) == 0) {
				set_option_value(option, value);//storage it.
			}
		}
	}

	menu_cnts = j + 1;
	return (j + 1) > 0 ? 0 : -1;
}

enum BOOT_DEV_TYPE {
	BOOT_DEV_UNKNOWN = 0,
	BOOT_DEV_SATA,
	BOOT_DEV_USB,
	BOOT_DEV_TFTP,
	BOOT_DEV_MMC,

	BOOT_DEV_COUNT,			/* Number of dev types */
};

static const char *boot_devtype_str[BOOT_DEV_COUNT] = {
	[BOOT_DEV_SATA]	= "scsi",
	[BOOT_DEV_USB]	= "usb",
	[BOOT_DEV_TFTP]	= "tftpboot",
	[BOOT_DEV_MMC]	= "mmc",
};

enum PARTITION_FORMAT {
	EXT4 = 0,
	FAT,
	ISO,

	PARTITION_COUNT,			/* Number of partition */
};

static const char *partition_format_str[PARTITION_COUNT] = {
	[EXT4]	= "ext4",
	[FAT]	= "fat",
	[ISO]	= "iso",
};

#if 0
/*
@breif: 获取设备号与分区号
@dev_id[out]: 设备号
@partition_id[out] : 分区号
@return: 设备类型
*/
static int get_index_of_dev_and_partition(char *str, int *dev_id, int *partition_id)
{
	char *p = NULL;
	int dev_type = 0;

	*dev_id = 0;
	*partition_id = 1;

	p = strchr(str, '(');
	if (p) {
		p++;
	} else if (strcasecmp(str, "/dev/") != 0) {
		p = strchr(str, '@');
		if (p) {
			p++;
		}
	}

	if (p != NULL) {
		char buf[8] = {0};
		int i;

		memset(buf, 0, sizeof(buf));
		for (i=0; i<sizeof(buf); i++) {
			buf[i] = *p;
			p++;
			if (*p <= '9') {
				if (strcasecmp(buf,"wd") == 0) {
					dev_type = BOOT_DEV_SATA;
				} else if (strcasecmp(buf,"usb") == 0) {
					dev_type = BOOT_DEV_USB;
				} else if (strcasecmp(buf,"cd") == 0) {
					dev_type = BOOT_DEV_SATA;
				}
				break;
			}
		}

		memset(buf, 0, sizeof(buf));
		for (i=0; i<sizeof(buf); i++) {
			buf[i] = *p;
			p++;
			if ((*p == ',') || (*p == '/')) {
				int num;
				strict_strtoul(buf, 16, (unsigned long *)&num);
				*dev_id = num;
				break;
			}
		}
		p++;

		if ((*p < '0') || (*p > '9')) {
			return dev_type;
		}

		memset(buf, 0, sizeof(buf));
		for (i=0; i<sizeof(buf); i++) {
			buf[i] = *p;
			p++;
			if (*p == ')') {
				int num;
				strict_strtoul(buf, 16, (unsigned long *)&num);
				*partition_id = num + 1;
				break;
			}
		}
	}

	return dev_type;
}
#endif

static int load_cfg_file_command_run(int format, int boot_dev_type, int dev, int part, unsigned long cfg_base, int *part_e)
{
	int ret, path;
	char cmd[1024];
	char *path_str[2] = {"/boot/", "/"};

	if (!part)
		return -2;

	for (path = 0; path < 2; ++path) {
		part_e[0] = part;
		sprintf(cmd, "%sload %s %d:%d %lx %sboot.cfg",
			partition_format_str[format], boot_devtype_str[boot_dev_type],
			dev, part, cfg_base, path_str[path]);
		ret = run_command(cmd, 0);

		if (ret && part == 1) {
			part_e[0] = 0;
			sprintf(cmd, "%sload %s %d %lx %sboot.cfg",
						partition_format_str[format], boot_devtype_str[boot_dev_type],
						dev, cfg_base, path_str[path]);
			ret = run_command(cmd, 0);
		}

		if (!ret)
			return ret;
	}
	return -1;
}

/* 返回分区格式 0:ext2/3/4  1:fat32 */
static int load_cfg_file(int boot_dev_type, unsigned long cfg_base, int *partition_format, int *dev_e, int *part_e)
{
	int ret = -1;
	int format;
	int dev, partition;
	char cmd[1024];

	if (!partition_format || !dev_e || !part_e)
		return -2;

	switch (boot_dev_type) {
	case BOOT_DEV_SATA:
		run_command("scsi reset", 0);
		ret = run_command("scsi devcie", 0);
		break;
	case BOOT_DEV_USB:
		run_command("usb reset", 0);
		ret = run_command("usb storage", 0);
		break;
	case BOOT_DEV_MMC:
		ret = run_command("mmc rescan", 0);
		break;
	case BOOT_DEV_TFTP: {
		sprintf(cmd, "tftpboot %lx /boot.cfg", cfg_base);
		ret = run_command(cmd, 0);
		return ret;
	}
	default:
		break;
	}

	if (ret != 0) {
		printf("init device fail. boot_dev_type:%s\n", boot_devtype_str[boot_dev_type]);
		return ret;
	}

	for (dev = 0; dev < MAX_DEVS; dev++) {
		for (partition = 1; partition < MAX_PARTITIONS; partition++) {
			switch (boot_dev_type) {
			case BOOT_DEV_SATA: {
				format = EXT4;
				ret = load_cfg_file_command_run(format, boot_dev_type, dev, partition, cfg_base, part_e);
				break;
			}
			case BOOT_DEV_USB:
			case BOOT_DEV_MMC: {
				printf("test disk as iso partition...\n");
				format = ISO;
				ret = load_cfg_file_command_run(format, boot_dev_type, dev, partition, cfg_base, part_e);
				if (ret) {
					printf("test disk as FAT partition...\n");
					format = FAT;
					ret = load_cfg_file_command_run(format, boot_dev_type, dev, partition, cfg_base, part_e);
				}
				break;
			}
			default:
				break;
			}
			if (!ret) {
				*dev_e = dev;
				*partition_format = format;
				return 0;
			}
		}
	}

	return ret;
}

struct cfgmenu_entry {
	unsigned short int num;		/* unique number 0 .. MAX_COUNT */
	char key[3];			/* key identifier of number */
	char *title;			/* title of entry */
	char *kernel;			/* hush kernel of entry */
	char *initrd;			/* hush initrd of entry */
	char *args;			/* hush args of entry */
	struct cfgmenu_data *menu;	/* this cfgmenu */
	struct cfgmenu_entry *next;	/* next menu entry (num+1) */
};

struct cfgmenu_data {
	int delay;			/* delay for autoboot */
	int active;			/* active menu entry */
	int count;			/* total count of menu entries */
	struct cfgmenu_entry *first;	/* first menu entry */
};

enum cfgmenu_key {
	KEY_NONE = 0,
	KEY_UP,
	KEY_DOWN,
	KEY_SELECT,
};

static void cfgmenu_print_entry(void *data)
{
	struct cfgmenu_entry *entry = data;
	int reverse = (entry->menu->active == entry->num);

	/*
	 * Move cursor to line where the entry will be drown (entry->num)
	 * First 3 lines contain cfgmenu header + 1 empty line
	 */
	printf(ANSI_CURSOR_POSITION, entry->num + 4, 1);

	puts("     ");

	if (reverse)
		puts(ANSI_COLOR_REVERSE);

#ifdef CONFIG_MENU_HAVE_NUM
	{
	char buf[8];
	char num = entry->num + 1;
	num = num > 9 ? (num - 10 + 0x61) : (num + 0x30);
	sprintf(buf, "[%c] ", num);
	puts(buf);
	}
#endif

	puts(entry->title);

	if (reverse) {
		puts(ANSI_COLOR_RESET);

		int pos = entry->menu->count + 7;
		printf(ANSI_CURSOR_POSITION, pos, 1);
		puts(ANSI_CLEAR_LINE);
		if (entry->kernel[0] != '\0') {
			puts("     kernel:");
			puts(entry->kernel);
		}
		puts(ANSI_CLEAR_LINE_TO_END);

		pos += 1;
		printf(ANSI_CURSOR_POSITION, pos, 1);
		puts(ANSI_CLEAR_LINE);
		if (entry->initrd[0] != '\0') {
			puts("     initrd:");
			puts(entry->initrd);
		}
		puts(ANSI_CLEAR_LINE_TO_END);

		pos += 1;
		printf(ANSI_CURSOR_POSITION, pos, 1);
		puts(ANSI_CLEAR_LINE);
		if (entry->args[0] != '\0') {
			puts("     args:");
			puts(entry->args);
		}
		puts(ANSI_CLEAR_LINE_TO_END);
	}
}

static void cfgmenu_autoboot_loop(struct cfgmenu_data *menu,
				enum cfgmenu_key *key, int *esc)
{
	int i, c;

	if (menu->delay > 0) {
		printf(ANSI_CURSOR_POSITION, menu->count + 5, 1);
		printf("  Hit any key to stop autoboot: %2d ", menu->delay);
	}

	while (menu->delay > 0) {
		for (i = 0; i < 100; ++i) {
			if (!tstc()) {
				schedule();
				mdelay(10);
				continue;
			}

			menu->delay = -1;
			c = getchar();

			switch (c) {
			case '\e':
				*esc = 1;
				*key = KEY_NONE;
				break;
			case '\r':
				*key = KEY_SELECT;
				break;
			default:
				*key = KEY_NONE;
				break;
			}

			break;
		}

		if (menu->delay < 0)
			break;

		--menu->delay;
		printf("\b\b\b%2d ", menu->delay);
	}

	printf(ANSI_CURSOR_POSITION, menu->count + 5, 1);
	puts(ANSI_CLEAR_LINE);

	if (menu->delay == 0)
		*key = KEY_SELECT;
}

static void cfgmenu_loop(struct cfgmenu_data *menu,
		enum cfgmenu_key *key, int *esc)
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

static char *cfgmenu_choice_entry(void *data)
{
	struct cfgmenu_data *menu = data;
	struct cfgmenu_entry *iter;
	enum cfgmenu_key key = KEY_NONE;
	int esc = 0;
	int i;

	while (1) {
		if (menu->delay >= 0) {
			/* Autoboot was not stopped */
			cfgmenu_autoboot_loop(menu, &key, &esc);
		} else {
			/* Some key was pressed, so autoboot was stopped */
			cfgmenu_loop(menu, &key, &esc);
		}

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
	debug("cfgmenu: this should not happen");
	return NULL;
}

static void cfgmenu_destroy(struct cfgmenu_data *menu)
{
	struct cfgmenu_entry *iter = menu->first;
	struct cfgmenu_entry *next;

	while (iter) {
		next = iter->next;
		if (iter->title)
			free(iter->title);
		if (iter->kernel)
			free(iter->kernel);
		if (iter->initrd)
			free(iter->initrd);
		if (iter->args)
			free(iter->args);
		free(iter);
		iter = next;
	}
	free(menu);
}


static struct cfgmenu_data *cfgmenu_create(int delay)
{
	unsigned short int i = 0;
	struct cfgmenu_data *menu;
	struct cfgmenu_entry *iter = NULL;

	int len;
	struct cfgmenu_entry *entry;

	menu = malloc(sizeof(struct cfgmenu_data));
	if (!menu)
		return NULL;

	menu->delay = delay;
	menu->active = 0;
	menu->first = NULL;

	for (i=0; i<menu_cnts; i++) {
		entry = malloc(sizeof(struct cfgmenu_entry));
		if (!entry)
			goto cleanup;
		memset(entry, 0, sizeof(struct cfgmenu_entry));

		len = strlen(menu_items[i].title);
		entry->title = malloc(len + 1);
		if (!entry->title) {
			free(entry);
			goto cleanup;
		}
		memcpy(entry->title, menu_items[i].title, len);
		entry->title[len] = 0;

		len = strlen(menu_items[i].kernel);
		entry->kernel = malloc(len + 1);
		if (!entry->kernel) {
			free(entry);
			goto cleanup;
		}
		memcpy(entry->kernel, menu_items[i].kernel, len);
		entry->kernel[len] = 0;

		len = strlen(menu_items[i].initrd);
		entry->initrd = malloc(len + 1);
		if (!entry->initrd) {
			free(entry);
			goto cleanup;
		}
		memcpy(entry->initrd, menu_items[i].initrd, len);
		entry->initrd[len] = 0;

		len = strlen(menu_items[i].args);
		entry->args = malloc(len + 1);
		if (!entry->args) {
			free(entry);
			goto cleanup;
		}
		memcpy(entry->args, menu_items[i].args, len);
		entry->args[len] = 0;

		sprintf(entry->key, "%d", i);

		entry->num = i;
		entry->menu = menu;
		entry->next = NULL;

		if (!iter)
			menu->first = entry;
		else
			iter->next = entry;

		iter = entry;
	}

	if (i <= MAXARGS - 1) {
		entry = malloc(sizeof(struct cfgmenu_entry));
		if (!entry)
			goto cleanup;
		memset(entry, 0, sizeof(struct cfgmenu_entry));
#ifndef BOOTMENU_END
#define BOOTMENU_END "U-Boot console"
#endif
		entry->title = strdup(BOOTMENU_END);
		if (!entry->title) {
			free(entry);
			goto cleanup;
		}

		entry->kernel = strdup("");
		if (!entry->kernel) {
			free(entry->title);
			free(entry);
			goto cleanup;
		}

		entry->initrd = strdup("");
		if (!entry->initrd) {
			free(entry->title);
			free(entry->kernel);
			free(entry);
			goto cleanup;
		}

		entry->args = strdup("");
		if (!entry->args) {
			free(entry->title);
			free(entry->kernel);
			free(entry->initrd);
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
	cfgmenu_destroy(menu);
	return NULL;
}

static void cfgmenu_display_statusline(struct menu *m)
{
	struct cfgmenu_data *menu;
	struct cfgmenu_entry *entry;

	if (menu_default_choice(m, (void *)&entry) < 0)
		return;

	menu = entry->menu;

	printf(ANSI_CURSOR_POSITION, 1, 1);
	puts(ANSI_CLEAR_LINE);
	printf(ANSI_CURSOR_POSITION, 2, 1);
	puts("  *** Boot Cfg Menu ***");
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

static int cfgmenu_show(int delay)
{
	int init = 0;
	void *choice = NULL;
	char *title = NULL;
	struct menu *menu;
	struct cfgmenu_data *cfgmenu;
	struct cfgmenu_entry *iter;
	int index = 0;

	cfgmenu = cfgmenu_create(delay);
	if (!cfgmenu)
		return index;

	menu = menu_create(NULL, cfgmenu->delay, 1, cfgmenu_display_statusline, cfgmenu_print_entry,
			   cfgmenu_choice_entry, cfgmenu);
	if (!menu) {
		cfgmenu_destroy(cfgmenu);
		return index;
	}

	for (iter = cfgmenu->first; iter; iter = iter->next) {
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
		index = iter->num;
		title = strdup(iter->title);
	}

cleanup:
	menu_destroy(menu);
	cfgmenu_destroy(cfgmenu);

	if (init) {
		puts(ANSI_CURSOR_SHOW);
		puts(ANSI_CLEAR_CONSOLE);
		printf(ANSI_CURSOR_POSITION, 1, 1);
	}

	if (title) {
		debug("Starting entry '%s'\n", title);
		free(title);
	}

	return index;
}

static void do_boot_pre_init(int partition_format, int boot_dev_type)
{
	if (partition_format == ISO) {
		if (boot_dev_type == BOOT_DEV_USB) {
			run_command("usb reset", 0);
			run_command("usb storage", 0);
		}
		if (boot_dev_type == BOOT_DEV_MMC)
			run_command("mmc rescan", 0);
	}
}

static int do_boot(int index, int partition_format, int boot_dev_type, int dev, int part)
{
	char cmd[1024];
	char* bootcmd_buf;
	unsigned long addr;
	int ret = 0;

	switch (boot_dev_type) {
	case BOOT_DEV_SATA:
	case BOOT_DEV_USB:
	case BOOT_DEV_MMC:
		if (menu_items[index].kernel && menu_items[index].kernel[0] != '\0') {
			/*
			 * get_index_of_dev_and_partition 这个 对于有的 ISO 里面的 bootcfg 会导致 uboot 崩溃
			 * 所以规定认为， initrd 和 内核 本身应该在 /boot/之下
			 * get_index_of_dev_and_partition 这个函数 使用 宏定义 #if 0 禁用了。
			 * 参数 dev part 是 在 load_cfg_file 的时候找到的。
			 */
			// get_index_of_dev_and_partition(menu_items[index].kernel, &dev_id, &partition_id);
			if (part)
				sprintf(cmd, "%sload %s %d:%d ${loadaddr} %s", partition_format_str[partition_format], boot_devtype_str[boot_dev_type],
						dev, part, remove_pmon_dev(menu_items[index].kernel));
			else
				sprintf(cmd, "%sload %s %d ${loadaddr} %s", partition_format_str[partition_format], boot_devtype_str[boot_dev_type],
						dev, remove_pmon_dev(menu_items[index].kernel));

			do_boot_pre_init(partition_format, boot_dev_type);
			ret += run_command(cmd, 0);
		}
		if (menu_items[index].initrd && menu_items[index].initrd[0] != '\0') {
			// get_index_of_dev_and_partition(menu_items[index].initrd, &dev_id, &partition_id);
			if (part)
				sprintf(cmd, "%sload %s %d:%d ${rd_start} %s", partition_format_str[partition_format], boot_devtype_str[boot_dev_type],
					dev, part, remove_pmon_dev(menu_items[index].initrd));
			else
				sprintf(cmd, "%sload %s %d ${rd_start} %s", partition_format_str[partition_format], boot_devtype_str[boot_dev_type],
					dev, remove_pmon_dev(menu_items[index].initrd));

			do_boot_pre_init(partition_format, boot_dev_type);
			ret += run_command(cmd, 0);
			run_command("setenv rd_size 0x${filesize}", 0);
		}
		break;
	case BOOT_DEV_TFTP:
		if (menu_items[index].kernel && menu_items[index].kernel[0] != '\0') {
			sprintf(cmd, "tftpboot ${loadaddr} %s", menu_items[index].kernel);
			ret += run_command(cmd, 0);
		}
		if (menu_items[index].initrd && menu_items[index].initrd[0] != '\0') {
			sprintf(cmd, "tftpboot ${rd_start} %s", menu_items[index].initrd);
			ret += run_command(cmd, 0);
			run_command("setenv rd_size 0x${filesize}", 0);
		}
		break;
	default:
		break;
	}

	if (menu_items[index].initrd && menu_items[index].initrd[0] != '\0') {
		sprintf(cmd, "setenv bootargs rd_start=${rd_start} rd_size=${rd_size} %s", menu_items[index].args);
	} else {
		sprintf(cmd, "setenv bootargs %s", menu_items[index].args);
	}

	run_command(cmd, 0);

	//认为无需改写 fdt_addr
	//run_command("setenv fdt_addr 0x8a000000;sf probe;sf read 0x8a000000 dtb", 0);

	switch (boot_dev_type) {
	case BOOT_DEV_SATA:
		//通过配置环境变量bootcmd，把默认启动项配置成bootcfg scsi
		bootcmd_buf = env_get("bootcmd");
		if (strcmp(bootcmd_buf, "bootcfg scsi")) {
			env_set("bootcmd", "bootcfg scsi");
			env_save();
		}
		break;
	}

	addr = simple_strtoul(env_get("loadaddr"), NULL, 16);
	if (valid_elf_image(addr)) {
		run_command("bootelf ${loadaddr} bootparamls_fixup", 0); //linux-4.19
//		run_command("bootelf ${loadaddr} bootparamls", 0);//linux-5.10
	} else {
		run_command("bootm ${loadaddr}", 0);
	}

	// 发现 上面的 bootelf 有时会执行了也不启动内核， 所以使用boot命令强制启动 前提是 内核 initrd成功加载
	if (!ret)
		run_command("boot", 0);

	return 0;
}

int do_bootcfg(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int i, ret = 0;
	unsigned long cfg_base = CONFIG_SYS_LOAD_ADDR;
	int boot_dev_type = BOOT_DEV_UNKNOWN;
	int partition_format = EXT4;
	int dev = 0, part = 0;

	if (argc > 1) {
		for (i = 1; i < BOOT_DEV_COUNT; i++) {
			if (!strcmp(argv[1], boot_devtype_str[i])) {
				boot_dev_type = i;
				break;
			}
		}
	}

	if (boot_dev_type == BOOT_DEV_UNKNOWN) {
		printf("#############################################################\n");
		printf("### bootcfg dev unknown, use scsi dev default\r\n");
		printf("### please intput dev: scsi\\usb\\tftpboot\\mmc\r\n");
		printf("### cur run as 'bootcfg scsi'\n");
		printf("#############################################################\n");
		boot_dev_type = BOOT_DEV_SATA;
	}

	if (argc < 3) {
		ret = load_cfg_file(boot_dev_type, cfg_base, &partition_format, &dev, &part);
		if (ret) {
			printf("failed to load boot.cfg file!\r\n");
			goto err;
		}
	} else {
		strict_strtoul(argv[2], 16, &cfg_base);
	}

	ret = menu_list_read((char *)cfg_base, 0);
	if (ret == 0) {
		unsigned long index = 0;
		unsigned long timeout = 0;

		strict_strtoul(get_option_value("timeout"), 10, &timeout);
		strict_strtoul(get_option_value("default"), 10, &index);

		if (timeout > 0 )
			index = cfgmenu_show(timeout);

		if (index == menu_cnts) {		//最后一项为返回uboot 控制台
			run_command("", 0);
		} else {
			do_boot(index, partition_format, boot_dev_type, dev, part);
		}
	}

	for (i=0; i<menu_cnts; i++) {
		free(menu_items[i].kernel);
		free(menu_items[i].args);
		free(menu_items[i].initrd);
		free(menu_items[i].root);
	}

	return 0;
err:
	return -1;
}

U_BOOT_CMD(
	bootcfg,    3,    1,     do_bootcfg,
	"boot kernel from read boot.cfg file over scsi\\usb\\tftpboot\\mmc.",
	""
);

