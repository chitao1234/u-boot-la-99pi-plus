#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <u-boot/crc.h>
#include <i2c_eeprom.h>
#include <search.h>
#include <watchdog.h>
#include <linux/ctype.h>
#include "bdi_internal.h"

#define EEPROM_BDINFO_SIZE	SZ_256

#define EEPROM_BDINFO_CRC_OFFSET 252

static int bdi_id_size[BDI_ID_MAX] = {
	[BDI_ID_NAME] = 32,
	[BDI_ID_SN] = 32,
	[BDI_ID_MAC0] = 16,
	[BDI_ID_MAC1] = 16,
	[BDI_ID_SYSPART] = 1,
#ifdef LS_DOUBLE_SYSTEM
	[BDI_ID_SYSPART_LAST] = 1,
#endif
	[BDI_ID_BOARD_PRODUCT_ID] = 1,
};

static int eeprom_content_len;
static inline size_t get_eeprom_content_len(void)
{
	if (unlikely(!eeprom_content_len)) {
		int i;
		size_t len = 0;

		for (i = 0; i < BDI_ID_MAX; ++i)
			len += bdi_id_size[i];
		return len;
	}
	return eeprom_content_len;
}

static size_t get_eeprom_offset(enum bdi_id id)
{
	size_t offs = 0;
	int i;

	for (i = 0; i < id; ++i)
		offs += bdi_id_size[i];

	return offs;
}

static int decode_bdi_data(uint8_t *savebuf, int size, enum bdi_id id, uint8_t *data)
{
	int len, i;
	char *value;

	if (!savebuf || !data || !strlen(data))
		return 0;

	// check if all printable char
	for (i = 0; i < strlen(data); ++i) {
		if (data[i] == 0xff)
			return 0;
	}

	switch (id)
	{
	case BDI_ID_NAME:
	case BDI_ID_SN:
		len = snprintf(savebuf, size, "%s=%s", bdi_id_name[id], data);
		break;
	case BDI_ID_MAC0:
	case BDI_ID_MAC1:
		len = snprintf(savebuf, size, "%s=%x:%x:%x:%x:%x:%x", bdi_id_name[id],
				data[0], data[1], data[2], data[3], data[4], data[5]);
		break;
	case BDI_ID_SYSPART:
#ifdef LS_DOUBLE_SYSTEM
	case BDI_ID_SYSPART_LAST:
#endif
	case BDI_ID_BOARD_PRODUCT_ID:
		len = snprintf(savebuf, size, "%s=%d", bdi_id_name[id], data[0]);
		break;
	default:
		return 0;
	}

	value = strchr(savebuf, '=');
	if(value && bdi_validate(id, ++value))
		return 0;

	// replace '\0' to BDI_SEPARATE.
	savebuf[len++] = BDI_SEPARATE;

	return len;
}

int load_bdinfo_eeprom(struct ls_bdinfo *bdi)
{
	uint8_t buf[33];
	uint8_t *bdidata;
	struct udevice *dev;
	int idx, offs, len = 0, ret = 0;
	unsigned char eeprom_content_buf[EEPROM_BDINFO_SIZE];
	uint32_t crc, crc_calu;

	bdidata = malloc(BOARD_INFO_MAX_SIZE);
	if (!bdidata)
		return -ENOMEM;

	memset(bdidata, 0, BOARD_INFO_MAX_SIZE);
	memset(eeprom_content_buf, 0, EEPROM_BDINFO_SIZE);

	schedule();

	for (uclass_first_device(UCLASS_I2C_EEPROM, &dev);
			dev; uclass_next_device(&dev)) {

		if (ofnode_read_bool(dev->node_, "loongson,bdinfo")) {
			bdi->dev = dev;
			bdi->loc = BDI_LOC_EEPROM;

			i2c_eeprom_read(dev, 0, eeprom_content_buf, get_eeprom_content_len());
			i2c_eeprom_read(dev, EEPROM_BDINFO_CRC_OFFSET, (unsigned char*)(&crc), sizeof(crc));
			crc_calu = crc32(0, eeprom_content_buf, get_eeprom_content_len());
			if (crc != crc_calu) {
				debug("bdinfo: crc error: crc(eeprom): %.4x and crc_calu: %.4x\n", crc, crc_calu);
				return -EINVAL;
			}

			for (idx = 0; idx < BDI_ID_MAX; ++idx) {
				memset(buf, 0, sizeof(buf));
				offs = get_eeprom_offset(idx);
				//ret = i2c_eeprom_read(dev, offs, (uint8_t*)buf, bdi_id_size[idx]);
				//because eeprom_content_buf read all value, dont need i2c read single property
				memcpy(buf, eeprom_content_buf + offs, bdi_id_size[idx]);
				if (ret) {
					printf("read %s failed: %d\n", bdi_id_name[idx], ret);
					goto err_out;
				}

				len += decode_bdi_data(bdidata + len,
						BOARD_INFO_MAX_SIZE - len, idx, buf);
			}

			break;
		}
	}

	schedule();

	if (len > 0 && himport_r(&bdi->htab, bdidata,
			len + 1, BDI_SEPARATE, H_EXTERNAL, 0, 0, NULL)) {
		bdi->status = BDI_VALID;
	} else {
		debug("import bdinfo failed\n");
		ret = -EINVAL;
	}

err_out:
	free(bdidata);
	return ret;
}

