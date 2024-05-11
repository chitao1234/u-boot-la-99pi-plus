/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __UPDATE_H__
#define __UPDATE_H__

#define UPDATEMENU_RETURN_CONSOLE 0
#define UPDATEMENU_RETURN_MENU 1

extern int updatemenu_type;
extern int updatemenu_return;
void updatemenu_show(void);

enum update_dev {
	UPDATE_DEV_UNKNOWN = 0,
	UPDATE_DEV_USB,
	UPDATE_DEV_TFTP,
	UPDATE_DEV_MMC,
	UPDATE_DEV_DHCP,

	UPDATE_DEV_COUNT,			/* Number of dev types */
};

/* Update types: */
enum update_type {
	UPDATE_TYPE_UNKNOWN = 0,
	UPDATE_TYPE_KERNEL,
	UPDATE_TYPE_ROOTFS,
	UPDATE_TYPE_UBOOT,
	UPDATE_TYPE_DTB,
	UPDATE_TYPE_SYSTEM,
	UPDATE_TYPE_ALL,

	UPDATE_TYPE_BOOTSELECT,
	UPDATE_TYPE_RESOLUTION,
	UPDATE_TYPE_ROTATION,

	UPDTAE_TYPE_PRODUCT,

	UPDATE_TYPE_COUNT,			/* Number of update types */
};

void get_resolution(void);
void update_resolution(char *command, int num);

#endif
