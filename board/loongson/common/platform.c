// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <linux/sizes.h>
#include <asm/global_data.h>
#include <asm/sections.h>
#include <asm/addrspace.h>
#include <dm.h>
#include <mapmem.h>
#include <video_console.h>
#include <usb.h>
#include <image.h>
#include <mach/loongson.h>
#include <linux/delay.h>
#include <sound.h>
#include "bdinfo/bdinfo.h"
#include <nand.h>
#include <env.h>
#include <net.h>
#include <phy.h>
#include <ansi.h>
#include <display_options.h>

#include "loongson_stdout_operation.h"
#include "loongson_board_info.h"

#define NOTICE_STR1 "c to enter u-boot console"
#define NOTICE_STR2 "m to enter boot menu"

#define ANSI_CURSOR_SAVE		"\e7"
#define ANSI_CURSOR_RESTORE		"\e8"

DECLARE_GLOBAL_DATA_PTR;

extern int multi_boards_check_store(void);

static const char* bdname;
static void find_bdname(void);

ulong board_get_usable_ram_top(ulong total_size)
{
	phys_addr_t ram_top = gd->ram_top;

	// U-boot 阶段仅使用最低的 256MB 内存，以便兼容不同内存容量的板卡。
	// 0x8F00_0000 ~ 0x8FFF_FFFF (16MB) 保留, 用于固件与内核的信息交互,
	// 具体参考 《龙芯CPU开发系统固件与内核接口详细规范》。
	// 注意：DVO0, DVO1 framebuffer 的保留内存 32MB (0x0D00_0000 ~ 0x0F00_0000),
	// 将在 board_f.c reserve_video() 中保留，此处无需处理。
	if (VA_TO_PHYS(ram_top) >= (phys_addr_t)(MEM_WIN_BASE + SZ_256M - SZ_16M)) {
		ram_top = (phys_addr_t)map_sysmem(MEM_WIN_BASE + SZ_256M - SZ_16M, 0);
	}

#ifdef CONFIG_SPL
	// Keep 4MB space for u-boot image.
	ram_top -= SZ_4M;
	if (CONFIG_TEXT_BASE < ram_top) {
		printf("Warning: Run u-boot from SPL, "
				"but the CONFIG_SYS_TEXT_BASE is out of "
				"the reserved space for u-boot image\n");
	}
#endif

	return ram_top;
}

u32 get_fdt_totalsize(const void *fdt)
{
	int conf, node, fdt_node, images, len;
	const char *fdt_name;
	const u32 *fdt_len;
	u32 totalsize = 0;

	if (fdt_check_header(fdt))
		return 0;

	totalsize = fdt_totalsize(fdt);

	conf = fdt_path_offset(fdt, FIT_CONFS_PATH);
	if (conf < 0) {
		debug("%s: Cannot find /configurations node: %d\n", __func__,
			  conf);
		goto finish;
	}

	images = fdt_path_offset(fdt, FIT_IMAGES_PATH);
	if (images < 0) {
		debug("%s: Cannot find /images node: %d\n", __func__, images);
		goto finish;
	}

	for (node = fdt_first_subnode(fdt, conf);
		node >= 0;
		node = fdt_next_subnode(fdt, node)) {

		fdt_name = fdt_getprop(fdt, node, FIT_FDT_PROP, &len);
		if (!fdt_name)
			continue;

		fdt_node = fdt_subnode_offset(fdt, images, fdt_name);
		if (fdt_node < 0)
			continue;

		fdt_len = fdt_getprop(fdt, fdt_node, "data-size", &len);
		if (!fdt_len || len != sizeof(*fdt_len))
			continue;

		totalsize += ALIGN(fdt32_to_cpu(*fdt_len), 4);
	}

finish:
	debug("fdt total size: %d\n", totalsize);
	return totalsize;
}

