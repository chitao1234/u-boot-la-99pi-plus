// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <stdlib.h>
#include <mmc.h>
#include <scsi.h>

#include "general_load.h"

typedef struct gl_arg_s {
	gl_device_t* device;
	gl_format_t* fmt;
	int fstype;
} gl_arg_t;

static int gl_get_devnum_from_part(char* part)
{
	char* p;
	int idx;

	for (p = part; *p != 0x00; p++)
	{
		if (*p == ':' || *p < '0' || *p > '9')
			break;
		idx = idx * 10 + (*p - '0');
	}

	return idx;
}

static struct blk_desc* gl_mmc_init(int devnum)
{
	return blk_get_devnum_by_uclass_id(UCLASS_MMC, devnum);
}

static struct blk_desc* gl_scsi_init(int devnum)
{
#ifdef CONFIG_SCSI
	int ret;
#ifndef CONFIG_DM_SCSI
	scsi_bus_reset(NULL);
#endif
	ret = scsi_scan(true);
	if (ret)
		return NULL;
#endif
	return blk_get_devnum_by_uclass_id(UCLASS_SCSI, devnum);
}

// [--from xxx [--fs xxx]] [--to xxx [--fs xxx]]
static int cmd_gl_parse_arg(char** arg, int n,
		gl_target_t* src, gl_target_t* dest, enum gl_extra_e* extra)
{
	char** a = arg;
	gl_target_t* t;

parse_opt:
	if (a > arg + (n-1) )
		return 0;

	if (strcmp(*a, "--if") == 0) {
		t = src;
		a++;
		goto parse_device;
	}
	else if (strcmp(*a, "--of") == 0) {
		t = dest;
		a++;
		goto parse_device;
	}
	else if ((strcmp(*a, "--fmt") == 0)) {
		a++;
		goto parse_fmt;
	}
	else if ((strcmp(*a, "--sym") == 0)) {
		a++;
		goto parse_symbol;
	}
	else if ((strcmp(*a, "--decompress") == 0)) {
		goto indicate_decompress;
	}
	else {
		printf("Error --opt unknown %s\n", *a);
		return -1;
	}

parse_device:
	if (a > arg + (n-1) )
	{
		printf("Error: --from/to need parameter\n");
		return -1;
	}
	if (strncmp(*a, "net", 3) == 0) {
		t->gl_device.type = GL_DEVICE_NET;
	}
	else if (strncmp(*a, "mmc", 3) == 0) {
		t->gl_device.type = GL_DEVICE_BLK;
		memcpy(t->gl_device.interface, *a, 3);
		strncpy(t->gl_device.part, (*a) + 3,
				sizeof(t->gl_device.part));
		t->gl_device.desc = gl_mmc_init(
				gl_get_devnum_from_part(t->gl_device.part));
	}
	else if (strncmp(*a, "usb", 3) == 0) {
		t->gl_device.type = GL_DEVICE_BLK;
		memcpy(t->gl_device.interface, *a, 3);
		strncpy(t->gl_device.part, (*a) + 3,
				sizeof(t->gl_device.part));
	}
	else if (strncmp(*a, "scsi", 4) == 0) {
		t->gl_device.type = GL_DEVICE_BLK;
		memcpy(t->gl_device.interface, *a, 4);
		strncpy(t->gl_device.part, (*a) + 4,
				sizeof(t->gl_device.part));
		t->gl_device.desc = gl_scsi_init(
				gl_get_devnum_from_part(t->gl_device.part));
	}
	else if	(strncmp(*a, "nand", 4) == 0) {
		t->gl_device.type = GL_DEVICE_BLK;
		memcpy(t->gl_device.interface, *a, 4);
		strncpy(t->gl_device.part, (*a) + 4,
				sizeof(t->gl_device.part));
	}
	else {
		printf("Error interface unknown %s\n", *a);
		return -1;
	}
	a++;
	goto parse_opt;

parse_fmt:
	if (a > arg + (n-1) )
	{
		printf("Error: --fs need parameter\n");
		return 0;
	}
	if (strcmp(*a, "tftp") == 0)
	{
		t->gl_format.proto = TFTPGET;
	}
	else if (strcmp(*a, "ext4") == 0) {
		t->gl_format.fstype = FS_TYPE_EXT;
	}
	else if (strcmp(*a, "vfat") == 0 ||
			strcmp(*a, "fat32") == 0 ||
			strcmp(*a, "fat") == 0) {
		t->gl_format.fstype = FS_TYPE_FAT;
	}
	else {
		printf("Error fs unknown %s\n", *a);
		return -1;
	}
	a++;
	goto parse_opt;

parse_symbol:
	if (a > arg + (n-1) )
	{
		printf("Error: --sym need parameter\n");
		return 0;
	}
	strncpy(t->symbol, *a, sizeof(t->symbol));
	a++;
	goto parse_opt;

indicate_decompress:
	*extra |= GL_EXTRA_DECOMPRESS;
	a++;
	goto parse_opt;
}

static int do_general_load(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	gl_target_t src = {0};
	gl_target_t dest = {0};
	enum gl_extra_e extra;

	if(argc == 1)
		return -1;

	if (cmd_gl_parse_arg((char**)argv + 1, argc - 1,
				&src, &dest, &extra))
		return ret;

	ret = general_load(&src, &dest, extra);

	return ret;
}

#define GENERAL_LOAD_CMD_TIP "Load file from device to device.\n"\
	"general_load [--if <device> [--fmt <ext4/tftp> --sym <path/name>]] [--of <device> [--fmt <ext4> --sym <path/name>]] [--decompress]\n"\
	"general_load --if net --fmt tftp --sym /uImage --of mmc0:1 --fmt ext4 --sym /boot/uImage\n"\
	"general_load --if usb:0 --fmt fat --sym /update/uImage --of mmc0:1 --fmt ext4 --sym /boot/uImage\n"\
	"general_load --if usb:0 --fmt fat --sym /update/rootfs.ext4.gz --of mmc0:2\n"\
	"\n"


// load [--if <device> [--fmt <ext4/tftp> --sym <path/name>]] [--of <device> [--fmt <ext4> --sym <path/name>]] [--decompress]
//	--if	设备，例如 usb0:1 mmc0 net
// 	--fmt	不指定，默认纯数据读写
// 	--sym	文件路径
// 	--decompress	gunzip 解压
U_BOOT_CMD(
	general_load,    14,    0,     do_general_load,
	"general load from xxx to xxx",
	GENERAL_LOAD_CMD_TIP
);


