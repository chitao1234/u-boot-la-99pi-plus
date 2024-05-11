// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <malloc.h>

#include "loongson_boot.h"

typedef int (*loongson_boot_func)(void);

static int loongson_boot_ssd(void)
{
#ifdef BOOT_SATA_DEFAULT
	printf("boot system from ssd .....\r\n");
	return run_command(BOOT_SATA_DEFAULT, 0);
#else
	return -1;
#endif
}

static int loongson_boot_nand(void)
{
#ifdef BOOT_NAND_DEFAULT
	printf("boot system from nand .....\r\n");
	return run_command(BOOT_NAND_DEFAULT, 0);
#else
	return -1;
#endif
}

static int loongson_boot_mmc(void)
{
#ifdef BOOT_MMC_DEFAULT
	printf("boot system from mmc .....\r\n");
	return run_command(BOOT_MMC_DEFAULT, 0);
#else
	return -1;
#endif
}

static char* boot_param_list[LOONGSON_BOOT_TYPE_SIZE] = {"ssd", "nand", "mmc"};
static loongson_boot_func boot_func_list[LOONGSON_BOOT_TYPE_SIZE] = {loongson_boot_ssd, loongson_boot_nand, loongson_boot_mmc};

static int do_loongson_boot(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	int index;

	if(argc != 2)
		return ret;

	if(!argv[1])
		return ret;

	index = 0;
	for (index = 0; index < LOONGSON_BOOT_TYPE_SIZE; ++index) {
		if(!strcmp(argv[1], boot_param_list[index])) {
			ret = (boot_func_list[index])();
			break;
		}
	}

	return ret;
}

#define LOONGSON_BOOT_HELP_HEAD "set system boot from where."

#define LOONGSON_BOOT_USAGE_HEAD "<option>\n" \

#ifdef BOOT_SATA_DEFAULT
#define LOONGSON_BOOT_SSD_USAGE "option: ssd: boot from ssd\n"
#else
#define LOONGSON_BOOT_SSD_USAGE "not support boot from ssd\n"
#endif

#ifdef BOOT_NAND_DEFAULT
#define LOONGSON_BOOT_NAND_USAGE "option: nand: boot from nand\n"
#else
#define LOONGSON_BOOT_NAND_USAGE "not support boot from nand\n"
#endif

#define LOONGSON_BOOT_HELP LOONGSON_BOOT_HELP_HEAD

#define LOONGSON_BOOT_USAGE LOONGSON_BOOT_USAGE_HEAD \
							LOONGSON_BOOT_SSD_USAGE \
							LOONGSON_BOOT_NAND_USAGE

U_BOOT_CMD(
	loongson_boot,    2,    1,     do_loongson_boot,
	LOONGSON_BOOT_HELP,
	LOONGSON_BOOT_USAGE
);
