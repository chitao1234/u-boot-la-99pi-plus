#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <mtd.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <u-boot/crc.h>
#include <search.h>
#include <watchdog.h>
#include <asm/addrspace.h>
#include <mapmem.h>
#include "bdi_internal.h"

#define SF_BOARD_INFO_PART_NAME		"bdinfo"

struct sf_bdi_data {
	uint32_t	crc;
	unsigned char	data[0];
};

extern int mtd_get_partition(int type, const char *partname,
				struct mtd_partition *part);

static int get_bdinfo_mtd_partition(struct mtd_partition* part)
{
	if (!part)
		return -1;

	return mtd_get_partition(MTD_DEV_TYPE_NOR, SF_BOARD_INFO_PART_NAME, part);
}

static int index_of_bdinfo_end(unsigned char* buf, int offset, int check_len)
{
	int i;
	int end_index = offset + check_len - 1;

	if (!buf)
		return -3;

	if (!offset) {
		// first 16 byte is 0xff mean spi flash erase mean invaild bdinfo
		for (i = 0; i < 16; ++i) {
			if (buf[i] != 0xff)
				break;
		}
		if (i == 16)
			return -2;

		offset += 4; // jump crc
	}

	for (i = offset; i < end_index; ++i) {
		if (buf[i] == 0 && buf[i + 1] == 0)
			return i;
	}
	return -1;
}

#ifdef CONFIG_SPL_BUILD

static int read_bdinfo(struct ls_bdinfo *bdi, u8 *buf, size_t bufsize)
{
	u8 *addr;
	size_t len;
	size_t i, read_time = 0;
	ssize_t offs, offset_buf;
	struct mtd_partition part;
	int once_len = 0x100;
	int ret = 0;

	if (get_bdinfo_mtd_partition(&part)) {
		printf("get bdinfo mtd partition in env mtdparts failed!\n");
		return ret;
	}

	bdi->mtd = NULL;
	if (get_bdinfo_mtd_partition(&part))
		return -ENOENT;
	offs = part.offset;
	addr = map_sysmem(BOOT_SPACE_BASE + offs, 
				BOARD_INFO_MAX_SIZE);

	len = (bufsize < part.size) ? bufsize : part.size;
	read_time = len >> 8; // / 0x100 read 0x100 size once

	offset_buf = 0;
	for (i = 0; i < read_time; ++i) {
		memcpy(buf + offset_buf, addr + offset_buf, once_len);

		ret = index_of_bdinfo_end(buf, offset_buf, once_len);

		if (ret == -2 || ret == -3) // all 0xff invaild bdinfo or buf invaild
			return -2;
		if (ret >= 0)
			return (ret - 2); // get 00 index about first 0, func need return bdinfo len so index + 2 and delete crc 4

		offset_buf += once_len; // read next 0x100 size
	}

	// not found bdinfo end target 00 so mean bdinfo invaild
	return -1;
}

#else
static struct mtd_info* find_bdinfo_partition(void)
{
	struct mtd_info *mtd, *found = NULL;

	mtd_probe_devices();

	mtd_for_each_device(mtd) {
		if (mtd->type == MTD_NORFLASH
			&& !strcmp(mtd->name, SF_BOARD_INFO_PART_NAME)) {
			found = mtd;
			break;
		}
	}

	return found;
}

#include <command.h>
#include <spi_flash.h>

static int setup_flash_device(struct spi_flash **env_flash)
{
#if CONFIG_IS_ENABLED(DM_SPI_FLASH)
	struct udevice *new;
	int	ret;

	/* speed and mode will be read from DT */
    //ret = spi_flash_probe_bus_cs(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS,
	//				CONFIG_ENV_SPI_MAX_HZ, CONFIG_ENV_SPI_MODE,
	//				&new);
	ret = spi_flash_probe_bus_cs(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS, &new);
	if (ret) {
		env_set_default("spi_flash_probe_bus_cs() failed", 0);
		return ret;
	}

	*env_flash = dev_get_uclass_priv(new);
#else
   // *env_flash = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS,
   // 				CONFIG_ENV_SPI_MAX_HZ, CONFIG_ENV_SPI_MODE);
   // ret = spi_flash_probe_bus_cs(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS, &new);
	if (!*env_flash) {
		env_set_default("spi_flash_probe() failed", 0);
		return -EIO;
	}
#endif
	return 0;
}

