#include "loongson_storage_read_file.h"

static unsigned char sys_ind_set[] = {0xb, 0xc, 0xe, 0x83, 0x85, 0x0};
static char* fs_type[] = {"fatload", "ext4load", NULL};
static int command_read_file(char* fs_type, char* storage_type, int devid, int partition,
								char* addr_desc, char* file_path, int last_enable, int* last_devid, int* last_partition)
{
	int ret;
	char cmd[256];
	if (!fs_type || !storage_type || !addr_desc || !file_path)
		return -1;

	memset(cmd, 0, 256);

	// fatload usb 0:0  ${loadaddr}  update/uImage
	if (last_enable)
		sprintf(cmd, "%s %s %d:%d %s %s", fs_type, storage_type, last_devid[0], last_partition[0], addr_desc, file_path);
	else
		sprintf(cmd, "%s %s %d:%d %s %s", fs_type, storage_type, devid, partition, addr_desc, file_path);
	printf("### cur test cmd : %s\n", cmd);
	ret = run_command(cmd, 0);
	if (!ret) {
		if (last_devid)
			last_devid[0] = devid;
		if (last_partition)
			last_partition[0] = partition;
	}
	return ret;
}

static int partition_sys_ind_support(unsigned char sys_ind)
{
	int i;
	for (i = 0; ; ++i) {
		if (!sys_ind_set[i])
			break;
		if (sys_ind == sys_ind_set[i])
			return 0;
	}
	return -1;
}

static int partition_sys_ind_to_load(unsigned char sys_ind)
{
	int i;
	pr_info("sys ind is : 0x%.x\n", sys_ind);
	for (i = 0; ; ++i) {
		if (!sys_ind_set[i])
			break;
		if (sys_ind == sys_ind_set[i])
			return 0;
	}
	return 1;
}

static int part_read_update_file(struct blk_desc* desc, char* storage,
									char* addr_desc, char* file_path,
									int last_enable, int* last_devid, int* last_partition)
{
	struct disk_partition disk_info;
	int sys_ind_no_match;
	int partition;
	int i;
	int ret;

	if (!desc)
		return -1;
	// 得到分区信息
	for (partition = 0; partition < 64; ++partition) {
		// 发现 当 U盘没有分区的时候 ret 为 -93
		ret = part_get_info(desc, partition, &disk_info);
		if (!ret && !partition_sys_ind_support(disk_info.sys_ind)) {
			sys_ind_no_match = partition_sys_ind_to_load(disk_info.sys_ind);
			if (sys_ind_no_match)
				continue;

			for (i = 0; ;++i) {
				if (!fs_type[i])
					break;
				ret = command_read_file(fs_type[i], storage, desc->devnum, partition,
										addr_desc, file_path, last_enable, last_devid, last_partition);
				if (!ret)
					return 0;
			}
			if (disk_info.sys_ind == 0x83) {
				ret = command_read_file(fs_type[0], storage, desc->devnum, partition,
							addr_desc, file_path, last_enable, last_devid, last_partition);
				if (!ret)
					return 0;
			}
		}
	}
	return -1;
}

int storage_read_file(enum uclass_id uclass_id, char* addr_desc, char* file_path,
						int last_enable, int* last_devid, int* last_partition)
{
	struct blk_desc* desc;
	char* storage_type;
	int devid;
	int index;
	int ret;

	if (uclass_id == UCLASS_USB)
		storage_type = "usb";
	else if (uclass_id == UCLASS_MMC)
		storage_type = "mmc";
	else {
		printf("unknow storage_type.\n");
		return -1;
	}

	for (devid = 0;;++devid) {
		// 得到 存储设备
		desc = blk_get_devnum_by_uclass_id(uclass_id, devid);
		if (!desc) {
			printf("blk_get_devnum_by_uclass_id fail.\n");
			return -1;
		}
		ret = part_read_update_file(desc, storage_type, addr_desc, file_path, last_enable, last_devid, last_partition);
		if (!ret) {
			return 0;
		} else {
			// 如果是 0:0 这个情况
			if (!devid) {
				for (index = 0; ; ++index) {
					if (!fs_type[index]) {
						ret = -1;
						break;
					}
					ret = command_read_file(fs_type[index], storage_type, 0, 0, addr_desc, file_path, last_enable, last_devid, last_partition);
					if (!ret)
						break;
				}
			}
			return ret;
		}
	}
	return -1;
}