#ifdef CONFIG_OF_BOARD
void *board_fdt_blob_setup(int *err)
{
#ifdef CONFIG_SPL
	uint8_t *fdt_dst;
	uint8_t *fdt_src;
	ulong size;

	*err = 0;
	fdt_dst = (uint8_t*)ALIGN((ulong)&__bss_end, ARCH_DMA_MINALIGN);
#ifdef CONFIG_SPL_BUILD
	fdt_src = (uint8_t*)((ulong)&_image_binary_end - (ulong)__text_start +
                        BOOT_SPACE_BASE_UNCACHED);
#else
	fdt_src = (uint8_t*)&_image_binary_end;

	//当fdt段落在bss段里面时，需要把fdt复制到bss外面，因为启动过程中bss段会被清零，
	//当fdt段落在bss段外面时，返回fdt段的地址即可。
	if ((ulong)fdt_src >= (ulong)&__bss_end)
		fdt_dst = fdt_src;
#endif

	if (fdt_dst != fdt_src) {
		size = get_fdt_totalsize(fdt_src);
		memmove(fdt_dst, fdt_src, size);
		gd->fdt_size = size;
	}

	return fdt_dst;
#else
	*err = 0;
	return (void*)gd->fdt_blob;
#endif
}
#endif

#ifdef CONFIG_GPIO_BEEPER
static void beep_boot(void)
{
	struct udevice *dev;

	uclass_get_device_by_name(UCLASS_SOUND, "gpio-beeper", &dev);
	if (!dev) {
		printf("Can not get beeper\n");
		return;
	}

	sound_beep(dev, 300, 20000);
}
#else
static void beep_boot(void) {}
#endif

static void acpi_config(void)
{
#ifndef NOT_USE_ACPI
	volatile u32 val;
	// disable wake on lan
	val = readl(LS_PM_RTC_REG);
	writel(val & ~(0x3 << 7), LS_PM_RTC_REG);

	// disable pcie and rtc wakeup
	val = readl(LS_PM1_EN_REG);
	val &= ~(1 << 10);
	val |= 1 << 14;
	writel(val, LS_PM1_EN_REG);

	// disable usb/ri/gmac/bat/lid wakeup event
	// and enable cpu thermal interrupt.
	writel(0xe, LS_GPE0_EN_REG);

	// clear pm status
	writel(0xffffffff, LS_PM1_STS_REG);
#endif	
}

#ifdef CONFIG_NET
// mac地址来源优先级：env > bdinfo > random
static void ethaddr_setup(void)
{
	uchar bdi_ethaddr[ARP_HLEN];
	uchar env_ethaddr[ARP_HLEN];
	char *bdi_ethaddr_str;
	int id;

	for (id = 0; id < 2; ++id) {
		if (!eth_env_get_enetaddr_by_index("eth", id, env_ethaddr)) {
			if (id == 0)
				bdi_ethaddr_str = bdinfo_get(BDI_ID_MAC0);
			else
				bdi_ethaddr_str = bdinfo_get(BDI_ID_MAC1);

			string_to_enetaddr(bdi_ethaddr_str, bdi_ethaddr);
			if (is_valid_ethaddr(bdi_ethaddr)) {
				memcpy(env_ethaddr, bdi_ethaddr, ARP_HLEN);
			} else {
				net_random_ethaddr(env_ethaddr);
				printf("\neth%d: using random MAC address - %pM\n",
					id, env_ethaddr);
			}
			eth_env_set_enetaddr_by_index("eth", id, env_ethaddr);
		}
	}
}
#endif

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	set_stdout(STDOUT_SERIAL, STDOUT_ONLY_ON);
	acpi_config();
#ifdef CONFIG_NET
	ethaddr_setup();
#endif
	return 0;
}
#endif

/*
 * display uboot version info on lcd
 * if you make with -DDISPLAY_BANNER_ON_VIDCONSOLE it will
 * display it on lcd
 */
