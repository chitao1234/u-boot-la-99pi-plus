#include <common.h>
#include <command.h>
#include <env.h>
#include <watchdog.h>
#include <malloc.h>
#include <linux/string.h>
#include <linux/fb.h>
#include "dm.h"
#include "loongson_board_info.h"
#include "bdinfo/bdinfo.h"
#include "board_kernel_product.h"
#include "loongson_update.h"

static char** cur_product_set;
static char** cur_product_option_set;

static inline int atoi_s(char* str, int* num)
{
	int i;
	int temp;
	if (!str || !num)
		return -1;
	if (str[0] == '0' && str[1] == '\0') {
		num[0] = 0;
		return 0;
	}

	temp = 0;
	for (i = 0; ;++i) {
		if (!str[i])
			break;
		if (str[i] >= '0' && str[i] <= '9') {
			temp *= 10;
			temp += str[i] - '0';
		} else
			return -1;
	}
	if (!temp)
		return -1;

	num[0] = temp;
	return 0;
}

static int write_ofnode_smbios_board_product(char* new_name)
{
	int res;
	struct udevice *dev = NULL;
	ofnode parent_node, node;

	if (!new_name)
		return -EFAULT;

	// find smbios baseboard and write product
	uclass_first_device(UCLASS_SYSINFO, &dev);
	if (!dev)
		return -EBUSY;
	parent_node = dev_read_subnode(dev, "smbios");
	if (!ofnode_valid(parent_node))
			return -EBUSY;
	dev = NULL;
	node = ofnode_find_subnode(parent_node, "baseboard");
	if (!ofnode_valid(node))
			return -EBUSY;

	res = ofnode_write_string(node, "product", new_name);
	// 谨记 CONFIG_OF_LIVE 打开
	if (!res)
		printf("smbios baseboard handle product name change : %s\n", new_name);
	return res;
}

static char* generic_null_option_set[] = {"no option=""\0", NULL};
static char* board_name_set[] = {LS2K0300_BOARD_NAME,
									NULL};
static char** product_set_ptr_set[] = {NULL, NULL, NULL};
static char** product_option_set_ptr_set[] = {generic_null_option_set, generic_null_option_set, generic_null_option_set};

static inline int get_board_index(char* board_name)
{
	int i;
	if (!board_name)
		return -1;

	// check board name to find product_set_ptr_set index
	for (i = 0; ;++i) {
		if (!board_name_set[i])
			return -1;
		if (!strcmp(board_name_set[i], board_name))
			return i;
	}
}

static char* get_product(char* board_name)
{
	int i;
	char* product_id_str;
	int product_id;
	int set_index;
	char** product_set;
	if (!board_name)
		return NULL;

	// get product set index in bdinfo
	product_id_str = bdinfo_get(BDI_ID_BOARD_PRODUCT_ID);
	if (atoi_s(product_id_str, &product_id))
		return NULL;

	//product_id = 2;
	if(!product_id)
		return NULL;

	// 前面的 product_id 的 获取 建议先做 因为后面的 逐个字符串比较比较慢

	// check board name to find product_set_ptr_set index
	set_index = get_board_index(board_name);
	if (set_index == -1)
		return NULL;

	// not other product name
	if (!product_set_ptr_set[set_index])
		return NULL;

	product_set = product_set_ptr_set[set_index];

	for (i = 0; ;++i) {
		// product_id too big
		if (!product_set[i])
			return NULL;
		if (i == product_id)
			return product_set[i];
	}
}

static inline char* get_board_name(void)
{
	return bdinfo_get(BDI_ID_NAME);
}

int update_board_kernel_product()
{
	char* board_name;
	char* product;
	if (!(board_name = get_board_name()))
		return -ENOMEM;

	product = get_product(board_name);
	// not found new product name just keep default which in dts
	if (!product)
		return 0;
	return write_ofnode_smbios_board_product(product);
}

int set_board_kernel_product_id(char* id)
{
	int res;
	char* board_name;
	int index = 0;
	if (!id || id[0] == '\0')
		return -ENOMEM;

	//check id is vaild
	res = atoi_s(id, &index);
	if (res || !index)
		return res;

	if (!(board_name = get_board_name()))
		return -ENOMEM;

	//没必要检查这个板卡会不会有复用关系 因为最后设置的时候就会自行检查 而且 菜单出现选择时 如果没得选 根本不会设置。
	res = bdinfo_set(BDI_ID_BOARD_PRODUCT_ID ,id);
#ifndef CONFIG_SPL_BUILD
	if (!res)
		res = bdinfo_save();
#endif
	printf("###################################################\n");
	printf("### board name   : %s\n", board_name);
	if (likely(cur_product_set)) {
		printf("### option desc  : %s\n", cur_product_set[index]);
		printf("### product name : %s\n", cur_product_option_set[index]);
	}
	printf("### result       : %s\n", (res ? "FAIL" : "SUCCESS"));
	printf("###################################################\n");
	return res;
}

// 得到单个菜单选项
char* get_board_kernel_product_menu_option(int n)
{
	char* board_name;
	int set_index;

	if (!(board_name = get_board_name()))
		return NULL;

	set_index = get_board_index(board_name);
	if (set_index == -1)
		return NULL;

	if (!product_set_ptr_set[set_index])
		return NULL;

	// 因为 只有第一次拿选项 才会 判断为 NULL 所以 使用 unlikely
	if (unlikely(!cur_product_set)) {
		cur_product_set = product_set_ptr_set[set_index];
		cur_product_option_set = product_option_set_ptr_set[set_index];
	}
	return product_option_set_ptr_set[set_index][n + 1];
}
