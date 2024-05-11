#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <search.h>
#include <asm/global_data.h>
#include <watchdog.h>
#include <errno.h>
#include <hang.h>
#include <env_flags.h>
#include <linux/ctype.h>
#include "bdinfo.h"
#include "bdi_internal.h"

DECLARE_GLOBAL_DATA_PTR;


static int htab_bdi_validate(const struct env_entry *item, const char *newval,
			 enum env_op op, int flag);

#ifdef DEBUG
void bdinfo_htab_dump(void);
#endif

char *bdi_id_name[BDI_ID_MAX] = {
	[BDI_ID_NAME] = "bdname",
	[BDI_ID_SN] = "sn",
	[BDI_ID_MAC0] = "mac0",
	[BDI_ID_MAC1] = "mac1",
	[BDI_ID_SYSPART] = "syspart",
#ifdef LS_DOUBLE_SYSTEM
	[BDI_ID_SYSPART_LAST] = "syspart_last",
#endif
	[BDI_ID_BOARD_PRODUCT_ID] = "board_product_id",
};

static char default_bdinfo[] = {
	"bdname=unknown\0"
	"sn=unknown\0"
	"mac0=00:00:00:00:00:00\0"
	"mac1=00:00:00:00:00:00\0"
	"syspart=1\0"
#ifdef LS_DOUBLE_SYSTEM
	"syspart_last=1\0"
#endif
	"board_product_id=0\0"
	"\0"
};

static struct ls_bdinfo bdinfo = {
	.htab = {
		.table = NULL,
		.change_ok = htab_bdi_validate,
	},
	.status = BDI_INVALID,
	.loc = BDI_LOC_UNKNOWN,
};

int bdi_validate(enum bdi_id id, char *data)
{
	char *p = NULL;
	int i;

	switch (id)
	{
	case BDI_ID_NAME:
	case BDI_ID_SN:
		p = data;
 		while (p && *p != '\0')
		{
			if (!isprint(*p++))
				return -EINVAL;
		}
		break;
	case BDI_ID_MAC0:
	case BDI_ID_MAC1:
		if (strlen(data) != 17)
			return -EINVAL;

		for (i = 0; i < 6; i++) {
			if (!isdigit(data[i*3])
				  || !isdigit(data[i*3+1]))
				return -EINVAL;

			if (i > 0 && (data[i*3-1] != ':'))
				return -EINVAL;
		}
		break;
	case BDI_ID_SYSPART:
#ifdef LS_DOUBLE_SYSTEM
	case BDI_ID_SYSPART_LAST:
#endif
	case BDI_ID_BOARD_PRODUCT_ID:
		p = data;
 		while (p && *p != '\0')
		{
			if (!isdigit(*p++))
				return -EINVAL;
		}
		break;
	default:
		debug("invalid id: %d\n", id);
		return -EINVAL;
	}

	return 0;
}

static int htab_bdi_validate(const struct env_entry *item, const char *newval,
			 enum env_op op, int flag)
{
	int i, id = BDI_ID_MAX;

	for (i = 0; i < BDI_ID_MAX; i++) {
		if (!strcmp(item->key, bdi_id_name[i])) {
			id = i;
			break;
		}
	}

	return bdi_validate(id, item->data);
}


int bdinfo_init(void)
{
	if (bdinfo.status == BDI_VALID)
		return 0;

	// Try to load the board info from eeprom.
	load_bdinfo_eeprom(&bdinfo);

	// If can not get the board info from eeprom,
	// we load it from spi-flash.
	if (bdinfo.status == BDI_INVALID) {
		load_bdinfo_sf(&bdinfo);
	}

	// No found the board info, use the default.
	if (bdinfo.status == BDI_INVALID) {
		puts("use the default bdinfo\n");
		bdinfo.loc = BDI_LOC_UNKNOWN;
		if (!himport_r(&bdinfo.htab, default_bdinfo,
				sizeof(default_bdinfo), BDI_SEPARATE, H_EXTERNAL, 0, 0, NULL)) {
			debug("Creat hash table for bdinfo failed\n");
			return -EINVAL;
		}
		bdinfo.status = BDI_DEFAULT;
	}

	printf("bdinfo is %s\n", bdinfo.loc == BDI_LOC_EEPROM ? "in eeprom" :
				 bdinfo.loc == BDI_LOC_SPI_FLASH ? "in spi-flash" :
				 "default");

#ifdef DEBUG_BDINFO_DUMP
	bdinfo_htab_dump();
#endif
	return 0;
}