static int vidconsole_notice(char *buf)
{
#if defined(CONFIG_VIDEO) && defined(CONFIG_LOONGSON_VIDCONSOLE_NOTICE)
	struct vidconsole_priv *priv;
	struct udevice *con, *vdev = NULL;
	int col, row, len, ret;
	int vidcon_id = 0;
	for (uclass_first_device(UCLASS_VIDEO, &vdev);
				vdev; uclass_next_device(&vdev)) {
		debug("video device: %s\n", vdev->name);
		vidcon_id++;
	}

	if (vidcon_id < 1) {
		debug("vidconsole no exist\n");
		sprintf(buf, "Press %s, %s", NOTICE_STR1, NOTICE_STR2);
		return 0;
	}

	for (uclass_first_device(UCLASS_VIDEO_CONSOLE, &con);
				con; uclass_next_device(&con)) {
		vidconsole_put_string(con, ANSI_CURSOR_SAVE);
		priv = dev_get_uclass_priv(con);
		row = priv->rows - 1;
#ifdef DISPLAY_BANNER_ON_VIDCONSOLE
		display_options_get_banner(false, buf, DISPLAY_OPTIONS_BANNER_LENGTH);
		len = strcspn(buf, "\n");
		buf[len] = 0;
		col = (priv->cols / 2) - (len / 2);
		if (col < 0)
			col = 0;
		vidconsole_position_cursor(con, col, row);
		vidconsole_put_string(con, buf);
		row -= 1;
#endif
		sprintf(buf, "Press %s, %s", NOTICE_STR1, NOTICE_STR2);
		len = strlen(buf);
		col = (priv->cols / 2) - (len / 2);
		if (col < 0)
			col = 0;
		vidconsole_position_cursor(con, col, row);
		vidconsole_put_string(con, buf);
		vidconsole_position_cursor(con, 0, 0);
		vidconsole_put_string(con, ANSI_CURSOR_RESTORE);
	}
#else
	sprintf(buf, "Press %s, %s", NOTICE_STR1, NOTICE_STR2);
#endif
	return 0;
}

static void print_notice(void)
{
	char notice[DISPLAY_OPTIONS_BANNER_LENGTH];

	if (vidconsole_notice(notice))
		sprintf(notice, "Press %s, %s", NOTICE_STR1, NOTICE_STR2);

	printf("************************** Notice **************************\n");
	printf("%s\r\n", notice);
	printf("************************************************************\n");
}

static void loongson_env_trigger(void)
{
	run_command("loongson_env_trigger init", 0);
	run_command("loongson_env_trigger ls_trigger_u_kernel", 0);
	run_command("loongson_env_trigger ls_trigger_u_rootfs", 0);
	run_command("loongson_env_trigger ls_trigger_u_uboot", 0);
	run_command("loongson_env_trigger ls_trigger_boot", 0);
}

#ifdef CONFIG_BOARD_EARLY_INIT_F
__weak int ls_board_early_init_f(void)
{
	return 0;
}

int board_early_init_f(void)
{
	return ls_board_early_init_f();
}
#endif

#ifdef CONFIG_BOARD_EARLY_INIT_R
__weak int ls_board_early_init_r(void)
{
	return 0;
}

static int update_unknown_board_name(void)
{
#ifndef CONFIG_LOONGSON_COMPAT
	char* temp;
	char* board_name;
	find_bdname();
	if (!bdname)
		return 0;
	board_name = bdinfo_get(BDI_ID_NAME);
	/*
	 * 设置 BDI_ID_NAME 的条件
	 * NULL
	 * 不是 当前板卡的 desc (包括了 unknown)
	 */
	temp = get_board_name_by_desc((char*)bdname);
	if (!temp)
		return 0;
	if (board_name && !strcmp(board_name, temp))
		return 0;
	board_name = temp;
	bdinfo_set(BDI_ID_NAME, board_name);
	bdinfo_save();
	board_name = bdinfo_get(BDI_ID_NAME);
#endif
	return 0;
}

int board_early_init_r(void)
{
	beep_boot();
	bdinfo_init();
	update_unknown_board_name();
	return ls_board_early_init_r();
}
#endif

#ifdef CONFIG_BOARD_LATE_INIT
__weak int ls_board_late_init(void)
{
	return 0;
}

