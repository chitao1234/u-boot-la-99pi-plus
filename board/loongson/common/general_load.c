#include <blk.h>
#include <part.h>
#include <gzip.h>
#include <mach/addrspace.h>
#include <asm/addrspace.h>
#include "general_load.h"

#ifdef DBG
#define GL_PRINTF(fmt, args...) printf("[GeneralLoad]"#fmt"\n", ##args)
#else
#define GL_PRINTF(fmt, args...)
#endif

static int64_t blk_partition_offset(char* interface, char* part)
{
	int p;
	struct disk_partition part_info;
	struct blk_desc *dev_desc;

	if (strstr(part, ":") == NULL)
		return 0;

	p = part_get_info_by_dev_and_name_or_num( interface, part,
			&dev_desc, &part_info, 0);
	if (p < 0)
		return 0;

	GL_PRINTF("%s[%s] part=%d, offset = %d",
			interface, part,
			p, (int)part_info.start);

	return part_info.start * part_info.blksz;
}

static int64_t net_load(char* ip, char* port,
		enum proto_t proto, char* symbol,
		int64_t offset, void* buffer, int64_t size)
{
	if(strcmp(symbol, net_boot_file_name) == 0)
		return 0;

	copy_filename(net_boot_file_name, symbol,
		      sizeof(net_boot_file_name));

	return net_loop(proto);
}

static int64_t blk_fs_load(char* interface, char* part,
		int fstype, char* symbol,
		int64_t offset, void* buffer, int64_t size)
{
	int64_t len = 0;
	int ret;
	if (fs_set_blk_dev(interface, part, fstype) == 0)
	{
		ret = fs_read(symbol, (ulong)buffer, offset,
				size, &len);
		if (ret < 0)
		{
			if (ret == -ENODATA)
				return 0;

			GL_PRINTF("ERR fs read fail %d", ret);
			return -1;
		}
	}
	else
	{
		GL_PRINTF("ERR set blk fail %s[%s]", interface, part);
	}
	return len;
}

static int64_t blk_load(struct blk_desc* dev,
		char* interface, char* part,
		int fstype, char* symbol,
		int64_t offset, void* buffer, int64_t size)
{
	int64_t len = -1;

	if (fstype == FS_TYPE_ANY)
	{
		u32 blk, cnt, n;
		offset += blk_partition_offset(interface, part);
		blk = offset/dev->blksz;
		cnt = size/dev->blksz + 1;
		n = blk_dread(dev, blk, cnt, buffer);
		len = n * dev->blksz;
	}
	else
	{
		len = blk_fs_load(interface, part, 
				fstype, symbol, 
				offset, buffer, size);
	}

	return len;
}

static int64_t gl_load(gl_device_t* gl_device, gl_format_t* gl_format, char* symbol,
		int64_t offset, void* buffer, int64_t size)
{
	int64_t len = -1;
	GL_PRINTF("load from %d(0:net,1:blk)+%lld to (%p:%lld)",
			gl_device->type, offset, buffer, size);
	switch (gl_device->type)
	{
		case GL_DEVICE_NET:
			len = net_load(gl_device->ip, gl_device->port,
					gl_format->proto, symbol,
					offset, buffer, size);
			break;
		case GL_DEVICE_BLK:
			len = blk_load((struct blk_desc*)gl_device->desc,
					gl_device->interface, gl_device->part,
					gl_format->fstype, symbol,
					offset, buffer, size);
			break;
		default:
			GL_PRINTF("ERR Device Type %d", gl_device->type);
			break;
	}

	GL_PRINTF("load %lld bytes", len);
	return len;
}

static int64_t blk_fs_burn(char* interface, char* part, int fstype, char* symbol,
		int64_t offset, void* buffer, int64_t size)
{
	int64_t len = 0;
	if (fs_set_blk_dev(interface, part, fstype) == 0)
	{
		if (fs_write(symbol, (ulong)buffer, offset,
				size, &len) < 0)
			return -1;
	}
	else
	{
		return -1;
	}
	return len;
}

static int64_t blk_burn(struct blk_desc* dev,
		char* interface, char* part,
		int fstype, char* symbol, enum gl_extra_e extra,
		int64_t offset, void* buffer, int64_t size)
{
	int64_t len = -1;

	if (fstype == FS_TYPE_ANY)
	{
		offset += blk_partition_offset(interface, part);
		if (extra & GL_EXTRA_DECOMPRESS)
		{
			// szwritebuf 越大写入越快
			// 2p500 emmc 不需要擦除，但nand需要，得谨慎考虑
			// 暂定4K
			if (gzwrite(buffer, size, dev, 1024*1024, offset, 0)
					== 0)
				len = size;
		}
		else
		{
			u32 blk, cnt, n;
			blk = offset/dev->blksz;
			cnt = size/dev->blksz + 1;
			n = blk_dwrite(dev, blk, cnt, buffer);
			len = n * dev->blksz;
			GL_PRINTF("blkdwrite: %lld", len);
		}
	}
	else
	{
		if (extra & GL_EXTRA_DECOMPRESS)
		{
			//gunzip();
			//fs_burn();
		}
		else
		{
			len = blk_fs_burn(interface, part, fstype, symbol,
					offset, buffer, size);
		}
	}

	return len;
}

static int64_t gl_burn(gl_device_t* gl_device, gl_format_t* gl_format, char* symbol,
		enum gl_extra_e extra, int64_t offset, void* buffer, int64_t size)
{
	int64_t len = -1;
	GL_PRINTF("burn to %d(0:net,1:blk)+%lld from (%p:%lld)",
			gl_device->type, offset, buffer, size);
	switch (gl_device->type)
	{
		case GL_DEVICE_BLK:
			len = blk_burn(gl_device->desc, 
					gl_device->interface, gl_device->part,
					gl_format->fstype, symbol, extra,
					offset, buffer, size);
			break;
		default:
			GL_PRINTF("ERR Device Type %d", device->type);
			break;
	}

	GL_PRINTF("burn %lld bytes", len);
	return len;
}

int general_load(gl_target_t* src, gl_target_t* dest, enum gl_extra_e extra)
{
	void* buffer = (void*)CONFIG_SYS_LOAD_ADDR;
#if CONFIG_SYS_LOAD_ADDR < HIGH_MEM_WIN_BASE
	int64_t buffer_size = 0xA000000; //160M (128M+32M)
#else
	int64_t buffer_size = 0x24000000; // 576M (512M+64M)
#endif
	int64_t offset = 0;
	int64_t load_size = 0;

	if (src == NULL)
		return -1;

	while (1) {
		load_size = gl_load(&src->gl_device, &src->gl_format, src->symbol,
				offset, buffer, buffer_size);
		if (load_size < 0)
			return -1;
		if (load_size == 0)
			return 0;
		if (dest == NULL)
			return 0;
		gl_burn(&dest->gl_device, &dest->gl_format, dest->symbol,
				extra, offset, buffer, load_size);
		offset += load_size;
		printf("load&burn %lld bytes, %lld finished\n", load_size, offset);
	}
	return 0;
}

