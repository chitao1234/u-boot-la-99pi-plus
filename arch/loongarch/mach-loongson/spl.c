#include <common.h>
#include <init.h>
#include <spl.h>
#include <asm/addrspace.h>
#include <asm/sections.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <asm/unaligned.h>
#include <spi.h>
#include <spi_flash.h>
#include <mach/loongson.h>
#include <image.h>
#include <gzip.h>
#include <linux/lzo.h>
#include <linux/mtd/partitions.h>

#if CONFIG_SYS_LOAD_ADDR >= LOCK_CACHE_BASE && CONFIG_SYS_LOAD_ADDR < (LOCK_CACHE_BASE + LOCK_CACHE_SIZE)
#error should adjust CONFIG_SYS_LOAD_ADDR because conflict with LOCK_CACHE_BASE and SIZE (asm/addrspace.h)
#endif

DECLARE_GLOBAL_DATA_PTR;

extern u32 get_fdt_totalsize(const void *fdt);
extern int mtd_get_partition(int type, const char *partname,
				struct mtd_partition *part);

void board_boot_order(u32 *spl_boot_list)
{
	int n = 0;

	spl_boot_list[n++] = BOOT_DEVICE_BOARD;

#ifdef CONFIG_SPL_SPI
	spl_boot_list[n++] = BOOT_DEVICE_SPI;
#endif
#ifdef CONFIG_SPL_MMC
	spl_boot_list[n++] = BOOT_DEVICE_MMC1;
#endif
#ifdef CONFIG_SPL_NAND_SUPPORT
	spl_boot_list[n++] = BOOT_DEVICE_NAND;
#endif
}

unsigned int spl_get_uboot_offs(void)
{
	ulong spl_size, fdt_size;
	unsigned int offs;

#if CONFIG_IS_ENABLED(MULTI_DTB_FIT)
	fdt_size = get_fdt_totalsize(gd_multi_dtb_fit());
#else
	fdt_size = get_fdt_totalsize(gd->fdt_blob);
#endif
	spl_size = (ulong)&_image_binary_end - (ulong)__text_start;

	offs = spl_size + fdt_size;

	debug("uboot offs: 0x%x\n", offs);
	return offs;
}

#ifdef CONFIG_SPL_SPI_LOAD
unsigned int spl_spi_get_uboot_offs(struct spi_flash *flash)
{
	return spl_get_uboot_offs();
}
#endif