int board_late_init(void)
{
	return ls_board_late_init();
}
#endif

#ifdef CONFIG_LAST_STAGE_INIT
extern void user_env_fetch(void);
extern int recover(void);

#ifdef CONFIG_MTD_RAW_NAND
void adjust_nand_pagesize(void)
{
	struct mtd_info* mtd;
	mtd = get_nand_dev_by_index(0);
	if (mtd) {
		int cur_page_size;
		char cur_page_size_str[10];
		char *env_page_size_str;
		char *default_page_size_str = "2048";

		env_page_size_str = env_get("nand_pagesize");
		if (!env_page_size_str) {
			printf("nand_pagesize is null, use default pagesize(2048)\n");
			env_page_size_str = default_page_size_str;
			env_set("nand_pagesize", env_page_size_str);
		}

		cur_page_size = mtd->writesize;

		memset(cur_page_size_str, 0, 10);
		sprintf(cur_page_size_str, "%d", cur_page_size);
		cur_page_size_str[9] = 0;

		if (strcmp(cur_page_size_str, env_page_size_str)) {
			printf("change env nand_pagesize to be %s(ori is %s)\n", cur_page_size_str, env_page_size_str);
			env_set("nand_pagesize", cur_page_size_str);
		}
	}
}
#endif

#ifndef CONFIG_SPL_BUILD
int last_stage_init(void)
{
	print_notice();
	user_env_fetch();

#if defined(CONFIG_SPL) && !defined(CONFIG_SPL_BUILD) && defined(CONFIG_LOONGSON_COMPAT)
	multi_boards_check_store();
#endif

	usb_init();
#ifdef CONFIG_LOONGSON_RECOVER
	/*
	 * 上电时长按按钮3秒进入recover功能, recover优先顺序usb>mmc>sata
	 * 按键使用 LS_RECOVER_GPIO_BUTTON 定义的gpio
	 */
	recover();
#endif

#ifdef CONFIG_MTD_RAW_NAND
	adjust_nand_pagesize();
#endif

	loongson_env_trigger();

	return 0;
}
EVENT_SPY_SIMPLE(EVT_LAST_STAGE_INIT, last_stage_init);
#endif
#endif

static void find_bdname(void)
{
	struct udevice *dev;
	ofnode parent_node, node;

	bdname = NULL;
	uclass_first_device(UCLASS_SYSINFO, &dev);
	if (dev) {
		parent_node = dev_read_subnode(dev, "smbios");
		if (!ofnode_valid(parent_node))
			return;

		node = ofnode_find_subnode(parent_node, "baseboard");
		if (!ofnode_valid(node))
			return;

		bdname = ofnode_read_string(node, "product");
	}
}

int checkboard(void)
{
	find_bdname();
	if (bdname)
		printf("Board: %s\n", bdname);
	return 0;
}

#if defined(CONFIG_SYS_CONSOLE_IS_IN_ENV) && \
	defined(CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE)
int overwrite_console(void)
{
	return 0;
}
#endif

#ifdef CONFIG_SPL_BOARD_INIT
__weak void spl_board_init(void)
{
	return ;
}
#endif

// Please use the scsi with driver model(CONFIG_DM_SCSI) first!
#ifdef CONFIG_SCSI_AHCI_PLAT
void scsi_init(void)
{
	void __iomem *ahci_base;

	ahci_base = (void __iomem *)LS_SATA_BASE;
	printf("scsi ahci plat %p\n", ahci_base);
	if(!ahci_init(ahci_base))
		scsi_scan(1);
}
#endif

static int cur_abort_min_limit;
void custom_abort_key_probe(char* presskey, int presskey_len, int* abort, int abort_min_limit)
{
	if (!abort || !presskey)
		return;

	cur_abort_min_limit = abort_min_limit;

	if (presskey_len >= 1) {
		if (!memcmp(presskey + presskey_len - 1, "r", 1))
			abort[0] = abort_min_limit + 1;
		else if (!memcmp(presskey + presskey_len - 1, "k", 1))
			abort[0] = abort_min_limit + 2;
		else if (!memcmp(presskey + presskey_len - 1, "a", 1))
			abort[0] = abort_min_limit + 3;
		else if (!memcmp(presskey + presskey_len - 1, "i", 1))
			abort[0] = abort_min_limit + 4;
		else if (!memcmp(presskey + presskey_len - 1, "o", 1))
			abort[0] = abort_min_limit + 5;
	}
}

