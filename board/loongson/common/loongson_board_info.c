#include <stdio.h>
#include <string.h>
#include "loongson_board_info.h"

static char* board_desc_set[] = {LS2K0300_DESC, NULL};
static char* board_name_set[] = {LS2K0300_BOARD_NAME, NULL};

char* get_board_name_by_desc(char* desc)
{
	int index;
	if (!desc)
		return NULL;
	for (index = 0; ;++index)
	{
		if (!board_desc_set[index])
			return NULL;
		if (!strcmp(board_desc_set[index], desc))
			return board_name_set[index];
	}
	return NULL;
}