#ifdef CONFIG_SPL_DISPLAY_PRINT
void spl_display_print(void)
{
    printf("\n");                                                                                                 
    printf("|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    printf("||  |||||||||       |||||       ||||   |||||  |||||      |||||       |||||       ||||   |||||  ||\n");
    printf("||  ||||||||   ||||  |||   ||||  |||    ||||  ||||  ||||  |||   ||||  |||   ||||  |||    ||||  ||\n");
    printf("||  ||||||||  |||||| |||  |||||| |||  |  |||  |||  ||||||||||||   |||||||  |||||| |||  |  |||  ||\n");
    printf("||  ||||||||  |||||| |||  |||||| |||  ||  ||  |||  |||    |||||||    ||||  |||||| |||  ||  ||  ||\n");
    printf("||  ||||||||  |||||| |||  |||||| |||  |||  |  |||  |||||  ||||||||||  |||  |||||| |||  |||  |  ||\n");
    printf("||  ||||||||   ||||  |||   ||||  |||  ||||    |||   ||||  |||   |||  ||||   ||||  |||  ||||    ||\n");
    printf("||       ||||       |||||       ||||  |||||   ||||       |||||      ||||||       ||||  |||||   ||\n");
    printf("|||||||||||||||||||||||||||||||||||||||[2020 LOONGSON]|||||||||||||||||||||||||||||||||||||||||||\n");
}
#endif

void spl_perform_fixups(struct spl_image_info *spl_image)
{
	spl_image->entry_point = 
		(uintptr_t)map_sysmem(spl_image->entry_point, 0);
}

static size_t get_uboot_part_size(void)
{
	struct mtd_partition part;
	int ret;

	ret = mtd_get_partition(MTD_DEV_TYPE_NOR, "uboot", &part);
	if (ret)
		return 0;

	return part.size;
}

static ulong spl_board_load_read(struct spl_load_info *load, ulong sector,
			       ulong count, void *buf)
{
	debug("%s: sector %lx, count %lx, buf %lx\n",
	      __func__, sector, count, (ulong)buf);

	memcpy(buf, sector, count);
	return count;
}

#define HEADER_HAS_FILTER	0x00000800L

static inline size_t lzo_buffer_size_calu(const unsigned char *src)
{
	u16 version;
	unsigned int i;
	size_t len;

	if (!lzop_is_valid_header(src))
		return 0;

	/* skip header */
	src += 9;

	/* get version (2bytes), skip library version (2),
	 * 'need to be extracted' version (2) and
	 * method (1) */
	version = get_unaligned_be16(src);
	src += 7;
	if (version >= 0x0940)
		src++;
	if (get_unaligned_be32(src) & HEADER_HAS_FILTER)
		src += 4; /* filter info */

	/* skip flags, mode and mtime_low */
	src += 12;
	if (version >= 0x0940)
		src += 4;	/* skip mtime_high */

	i = *src++;
	/* don't care about the file name, and skip checksum */
	src += i + 4;

	// src now is offset to uncompressed size

	len = 0;
	do {
		// if uncompressed size is 0 mean file unzip finish
		i = get_unaligned_be32(src);
		if (likely(i)) {
			i = get_unaligned_be32(src + 4); // get compressed size
			len += i;
			// jump uncompressed_size compressed_size check-sum-uncompressed
			src += i + 12;
		}
	} while(i);

	/*
	 * because we need this len is buffer len not real lzop file data content len
	 * so need add lzop header len(or more), to ensure lzop file all content in buffer
	 * next opertion will add a num which filed [0x200 - 0x2ff] (because let result align 0xff)
	 * because name in header len may 0-0xff so 0x100 + 0x100 = 0x200
	 */
	len += 0x200;
	if (likely(len)) {
		if (len & 0xff)
			len += 0x100;
		len >>= 0x8;
		len <<= 0x8;
	}

	return len;
}

static int spl_board_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	u8 *imgaddr, *buf;
	struct legacy_img_hdr *header;
	size_t size, uncompress_size = SZ_4M;
	ulong payload_offs;
	int ret;

	payload_offs = spl_get_uboot_offs();
	imgaddr = map_sysmem(BOOT_SPACE_BASE + payload_offs, 0);

	size = sizeof(*header);
	header = spl_get_load_buffer(-size, size);
	buf = (u8 *)header;

	/* Load u-boot, mkimage header is 64 bytes. */
	memcpy(buf, imgaddr, size);

	// If the payload is compressed, Uncompress it first.
	if (IS_ENABLED(CONFIG_SPL_GZIP) &&
		  buf[0] == 0x1f && buf[1] == 0x8b) {	// check gzip magic
		size = get_uboot_part_size();
		if (size > 0)
			size = size - payload_offs;
		else
			size = BOOT_SPACE_SIZE - payload_offs;
		header = spl_get_load_buffer(-uncompress_size, uncompress_size);

		buf = (u8*)CONFIG_SYS_LOAD_ADDR;
		memcpy(buf, imgaddr, size);

		ret = gunzip(header, uncompress_size, buf, &size);
		if (ret) {
			puts("gunzip Uncompress failed!\n");
			return ret;
		}

		debug("gzip uncompressed size: %lu\n", size);
		imgaddr = (u8 *)header;
	} else if (IS_ENABLED(CONFIG_SPL_LZO) &&
			  lzop_is_valid_header(buf)) {
		size = lzo_buffer_size_calu(imgaddr);

		// if cannt read real size just read all uboot storage dev
		if (unlikely(!size))
			size = BOOT_SPACE_SIZE - payload_offs;

		header = spl_get_load_buffer(-uncompress_size, uncompress_size);

		buf = (u8*)CONFIG_SYS_LOAD_ADDR;
		memcpy(buf, imgaddr, size);

		ret = lzop_decompress(buf, size, (unsigned char*)header, &uncompress_size);
		if (ret != LZO_E_OK) {
			printf("lzop uncompress failed\n");
			return ret;
		}

		debug("lzo uncompressed size: %ld\n", uncompress_size);
		imgaddr = (u8 *)header;
	}

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT_FULL) &&
			image_get_magic(header) == FDT_MAGIC) {
		memcpy((void *)CONFIG_SYS_LOAD_ADDR,
				imgaddr, roundup(fdt_totalsize(header), 4));
		ret = spl_parse_image_header(spl_image, bootdev,
				(struct image_header *)CONFIG_SYS_LOAD_ADDR);
	} else if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
			image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		debug("Found FIT\n");
        spl_set_bl_len(&load, 1);
		load.read = spl_board_load_read;
		spl_load_simple_fit(spl_image, &load, 0, header);
	} else {
		ret = spl_parse_image_header(spl_image, bootdev, header);
		if (ret)
			return ret;
		memcpy((void *)spl_image->load_addr, 
			imgaddr + spl_image->offset, spl_image->size);
	}

	return 0;
}

SPL_LOAD_IMAGE_METHOD("BootSpace", 0, BOOT_DEVICE_BOARD, spl_board_load_image);