static int abort_key_bootmenu(void)
{
	const char* s;
	set_stdout(STDOUT_VIDEO, STDOUT_ON);
	s = env_get("menucmd");
	if (s)
		run_command_list(s, -1, 0);
	return 0;
}

static int abort_key_cmdline(void)
{
		set_stdout(STDOUT_VIDEO, STDOUT_OFF);
#if defined(CONFIG_VIDEO)
		puts(ANSI_CLEAR_CONSOLE);
#endif
		set_stdout(STDOUT_VIDEO, STDOUT_ON);
		return 0;
}

static void split_line_printf(void)
{
	printf("######################################\n");
}

static int abort_key_update_rootfs(void)
{
	return run_command("loongson_update usb rootfs nand", 0);
}

static int abort_key_update_kernel(void)
{
	return run_command("loongson_update usb kernel nand", 0);
}

static int abort_key_update_kernel_rootfs(void)
{
	int ret = 0;
	int ret_k;
	int ret_r;
	ret_k = abort_key_update_kernel();
	ret_r = abort_key_update_rootfs();
	split_line_printf();
	if (!ret_k)
		printf("update kernel success!\n");
	if (!ret_r)
		printf("update rootfs success!\n");
	if (ret_k && ret_r) {
		ret = 1;
		printf("update kernel or rootfs failed!\n");
	}
	split_line_printf();
	return ret;
}

static int abort_key_install_system(void)
{
	return run_command("recover_cmd usb", 0);
}

static int abort_key_try_update_uboot_then_install_system(void)
{
	int ret_uboot;
	int ret_recov;
	ret_uboot = run_command("loongson_update usb uboot", 0);
	ret_recov = run_command("recover_cmd usb", 0);
	if (ret_uboot == 0)
		run_command("reset", 0);
	return ret_uboot | ret_recov;
}

typedef int (*abort_key_handle_func)(void);

void abort_key_handle_custom(int abort)
{
	int ret;
	abort_key_handle_func func_set[] = {
		abort_key_update_rootfs,
		abort_key_update_kernel,
		abort_key_update_kernel_rootfs,
		abort_key_install_system,
		abort_key_try_update_uboot_then_install_system,
	};

	if (abort < 0 || abort >= sizeof(func_set) / sizeof(abort_key_handle_func))
		return;
	ret = func_set[abort]();

	if (!ret) {
		printf("boot after 3s!\n");
		mdelay(3000);
		run_command("boot", 0);
		return;
	}

	split_line_printf();
	printf("update failed!\n");
	split_line_printf();

#ifdef CONFIG_LOONGSON_KEYHANDLE_FAIL_CONTINUE_BOOT
	// 失败后可以继续启动
	printf("boot after 3s!\n");
	mdelay(3000);
	run_command("boot", 0);
#endif
}

void custom_abort_key_handle(int abort)
{
	abort_key_handle_func generic_func_set[] = {abort_key_bootmenu, abort_key_cmdline};

	if (abort <= 0)
		return;

	--abort;
	if (abort < cur_abort_min_limit)  //bootdelaykey or bootstopkey
		generic_func_set[abort]();
	else {
		abort -= cur_abort_min_limit; // start form limit (limit -> 0)
		abort_key_handle_custom(abort);
	}
}

void autoboot_failed_custom_handle(const char* s)
{
	set_stdout(STDOUT_VIDEO, STDOUT_ON);
	printf("Bootcmd=\"%s\"\n", s ? s : "<UNDEFINED>");
	printf("Boot Kernel failed. Kernel not found or bad.\n");
}
