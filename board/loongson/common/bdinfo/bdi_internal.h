#ifndef __LS_BDINFO_INTERNAL_H__
#define __LS_BDINFO_INTERNAL_H__

#include <linux/sizes.h>
#include <linux/mtd/mtd.h>
#include "bdinfo.h"

#define BOARD_INFO_MAX_SIZE		SZ_4K
#define BDI_SEPARATE	'\0'

enum bdi_status {
	BDI_INVALID = 0,
	BDI_VALID,
	BDI_DEFAULT,
};

enum bdi_location {
	BDI_LOC_UNKNOWN,
	BDI_LOC_EEPROM,
	BDI_LOC_SPI_FLASH,
};

struct ls_bdinfo {
	struct hsearch_data 	htab;
	enum bdi_status		status;
	enum bdi_location	loc;
	struct mtd_info 	*mtd;
	struct udevice 		*dev;
};

extern char *bdi_id_name[BDI_ID_MAX];

int bdi_validate(enum bdi_id id, char *data);

int load_bdinfo_eeprom(struct ls_bdinfo *bdi);
int load_bdinfo_sf(struct ls_bdinfo *bdi);

#ifndef CONFIG_SPL_BUILD
int save_bdinfo_eeprom(struct ls_bdinfo *bdi,
		const char *data, size_t len);
int save_bdinfo_sf(struct ls_bdinfo *bdi,
		const char *data, size_t len);
#endif

#endif /* __LS_BDINFO_INTERNAL_H__ */