#ifndef CONFIG_SPL_BUILD
static int code_bdi_data(uint8_t *savebuf, int size, enum bdi_id id, uint8_t *data)
{
	int len = 0, idx, i;
	char *p;

	if (!savebuf || !data || !strlen(data))
		return 0;

	switch (id) {
	case BDI_ID_NAME:
	case BDI_ID_SN:
		len = strlen(data);
		memcpy(savebuf, data, len);
		break;
	case BDI_ID_MAC0:
	case BDI_ID_MAC1:
		idx = 0;
		p = data;
		while (*p++) {
			if (*p == ':')
				*p = '\0';
		}
		for (i = 0; i < 6; ++i) {
			savebuf[idx++] = simple_strtoul(&data[i*3], NULL, 16);
		}
		len = 6;
		break;
	case BDI_ID_SYSPART:
#ifdef LS_DOUBLE_SYSTEM
	case BDI_ID_SYSPART_LAST:
#endif
	case BDI_ID_BOARD_PRODUCT_ID:
		*savebuf = simple_strtoul(data, NULL, 0);
		len = 1;
		break;
	default:
		return 0;
	}

	return len;
}

int save_bdinfo_eeprom(struct ls_bdinfo *bdi, const char *data, size_t len)
{
	struct udevice *dev;
	size_t offset;
	uint8_t *buf;
	char *name, *value, *datatmp, *dp, *sp;
	int idx, buf_len, size, ret = 0;
	unsigned char eeprom_content_buf[EEPROM_BDINFO_SIZE];
	uint32_t crc;

	if (!bdi || !data || !bdi->dev)
		return -EINVAL;

	dev = bdi->dev;

	datatmp = malloc(len);
	if (!datatmp)
		return -ENOMEM;

	memcpy(datatmp, data, len);
	dp = datatmp;

	for (idx = 0, buf_len = 0; idx < BDI_ID_MAX; ++idx) {
		if (bdi_id_size[idx] > buf_len)
			buf_len = bdi_id_size[idx];
	}

	buf = malloc(buf_len);
	if (!buf) {
		free(datatmp);
		return -ENOMEM;
	}

	memset(eeprom_content_buf, 0, EEPROM_BDINFO_SIZE);
	do {
		/* parse name */
		for (name = dp; *dp != '=' && *dp && *dp != BDI_SEPARATE; ++dp)
			;

		/* deal with "name" and "name=" entries */
		if (*dp == '\0' || *(dp + 1) == '\0' ||
			*dp == BDI_SEPARATE || *(dp + 1) == BDI_SEPARATE) {
			if (*dp == '=')
				*dp++ = '\0';
			*dp++ = '\0';	/* terminate name */
			continue;
		}
		*dp++ = '\0';	/* terminate name */

		/* parse value; deal with escapes */
		for (value = sp = dp; *dp && (*dp != BDI_SEPARATE); ++dp) {
			if ((*dp == '\\') && *(dp + 1))
				++dp;
			*sp++ = *dp;
		}
		*sp++ = '\0';	/* terminate value */
		++dp;

		if (*name == 0)
			continue;

		for (idx = 0; idx < BDI_ID_MAX; ++idx) {
			if (!strcmp(name, bdi_id_name[idx])) {
				memset(buf, 0, buf_len);
				offset = get_eeprom_offset(idx);
				//clear
				ret = i2c_eeprom_write(dev, offset, buf, bdi_id_size[idx]);
				if (ret) {
					printf("clear %s failed: %d\n", bdi_id_name[idx], ret);
					goto err_out;
				}

				// write
				size = code_bdi_data(buf, bdi_id_size[idx], idx, value);
				if (size > bdi_id_size[idx]) {
					log_warning("size of %s`s value is exceeded\n", bdi_id_name[idx]);
					size = bdi_id_size[idx];
				}

				ret = i2c_eeprom_write(dev, offset, buf, size);
				if (ret) {
					printf("write %s failed: %d\n", bdi_id_name[idx], ret);
					goto err_out;
				}
			}
		}
	} while ((dp < datatmp + len) && *dp);

	//发现上面的 写入内容里面, 有可能会不写入某个信息, 因为 那个信息不存在 从而eeprom那边就是原来的值，所以重新读取eeprom的值最保险。
	i2c_eeprom_read(dev, 0, eeprom_content_buf, get_eeprom_content_len());
	crc = crc32(0, eeprom_content_buf, get_eeprom_content_len());
	i2c_eeprom_write(dev, EEPROM_BDINFO_CRC_OFFSET, (unsigned char *)(&crc), sizeof(crc));

err_out:
	free(buf);
	free(datatmp);
	return ret;
}
#endif
