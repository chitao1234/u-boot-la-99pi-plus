/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __LOONGSON_ENV_H__
#define __LOONGSON_ENV_H__

#define CMDLINE_CONSOLE		"console=ttyS0,115200"

#ifdef CONFIG_64BIT
#define FDT_ADDR    0x900000000a000000
#define RD_ADDR     0x9000000007000000

#if RD_ADDR >= LOCK_CACHE_BASE && RD_ADDR < (LOCK_CACHE_BASE + LOCK_CACHE_SIZE)
#error should adjust RD_ADDR because conflict with LOCK_CACHE_BASE and SIZE (asm/addrspace.h)
#endif

#else
#define FDT_ADDR    0x0a000000
#define RD_ADDR     0x07000000
#endif

#define RD_SIZE		0x2000000 /* ramdisk size:32M == 32768K*/

/*dtb size: 64K*/
#define FDT_SIZE	0x10000

#define LOONGSON_BOOTMENU \
	"menucmd=bootmenu\0" \
	"bootmenu_0=System boot select=updatemenu bootselect 1\0" \
	"bootmenu_1=Update kernel=updatemenu kernel 1\0" \
	"bootmenu_2=Update rootfs=updatemenu rootfs 1\0" \
	"bootmenu_3=Update u-boot=updatemenu uboot 1\0" \
	"bootmenu_4=Update ALL=updatemenu all 1\0" \
	"bootmenu_5=System install or recover=updatemenu system 1\0" \
	"bootmenu_6=Board product=updatemenu product 1\0"

#if !defined(CONFIG_VIDEO)
#define LOONGSON_BOOTMENU_VIDEO \
	"bootmenu_7=Video resolution select=updatemenu resolution 1\0" \
	"bootmenu_8=Video rotation select=updatemenu rotation 1\0"
#else
#define LOONGSON_BOOTMENU_VIDEO
#endif

#define LOONGSON_BOOTMENU_DELAY "bootmenu_delay=10\0"

#define BOOTMENU_END "Return u-boot console"

#ifdef CONFIG_BUTTON_GPIO
#define STDIN_GPIOBTN "gpiobtn"
#else
#define STDIN_GPIOBTN ""
#endif

#ifdef CONFIG_VIDEO
#define CONSOLE_STDOUT_SETTINGS \
	"splashimage=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"stdin=serial,"STDIN_GPIOBTN",usbkbd\0" \
	"stdout=serial\0" \
	"stderr=serial,vidconsole,vidconsole1\0"
#else
#define CONSOLE_STDOUT_SETTINGS \
	"stdin=serial,"STDIN_GPIOBTN"\0" \
	"stdout=serial\0" \
	"stderr=serial\0"
#endif

#define RECOVER_FRONT_BOOTARGS "setenv bootargs " CMDLINE_CONSOLE " rd_start=${rd_start} rd_size=${rd_size} \
mtdparts=${mtdparts} root=/dev/ram init=/linuxrc rw rootfstype=ext2 fbcon=rotate:${rotate} panel=${panel};"

#define RECOVER_START "bootm ${loadaddr}"

#define RECOVER_USB_DEFAULT "usb reset;fatload usb 0:1 ${loadaddr} /install/uImage;fatload usb 0:1 ${rd_start} /install/ramdisk.gz;"\
RECOVER_FRONT_BOOTARGS "setenv bootargs ${bootargs} ins_way=usb;" RECOVER_START

#define RECOVER_MMC_DEFAULT "mmc rescan;fatload mmc 0:1 ${loadaddr} /install/uImage;fatload mmc 0:1 ${rd_start} /install/ramdisk.gz;"\
RECOVER_FRONT_BOOTARGS "setenv bootargs ${bootargs} ins_way=mmc;" RECOVER_START

