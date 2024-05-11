#ifndef _LOONGSON_STORAGE_READ_FILE_H_
#define _LOONGSON_STORAGE_READ_FILE_H_

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <blk.h>
#include <part.h>

int storage_read_file(enum uclass_id uclass_id, char* addr_desc, char* file_path,
						int last_enable, int* last_devid, int* last_partition);

#endif