static int read_bdinfo(struct ls_bdinfo *bdi, u8 *buf, size_t bufsize)
{
	size_t len;
	size_t i, offset;
	int ret = 0;
	struct spi_flash *env_flash;
	struct mtd_partition part;
	int read_time;

	int once_len = 0x100;

	ret = get_bdinfo_mtd_partition(&part);

	if (ret) {
		printf("get bdinfo mtd partition in env mtdparts failed!\n");
		return ret;
	}

	ret += setup_flash_device(&env_flash);

	len = (bufsize < part.size) ? bufsize : part.size;
	read_time = len >> 8; // / 0x100 read 0x100 size once

	offset = 0;
	for (i = 0; i < read_time; ++i) {
		ret = spi_flash_read(env_flash, part.offset + offset, once_len, buf + offset);
		if (ret) {
			printf("Read board info from spi flash failed\n");
			return ret;
		}

		ret = index_of_bdinfo_end(buf, offset, once_len);

		if (ret == -2 || ret == -3) // all 0xff invaild bdinfo or buf invaild
			return -2;
		if (ret >= 0)
			return (ret - 2); // get 00 index about first 0, func need return bdinfo len so index + 2 and delete crc 4

		offset += once_len; // read next 0x100 size
	}

	// not found bdinfo end target 00 so mean bdinfo invaild
	return -1;
}

int save_bdinfo_sf(struct ls_bdinfo *bdi,
		const char *data, size_t len)
{
	struct mtd_info *mtd;
	struct erase_info erase;
	uint32_t crc;
	size_t retlen;
	int ret;

	if (!bdi || !data)
		return -EINVAL;
	
	if (bdi->mtd)
		mtd = bdi->mtd;
	else
		mtd = find_bdinfo_partition();
	
	if (!mtd)
		return -ENODEV;

	if (len > mtd->size - sizeof(crc))
		len = mtd->size - sizeof(crc);

	memset(&erase, 0, sizeof(erase));
	erase.mtd = mtd;
	erase.addr = 0x0;
	erase.len = mtd->size;

	debug("Save bdinfo to spi-flash...");
	debug("Erasing...");
	ret = mtd_erase(mtd, &erase);
	if (ret) {
		printf("Erase bdinfo partition failed\n");
		return ret;
	}

	debug("Writing...");
	crc = crc32(0, data, len);
	ret = mtd_write(mtd, 0, sizeof(crc), &retlen, (u_char*)&crc);
	if (ret || (retlen != sizeof(crc))) {
		printf("Write bdinfo crc failed\n");
		return ret ? ret : -EIO;
	}

	ret = mtd_write(mtd, sizeof(crc), len, &retlen, (u_char*)data);
	if (ret || (retlen != len)) {
		printf("Write bdinfo failed\n");
		return ret ? ret : -EIO;
	}

	debug("OK\n");
	return 0;
}
#endif

int load_bdinfo_sf(struct ls_bdinfo *bdi)
{
	struct sf_bdi_data *bdidata;
	u8 *buf;
	size_t len;
	int ret = 0;

	len = BOARD_INFO_MAX_SIZE;
	buf = malloc(len);
	if (!buf)
		return -ENOMEM;

	schedule();

	ret = read_bdinfo(bdi, buf, len);
	if (ret < 0) {
		debug("read bdinfo failed\n");
		return ret;
	}

	len = ret;
	bdi->loc = BDI_LOC_SPI_FLASH;
	bdidata = (struct sf_bdi_data *)buf;

	if (crc32(0, bdidata->data, len) != bdidata->crc) {
		debug("Board info is invalid\n");
		ret = -EINVAL;
		goto err_out;
	}

	schedule();

	if (himport_r(&bdi->htab, bdidata->data, len, BDI_SEPARATE, H_EXTERNAL, 0, 0, NULL)) {
		bdi->status = BDI_VALID;
	} else {
		debug("Import bdinfo failed\n");
		ret = -EINVAL;
	}

err_out:
	free(buf);
	return ret;
}