#define RECOVER_TFTP_DEFAULT "tftpboot ${loadaddr} uImage;tftpboot ${rd_start} ramdisk.gz;"\
RECOVER_FRONT_BOOTARGS "setenv bootargs ${bootargs} ins_way=tftp u_ip=${ipaddr} u_sip=${serverip};" RECOVER_START

#ifdef CONFIG_LINUX_KERNEL_LOG_CLOSE
#define LINUX_KERNEL_LOG_LEVEL_SETUP " loglevel=0"
#else
#define LINUX_KERNEL_LOG_LEVEL_SETUP ""
#endif

#define SATA_BOOT_ENV "setenv bootargs " CMDLINE_CONSOLE LINUX_KERNEL_LOG_LEVEL_SETUP " rootfstype=ext4 rw rootwait; \
setenv bootcmd ' setenv bootargs ${bootargs} root=/dev/sda${syspart} mtdparts=${mtdparts} fbcon=rotate:${rotate} panel=${panel}; \
sf probe;sf read ${fdt_addr} dtb;scsi reset;ext4load scsi 0:${syspart} ${loadaddr} /boot/uImage;bootm ';\
saveenv;"

#define BOOT_SATA_DEFAULT SATA_BOOT_ENV"boot"

#define NAND_BOOT_ENV "setenv bootargs " CMDLINE_CONSOLE LINUX_KERNEL_LOG_LEVEL_SETUP " root=ubi0:rootfs noinitrd init=/linuxrc rootfstype=ubifs rw; \
setenv bootcmd ' setenv bootargs ${bootargs} ubi.mtd=root,${nand_pagesize} mtdparts=${mtdparts} fbcon=rotate:${rotate} panel=${panel}; \
sf probe;sf read ${fdt_addr} dtb;nboot kernel;bootm ';\
saveenv;"

#define BOOT_NAND_DEFAULT NAND_BOOT_ENV"boot"

#define MMC_BOOT_ENV "setenv bootargs " CMDLINE_CONSOLE LINUX_KERNEL_LOG_LEVEL_SETUP " root=/dev/mmcblk0 noinitrd init=/linuxrc rootfstype=ext4 rw rootwait; \
setenv bootcmd ' setenv bootargs ${bootargs} mtdparts=${mtdparts}; \
ext4load mmc 0 ${loadaddr} /boot/uImage;bootm ';\
saveenv;"

#define BOOT_MMC_DEFAULT MMC_BOOT_ENV"boot"

#define BOOT_SATA_CFG_DEFAULT "setenv bootcmd ' bootcfg scsi ';\
saveenv;\
boot"

#define BOOT_USB_CFG_DEFAULT "setenv bootcmd ' bootcfg usb ';\
saveenv;\
boot"

#ifdef CONFIG_AHCI
#define LS_DOUBLE_SYSTEM
#endif

#define	CFG_EXTRA_ENV_SETTINGS					\
	CONSOLE_STDOUT_SETTINGS \
	LOONGSON_BOOTMENU \
	LOONGSON_BOOTMENU_VIDEO \
	LOONGSON_BOOTMENU_DELAY \
	"nand_pagesize=2048\0" \
	"loadaddr=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" 		\
	"fdt_addr=" __stringify(FDT_ADDR) "\0" 					\
	"fdt_size=" __stringify(FDT_SIZE) "\0" 					\
	"rd_start=" __stringify(RD_ADDR) "\0" 					\
	"rd_size=" __stringify(RD_SIZE) "\0" 					\
	"mtdids=" CONFIG_MTDIDS_DEFAULT "\0"					\
	"mtdparts=" CONFIG_MTDPARTS_DEFAULT "\0"				\
	"splashpos=m,m\0" \
	"panel=default\0" \
	"rotate=0\0" \
	"syspart=1\0" \
	"syspart_last=4\0" \
	"syspart_ch=0\0"

#define CONFIG_IPADDR		192.168.1.20
#define CONFIG_NETMASK		255.255.255.0
#define CONFIG_SERVERIP		192.168.1.2

#endif /* __LOONGSON_ENV_H__ */