int bdinfo_save_in_nv(void)
{
	return (bdinfo.status == BDI_DEFAULT) ? 1 : 0;
}

char* bdinfo_get(enum bdi_id id)
{
	struct env_entry e, *ep;

	if (bdinfo.status == BDI_INVALID || id >= BDI_ID_MAX)
		return NULL;

	schedule();

	e.key	= bdi_id_name[id];
	e.data	= NULL;
	hsearch_r(e, ENV_FIND, &ep, &bdinfo.htab, 0);

	return ep ? ep->data : NULL;
}

int bdinfo_set(enum bdi_id id, const char *val)
{
	struct env_entry e, *ep;
	const char *name;
	int ret;

	if (bdinfo.status == BDI_INVALID || id >= BDI_ID_MAX)
		return -EINVAL;

	name = bdi_id_name[id];
	if (val == NULL || val[0] == '\0') {	// delete
		ret = hdelete_r(name, &bdinfo.htab, H_PROGRAMMATIC);
		/* If the variable didn't exist, don't report an error */
		if (ret == -ENOENT)
			ret = 0;
		return ret;
	}

	schedule();

	e.key = name;
	e.data = (char*)val;
	hsearch_r(e, ENV_ENTER, &ep, &bdinfo.htab, H_PROGRAMMATIC);
	if (!ep) {
		debug("bdinfo set %s to %s failed\n", name, val);
		return -EINVAL;
	}

	return 0;
}

#ifndef CONFIG_SPL_BUILD
int bdinfo_save(void)
{
	char *buf;
	ssize_t	len;
	int ret = -EINVAL;

	buf = malloc(BOARD_INFO_MAX_SIZE);
	if (!buf)
		return -ENOMEM;

	len = hexport_r(&bdinfo.htab, BDI_SEPARATE, 0, &buf, len, 0, NULL);
	if (len < 0) {
		printf("can not export bdinfo: errno = %d\n", errno);
		ret = errno;
		goto err_out;
	}

	if (bdinfo.loc == BDI_LOC_EEPROM) {
		ret = save_bdinfo_eeprom(&bdinfo, buf, len);
		if (ret) {
			printf("save bdinfo to eeprom failed\n");
			goto err_out;
		}
	} else if (bdinfo.loc == BDI_LOC_SPI_FLASH) {
		ret = save_bdinfo_sf(&bdinfo, buf, len);
		if (ret) {
			printf("save bdinfo to spi-flash failed\n");
			goto err_out;
		}
	} else {
		// try eeprom first
		ret = save_bdinfo_eeprom(&bdinfo, buf, len);
		// then spi-flash
		if (ret)
			ret = save_bdinfo_sf(&bdinfo, buf, len);
	}

err_out:
	free(buf);
	return ret;
}
#endif

#ifdef DEBUG_BDINFO_DUMP
static int bdi_print_entry(struct env_entry *entry)
{
	enum env_flags_vartype type;
	int access;

	type = (enum env_flags_vartype)
		(entry->flags & ENV_FLAGS_VARTYPE_BIN_MASK);
	access = (entry->flags & ENV_FLAGS_VARACCESS_BIN_MASK);
	printf("\t%-20s %-20s 0x%x 0x%x\n", entry->key, entry->data,
		type, access);

	return 0;
}

void bdinfo_htab_dump(void)
{
	puts(">>>>>>>> bdinfo htab <<<<<<<<\n");
	hwalk_r(&bdinfo.htab, bdi_print_entry);
	puts("\n");
}
#endif
